#include "Precomp.h"
#include "Scripting/ScriptFunc.h"

#include "Meta/MetaFunc.h"
#include "Meta/MetaManager.h"
#include "Meta/MetaProps.h"
#include "Meta/MetaType.h"
#include "Scripting/ScriptTools.h"
#include "Scripting/Nodes/EntryAndReturnScriptNode.h"
#include "Scripting/ScriptNode.h"
#include "GSON/GSONBinary.h"
#include "Assets/Script.h"
#include "Core/AssetManager.h"
#include "Core/VirtualMachine.h"
#include "Utilities/Events.h"

namespace CE::Internal
{
	template <typename VectorType, typename IdType>
	auto FindElement(VectorType& vec, IdType id)
	{
		// Overflow is id.Get() == 0 is intentional
		auto asInt = id.Get() - 1;

		if (asInt >= vec.size())
		{
			return vec.end();
		}

#ifdef REMOVE_FROM_SCRIPTS_ENABLED
		if (IsNull(vec[asInt]))
		{
			return vec.end();
		}
#endif

		return vec.begin() + asInt;
	}
}

CE::ScriptFunc::ScriptFunc(const Script& script, const std::string_view name) :
	mName(name),
	mNameOfScriptAsset(script.GetName()),
	mTypeIdOfScript(Name::HashString(script.GetName())),
	mProps(std::make_unique<MetaProps>())
{}

CE::ScriptFunc::ScriptFunc(const Script& script, const EventBase& event) :
	mName(event.mName),
	mNameOfScriptAsset(script.GetName()),
	mTypeIdOfScript(Name::HashString(script.GetName())),
	mReturns(event.mOutputPin.mTypeTraits != MakeTypeTraits<void>() ? 
		std::optional<ScriptVariableTypeData>{ ScriptVariableTypeData{ event.mOutputPin.mTypeTraits, event.mOutputPin.mName } } :
		std::nullopt),
	mBasedOnEvent(&event),
	mProps(std::make_unique<MetaProps>())
{
	for (const MetaFuncNamedParam& param : event.mInputPins)
	{
		mParams.emplace_back(param.mTypeTraits, param.mName);
	}
}

CE::ScriptFunc::~ScriptFunc()
{
	for (ScriptNode& node : mNodes)
	{
		delete& node;
	}
}

void CE::ScriptFunc::DeclareMetaFunc(MetaType& addToType)
{
	MetaFunc* declared{};
	if (IsEvent())
	{
	 	declared = &mBasedOnEvent->Declare(mTypeIdOfScript, addToType);
	}
	else
	{

		std::vector<MetaFuncNamedParam> metaParams{};

		for (ScriptVariableTypeData& scriptParam : GetParameters(false))
		{
			scriptParam.RefreshTypePointer();

			if (scriptParam.TryGetType() == nullptr)
			{
				VirtualMachine::PrintError(ScriptError{ ScriptError::UnreflectedType, *this, scriptParam.GetTypeName() });
				continue;
			}

			metaParams.emplace_back(TypeTraits{ scriptParam.TryGetType()->GetTypeId(), scriptParam.GetTypeForm() }, scriptParam.GetName());
		}

		MetaFuncNamedParam metaReturn{ MakeTypeTraits<void>() };

		if (mReturns.has_value())
		{
			if (mReturns->TryGetType() == nullptr)
			{
				VirtualMachine::PrintError(ScriptError{ ScriptError::UnreflectedType, *this, mReturns->GetTypeName() });
			}
			else
			{
				metaReturn = { { mReturns->TryGetType()->GetTypeId(), mReturns->GetTypeForm() }, mReturns->GetTypeName() };
			}
		}

		declared = &addToType.AddFunc([](MetaFunc::DynamicArgs, MetaFunc::RVOBuffer) -> FuncResult
			{
				return { "There were unresolved compilation errors" };
			},
			mName,
			metaReturn,
			metaParams
		);

		declared->GetProperties().Set(Props::sIsScriptPure, IsPure()).Add(Props::sIsScriptableTag);
	}

	declared->GetProperties().Add(*mProps);
}

void CE::ScriptFunc::DefineMetaFunc(MetaFunc& func)
{
	ASSERT(VirtualMachine::Get().GetErrors(*this).empty());

	// Get a reference to our script to prevent unloading
	AssetHandle<Script> ourScript = AssetManager::Get().TryGetAsset<Script>(mNameOfScriptAsset);

	if (ourScript == nullptr)
	{
		LOG(LogScripting, Error, "We *should* be able to compile this scriptfunc, except we were somehow not \
able to receive our script asset {}. Compilation will silently fail.",
mNameOfScriptAsset);
		return;
	}

	if (IsEvent())
	{
		mBasedOnEvent->Define(func, *this, ourScript);
	}
	else
	{
		Expected<std::reference_wrapper<const ScriptNode>, std::string> possibleFirstNode = GetFirstNode();
		Expected<const FunctionEntryScriptNode*, std::string> possibleEntryNode = GetEntryNode();

		ASSERT(!possibleFirstNode.HasError() && "Should've been part of the compilation errors");

		func.RedirectFunction([this, ourScript, firstNode = possibleFirstNode.GetValue(), entry = possibleEntryNode.GetValue()]
		(MetaFunc::DynamicArgs args, MetaFunc::RVOBuffer rvoBuffer) -> FuncResult
			{
				return VirtualMachine::Get().ExecuteScriptFunction(args, rvoBuffer, *this, firstNode, entry);
			});
	}
}

std::vector<std::reference_wrapper<CE::ScriptLink>> CE::ScriptFunc::GetAllLinksConnectedToPin(PinId id)
{
	std::vector<std::reference_wrapper<ScriptLink>> returnValue{};

	for (ScriptLink& link : GetLinks())
	{
		if (link.GetInputPinId() == id || link.GetOutputPinId() == id)
		{
			returnValue.push_back(link);
		}
	}

	return returnValue;
}

std::vector<std::reference_wrapper<const CE::ScriptLink>> CE::ScriptFunc::GetAllLinksConnectedToPin(PinId id) const
{
	std::vector<std::reference_wrapper<const ScriptLink>> returnValue{};

	for (const ScriptLink& link : GetLinks())
	{
		if (link.GetInputPinId() == id || link.GetOutputPinId() == id)
		{
			returnValue.push_back(link);
		}
	}

	return returnValue;
}

std::vector<CE::ScriptVariableTypeData> CE::ScriptFunc::GetParameters(bool ignoreObjectInstanceIfMemberFunction) const
{
	std::vector<ScriptVariableTypeData> params{};

	if (!mIsStatic
		&& !ignoreObjectInstanceIfMemberFunction)
	{
		static_assert(Script::sIsTypeIdTheSameAsNameHash);
		params.emplace_back(TypeTraits{ mTypeIdOfScript, TypeForm::Ref });
	}

	params.insert(params.end(), mParams.begin(), mParams.end());
	return params;
}

void CE::ScriptFunc::CollectErrors(ScriptErrorInserter inserter, const Script& owningScript) const
{
	ASSERT(owningScript.GetName() == mNameOfScriptAsset);

	{ // Check if our name is unique
		const ptrdiff_t numberWithThisName = std::count_if(owningScript.GetFunctions().begin(), owningScript.GetFunctions().end(),
			[name = mName](const ScriptFunc& func)
			{
				return func.GetName() == name;
			});

		if (numberWithThisName != 1)
		{
			if (numberWithThisName == 0)
			{
				LOG(LogScripting, Error, "OwningScript {} was not the script that owns {}, found while collecting errors",
					owningScript.GetName(),
					mName);
			}

			inserter = { ScriptError::Type::NameNotUnique, *this };
		}
	}

	const auto allEvents = GetAllEvents();

	// Check if our name is not an event
	if (!IsEvent()
		&& std::any_of(allEvents.begin(), allEvents.end(),
			[ourName = mName](const EventBase& event)
			{
				return event.mName == ourName;
			}))
	{
		inserter = { ScriptError::Type::NameNotUnique, *this, Format("Name {} is reserved for the event of the same name", mName) };
	}

	{ // Check if the types we are using are still valid
		for (const ScriptVariableTypeData& param : mParams)
		{
			const MetaType* const type = param.TryGetType();

			if (type == nullptr)
			{
				inserter = { ScriptError::Type::UnreflectedType, *this, param.GetTypeName() };
			}
			else if (!CanTypeBeUsedInScripts(*type, param.GetTypeForm()))
			{
				inserter = { ScriptError::Type::TypeCannotBeReferencedFromScripts, *this, param.GetTypeName() };
			}
		}

		if (mReturns.has_value())
		{
			const MetaType* const type = mReturns->TryGetType();

			if (type == nullptr)
			{
				inserter = { ScriptError::Type::UnreflectedType, *this, mReturns->GetTypeName() };
			}
			else if (!CanTypeBeUsedInScripts(*type, mReturns->GetTypeForm()))
			{
				inserter = { ScriptError::Type::TypeCannotBeReferencedFromScripts, *this, mReturns->GetTypeName() };
			}
		}
	}

	{ // Check if we have a valid node to start executing from
		Expected<std::reference_wrapper<const ScriptNode>, std::string> firstNode = GetFirstNode();

		if (firstNode.HasError())
		{
			inserter = { ScriptError::Type::NotPossibleToEnterFunc, *this,
				firstNode.GetError() };
		}
	}

	for (const ScriptNode& node : GetNodes())
	{
		node.CollectErrors(inserter, *this);
	}

	for (const ScriptLink& link : GetLinks())
	{
		link.CollectErrors(inserter, *this);
	}
}

Expected<std::reference_wrapper<const CE::ScriptNode>, std::string> CE::ScriptFunc::GetFirstNode() const
{
	Expected<const FunctionEntryScriptNode*, std::string> entryNode = GetEntryNode();

	if (entryNode.HasError())
	{
		return { std::move(entryNode.GetError()) };
	}

	uint32 numOfReturnNodes{};
	const ScriptNode* funcReturn{};

	// Find the first node
	for (const ScriptNode& node : GetNodes())
	{
		if (node.GetType() == ScriptNodeType::FunctionReturn)
		{
			++numOfReturnNodes;
			funcReturn = &node;
		}
	}

	if (IsPure() // We require only 1 return node if this function is pure
		&& numOfReturnNodes != 1)
	{
		return { Format("Expected 1 return node, but found {}", numOfReturnNodes) };
	}

	const ScriptNode* firstNode = IsPure() ? funcReturn : entryNode.GetValue();

	if (firstNode == nullptr) // This can only be true if there is a flaw in my logic above, or my definition of Pure changes.
	{
		return { Format("Compiler error - First node was nullptr, should not be possible at this point.") };
	}

	return { *firstNode };
}

Expected<const CE::FunctionEntryScriptNode*, std::string> CE::ScriptFunc::GetEntryNode() const
{
	uint32 numOfEntryNodes{};
	const ScriptNode* funcEntry{};

	for (const ScriptNode& node : GetNodes())
	{
		if (node.GetType() == ScriptNodeType::FunctionEntry)
		{
			++numOfEntryNodes;
			funcEntry = &node;
		}
	}

	if ((!mParams.empty() // in this case, the entry node has pins representing the parameters
		|| !IsPure()) // We need to have only one entry flow pin to know where execution starts
		&& numOfEntryNodes != 1)
	{
		return { Format("Expected 1 entry node, but found {}", numOfEntryNodes) };
	}

	return { static_cast<const FunctionEntryScriptNode*>(funcEntry) };
}

std::tuple<std::span<std::reference_wrapper<CE::ScriptNode>>, std::span<CE::ScriptLink>, std::span<CE::ScriptPin>> CE::ScriptFunc::AddCondensed(std::vector<std::unique_ptr<ScriptNode>>&& nodes,
	std::vector<ScriptLink>&& links, std::vector<ScriptPin>&& pins)
{
	std::sort(pins.begin(), pins.end(), [](const ScriptPin& lhs, const ScriptPin& rhs)
		{
			if (lhs.GetNodeId() != rhs.GetNodeId())
			{
				return lhs.GetNodeId().Get() < rhs.GetNodeId().Get();
			}

			if (lhs.GetKind() != rhs.GetKind())
			{
				return lhs.GetKind() == ScriptPinKind::Input;
			}

			return lhs.GetId().Get() < rhs.GetId().Get();
		});

	std::unordered_map<NodeId, NodeId> nodeRemappings{};
	std::unordered_map<LinkId, LinkId> linkRemappings{};
	std::unordered_map<PinId, PinId> pinRemappings{ { 0, 0 } };

	ScriptIdInserter::container_type idsToRemap{};

	for (size_t i = 0; i < nodes.size(); i++)
	{
		std::unique_ptr<ScriptNode>& node = nodes[i];
		nodeRemappings.emplace(node->GetId(), static_cast<NodeId::ValueType>(mNodes.size() + i + 1));

		node->GetReferencesToIds(std::back_inserter(idsToRemap));
	}

	for (size_t i = 0; i < links.size(); i++)
	{
		ScriptLink& link = links[i];
		linkRemappings.emplace(link.GetId(), static_cast<LinkId::ValueType>(mLinks.size() + i + 1));

		link.GetReferencesToIds(std::back_inserter(idsToRemap));
	}

	for (size_t i = 0; i < pins.size(); i++)
	{
		ScriptPin& pin = pins[i];
		pinRemappings.emplace(pin.GetId(), static_cast<PinId::ValueType>(mPins.size() + i + 1));

		pin.GetReferencesToIds(std::back_inserter(idsToRemap));
	}

	for (auto& refToId : idsToRemap)
	{
		switch (refToId.index())
		{
		case 0:
		{
			LinkId& id = std::get<0>(refToId);
			id = linkRemappings.at(id.Get());
			break;
		}
		case 1:
		{
			PinId& id = std::get<1>(refToId);
			id = pinRemappings.at(id.Get());
			break;
		}
		case 2:
		{
			NodeId& id = std::get<2>(refToId);
			id = nodeRemappings.at(id.Get());
			break;
		}
		default: ABORT;
		}
	}

	const size_t nodeSizeBefore = mNodes.size();
	const size_t pinSizeBefore = mPins.size();
	const size_t linkSizeBefore = mLinks.size();

	mNodes.reserve(nodeSizeBefore + nodes.size());
	for (auto& node : nodes)
	{
		mNodes.emplace_back(*node.release());
	}

	mPins.reserve(pinSizeBefore + pins.size());
	for (ScriptPin& pin : pins)
	{
		mPins.emplace_back(std::move(pin));
	}

	mLinks.reserve(linkSizeBefore + links.size());
	for (ScriptLink& link : links)
	{
		GetPin(link.GetInputPinId()).SetCachedPinWeAreLinkedWith(link.GetOutputPinId());
		GetPin(link.GetOutputPinId()).SetCachedPinWeAreLinkedWith(link.GetInputPinId());
		mLinks.emplace_back(std::move(link));
	}

#ifdef ASSERTS_ENABLED
	for (size_t i = 0; i < mNodes.size(); i++)
	{
		if (mNodes[i].get().GetId() == NodeId::Invalid)
		{
			continue;
		}

		ASSERT(mNodes[i].get().GetId().Get() == i + 1);

		for (const ScriptPin& input : mNodes[i].get().GetInputs(*this))
		{
			ASSERT(input.GetKind() == ScriptPinKind::Input);
		}

		for (const ScriptPin& output : mNodes[i].get().GetOutputs(*this))
		{
			ASSERT(output.GetKind() == ScriptPinKind::Output);
		}
	}

	for (size_t i = 0; i < mLinks.size(); i++)
	{
		if (mLinks[i].GetId() == LinkId::Invalid)
		{
			continue;
		}

		ASSERT(mLinks[i].GetId().Get() == i + 1);
		ASSERT(GetPin(mLinks[i].GetInputPinId()).GetKind() == ScriptPinKind::Input);
		ASSERT(GetPin(mLinks[i].GetOutputPinId()).GetKind() == ScriptPinKind::Output);
	}

	for (size_t i = 0; i < mPins.size(); i++)
	{
		ASSERT(mPins[i].GetId().Get() == i + 1 || mPins[i].GetId() == PinId::Invalid);
	}
#endif

	return { { mNodes.data() + nodeSizeBefore, nodes.size() }, { mLinks.data() + linkSizeBefore, links.size() }, { mPins.data() + pinSizeBefore, pins.size() } };
}

void CE::ScriptFunc::PostDeclarationRefresh()
{
	for (ScriptVariableTypeData& param : mParams)
	{
		param.RefreshTypePointer();
	}

	if (mReturns.has_value())
	{
		mReturns->RefreshTypePointer();
	}

	for (ScriptNode& node : GetNodes())
	{
		node.PostDeclarationRefresh(*this);
	}

	for (ScriptPin& pin : GetPins())
	{
		pin.PostDeclarationRefresh();
	}
}

#ifdef REMOVE_FROM_SCRIPTS_ENABLED

CE::ScriptLink* CE::ScriptFunc::TryAddLink(const ScriptPin& pinA, const ScriptPin& pinB)
{
	if (!CanCreateLink(pinA, pinB))
	{
		return nullptr;
	}

	const ScriptPin* inputPin = &pinA;
	const ScriptPin* outputPin = &pinB;

	if (pinB.IsInput())
	{
		std::swap(inputPin, outputPin);
	}

	ASSERT(inputPin->IsInput());
	ASSERT(outputPin->IsOutput());

	if (outputPin->IsFlow())
	{
		const std::vector<std::reference_wrapper<ScriptLink>> existingLinksFromOutput = GetAllLinksConnectedToPin(outputPin->GetId());

		for (const ScriptLink& link : existingLinksFromOutput)
		{
			RemoveLink(link.GetId());
		}
	}

	if (!inputPin->IsFlow())
	{
		const std::vector<std::reference_wrapper<ScriptLink>> existingLinksFromInput = GetAllLinksConnectedToPin(inputPin->GetId());

		for (const ScriptLink& link : existingLinksFromInput)
		{
			RemoveLink(link.GetId());
		}
	}

	const_cast<ScriptPin*>(inputPin)->SetCachedPinWeAreLinkedWith(outputPin->GetId());
	const_cast<ScriptPin*>(outputPin)->SetCachedPinWeAreLinkedWith(inputPin->GetId());

	return &mLinks.emplace_back(static_cast<LinkId::ValueType>(mLinks.size() + 1), *inputPin, *outputPin);
}

void CE::ScriptFunc::RemoveNode(NodeId nodeId)
{
	ScriptNode* node = TryGetNode(nodeId);

	if (node == nullptr)
	{
		// LOG(LogAssets, Warning, "Node was already removed");
		return;
	}

	node->ClearPins(*this);
	node->SetId(NodeId::Invalid);

#ifdef ASSERTS_ENABLED
	for (const ScriptNode& nodeTest : GetNodes())
	{
		ASSERT(nodeTest.GetId() != NodeId::Invalid && "Iterator not working, returned null");
	}
#endif // ASSERTS_ENABLED
}

void CE::ScriptFunc::RemoveLink(LinkId linkId)
{
	ScriptLink* link = TryGetLink(linkId);

	if (link == nullptr)
	{
		// LOG(LogAssets, Warning, "Link was already removed");
		return;
	}

	link->SetId(LinkId::Invalid);

	ScriptPin& inputPin = GetPin(link->GetInputPinId());
	ScriptPin& outputPin = GetPin(link->GetOutputPinId());

	auto linksToInput = GetAllLinksConnectedToPin(inputPin.GetId());
	auto linksToOutput = GetAllLinksConnectedToPin(outputPin.GetId());

	inputPin.SetCachedPinWeAreLinkedWith(linksToInput.empty() ? PinId::Invalid : linksToInput[0].get().GetOutputPinId());
	outputPin.SetCachedPinWeAreLinkedWith(linksToOutput.empty() ? PinId::Invalid : linksToOutput[0].get().GetInputPinId());

#ifdef ASSERTS_ENABLED
	for (const ScriptLink& linkTest : GetLinks())
	{
		ASSERT(linkTest.GetId() != LinkId::Invalid && "Iterator not working, returned null");
	}
#endif // ASSERTS_ENABLED
}

void CE::ScriptFunc::FreePins(PinId firstPin, uint32 amount)
{
	auto it = Internal::FindElement(mPins, firstPin);
	ASSERT(it != mPins.end());

	std::span<ScriptPin> pins = { &*it, amount };

	for (const ScriptPin& pin : pins)
	{
		std::vector<std::reference_wrapper<ScriptLink>> linksToBreak = GetAllLinksConnectedToPin(pin.GetId());
		std::vector<LinkId> idsOfLinksToBreak{};

		std::transform(linksToBreak.begin(), linksToBreak.end(), std::back_inserter(idsOfLinksToBreak),
			[](const ScriptLink& link)
			{
				return link.GetId();
			});

		for (const LinkId linkToBreakId : idsOfLinksToBreak)
		{
			RemoveLink(linkToBreakId);
		}
	}

	for (ScriptPin& pin : pins)
	{
		pin.SetId(PinId::Invalid);
	}

#ifdef ASSERTS_ENABLED
	for (const ScriptPin& pin : GetPins())
	{
		ASSERT(pin.GetId() != PinId::Invalid && "Iterator not working, returned null");
	}
#endif // ASSERTS_ENABLED
}

bool CE::Internal::IsNull(const ScriptLink& link)
{
	return link.GetId() == LinkId::Invalid;
}

bool CE::Internal::IsNull(const ScriptPin& pin)
{
	return pin.GetId() == PinId::Invalid;
}

bool CE::Internal::IsNull(const ScriptNode& node)
{
	return node.GetId() == NodeId::Invalid;
}
#endif // REMOVE_FROM_SCRIPTS_ENABLED

void CE::ScriptFunc::SerializeTo(BinaryGSONObject& object) const
{
	if (IsEvent())
	{
		object.AddGSONMember("event") << std::string{ mBasedOnEvent->mName };
	}
	else
	{
		if (mReturns.has_value())
		{
			object.AddGSONMember("returnInfo") << mReturns;
		}

		object.AddGSONMember("paramsInfo") << mParams;
		object.AddGSONMember("isPure") << mIsPure;
		object.AddGSONMember("isStatic") << mIsStatic;
	}

	object.AddGSONMember("name") << mName;
	object.AddGSONMember("props") << *mProps;

	BinaryGSONObject& nodes = object.AddGSONObject("nodes");
	nodes.ReserveChildren(mNodes.size());

	for (const ScriptNode& node : GetNodes())
	{
		node.SerializeTo(nodes.AddGSONObject(""), *this);
	}

	BinaryGSONObject& links = object.AddGSONObject("links");
	links.ReserveChildren(mLinks.size());

	for (const ScriptLink& link : GetLinks())
	{
		link.SerializeTo(links.AddGSONObject(""));
	}
}

std::optional<CE::ScriptFunc> CE::ScriptFunc::DeserializeFrom(const BinaryGSONObject& object, const Script& owningScript, const uint32 version)
{
	const BinaryGSONObject* linksObj = object.TryGetGSONObject("links");
	const BinaryGSONObject* nodesObj = object.TryGetGSONObject("nodes");
	const BinaryGSONMember* serializedName = object.TryGetGSONMember("name");

	if (serializedName == nullptr
		|| linksObj == nullptr
		|| nodesObj == nullptr)
	{
		UNLIKELY;
		LOG(LogAssets, Warning, "Failed to deserialize script function, missing values");
		return std::nullopt;
	}

	std::string name{};
	*serializedName >> name;

	std::vector<std::unique_ptr<ScriptNode>> nodes{};
	std::vector<ScriptLink> links{};
	std::vector<ScriptPin> pins{};

	for (const BinaryGSONObject& node : nodesObj->GetChildren())
	{
		if (nodes.emplace_back(ScriptNode::DeserializeFrom(node, std::back_inserter(pins), version)) == nullptr)
		{
			UNLIKELY;
			return std::nullopt;
		}
	}

	for (const BinaryGSONObject& link : linksObj->GetChildren())
	{
		std::optional<ScriptLink> deserializedLink = ScriptLink::DeserializeFrom(link);

		if (!deserializedLink.has_value())
		{
			UNLIKELY;
			return std::nullopt;
		}

		links.emplace_back(std::move(*deserializedLink));
	}

	std::optional<ScriptFunc> returnValue{};

	const BinaryGSONMember* serializedEventName = object.TryGetGSONMember("event");

	if (serializedEventName != nullptr)
	{
		std::string eventName{};
		*serializedEventName >> eventName;

		for (const EventBase& event : GetAllEvents())
		{
			if ((static_cast<int>(event.mFlags) & static_cast<int>(EventFlags::NotScriptable))
				|| event.mName != eventName)
			{
				continue;
			}

			returnValue.emplace(owningScript, event);
			break;
		}

		if (!returnValue.has_value())
		{
			UNLIKELY;
			LOG(LogAssets, Warning, "Event {} does not exist anymore, {} will no longer have this function", eventName, owningScript.GetName());
			return std::nullopt;
		}
	}
	else
	{
		const BinaryGSONMember* serializedReturn = object.TryGetGSONMember("returnInfo");
		const BinaryGSONMember* serializedParams = object.TryGetGSONMember("paramsInfo");
		const BinaryGSONMember* isPure = object.TryGetGSONMember("isPure");
		const BinaryGSONMember* isStatic = object.TryGetGSONMember("isStatic");

		if (serializedName == nullptr
			|| serializedParams == nullptr
			|| isPure == nullptr
			|| isStatic == nullptr)
		{
			UNLIKELY;
			LOG(LogAssets, Warning, "Failed to deserialize script function, missing values");
			return std::nullopt;
		}

		returnValue.emplace(owningScript, name);

		*isPure >> returnValue->mIsPure;
		*isStatic >> returnValue->mIsStatic;

		if (serializedReturn != nullptr)
		{
			*serializedReturn >> returnValue->mReturns;
		}

		*serializedParams >> returnValue->mParams;
	}

	returnValue->AddCondensed(std::move(nodes), std::move(links), std::move(pins));

	const BinaryGSONMember* serializedProps = object.TryGetGSONMember("props");

	if (serializedProps != nullptr)
	{
		*serializedProps >> *returnValue->mProps;
	}

	return returnValue;
}

std::span<CE::ScriptPin> CE::ScriptFunc::AllocPins(NodeId calledFrom,
	std::vector<ScriptVariableTypeData>&& inputs, std::vector<ScriptVariableTypeData>&& outputs)
{
	const size_t currentSize = mPins.size();

	for (ScriptVariableTypeData& param : inputs)
	{
		mPins.emplace_back(static_cast<PinId::ValueType>(mPins.size() + 1), calledFrom, ScriptPinKind::Input, std::move(param));
	}

	for (ScriptVariableTypeData& param : outputs)
	{
		mPins.emplace_back(static_cast<PinId::ValueType>(mPins.size() + 1), calledFrom, ScriptPinKind::Output, std::move(param));
	}

	return { &mPins[currentSize], inputs.size() + outputs.size() };
}

CE::ScriptNode* CE::ScriptFunc::TryGetNode(NodeId id)
{
	auto it = Internal::FindElement(mNodes, id);
	return it == mNodes.end() ? nullptr : &it->get();
}

const CE::ScriptNode* CE::ScriptFunc::TryGetNode(NodeId id) const
{
	auto it = Internal::FindElement(mNodes, id);
	return it == mNodes.end() ? nullptr : &it->get();
}

CE::ScriptLink* CE::ScriptFunc::TryGetLink(LinkId id)
{
	auto it = Internal::FindElement(mLinks, id);
	return it == mLinks.end() ? nullptr : &*it;
}

const CE::ScriptLink* CE::ScriptFunc::TryGetLink(LinkId id) const
{
	auto it = Internal::FindElement(mLinks, id);
	return it == mLinks.end() ? nullptr : &*it;
}

CE::ScriptPin* CE::ScriptFunc::TryGetPin(PinId id)
{
	auto it = Internal::FindElement(mPins, id);
	return it == mPins.end() ? nullptr : &*it;
}

const CE::ScriptPin* CE::ScriptFunc::TryGetPin(PinId id) const
{
	auto it = Internal::FindElement(mPins, id);
	return it == mPins.end() ? nullptr : &*it;
}