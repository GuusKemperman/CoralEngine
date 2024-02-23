#include "Precomp.h"
#include "Core/VirtualMachine.h"

#include "Core/AssetManager.h"
#include "Meta/MetaManager.h"
#include "Meta/MetaType.h"
#include "Meta/MetaFunc.h"
#include "Scripting/ScriptTools.h"
#include "Scripting/Nodes/MetaFuncScriptNode.h"
#include "Scripting/Nodes/MetaMemberScriptNode.h"
#include "Scripting/Nodes/EntryAndReturnScriptNode.h"
#include "Scripting/Nodes/ControlScriptNodes.h"
#include "Assets/Script.h"
#include "Meta/MetaTools.h"
#include "Utilities/Reflect/ReflectComponentType.h"
#include "Scripting/ScriptConfig.h"

void Engine::VirtualMachine::PostConstruct()
{
	Recompile();
}

Engine::VirtualMachine::~VirtualMachine()
{
	// I mean the meta manager will be destroyed right after us, so *technically* we can skip this step
	// But it's still polite to clean up our own mess
	DestroyAllTypesCreatedThroughScripts();
}

void Engine::VirtualMachine::Recompile()
{
	LOG(LogScripting, Message, "Recompiling...");

	ClearCompilationResult();

	std::vector<WeakAsset<Asset>> allAssets = AssetManager::Get().GetAllAssets();

	std::vector<std::pair<std::reference_wrapper<MetaType>, std::reference_wrapper<Script>>> createdTypes{};

	for (WeakAsset<Asset>& asset : allAssets)
	{
		if (!asset.GetAssetClass().IsDerivedFrom<Script>())
		{
			continue;
		}

		Script& script = const_cast<Script&>(*std::static_pointer_cast<const Script>(asset.MakeShared()));
		
		MetaType* type = script.DeclareMetaType();

		if (type != nullptr)
		{
			createdTypes.emplace_back(*type, script);
		}
	}

	for (auto& [createdType, script] : createdTypes)
	{
		script.get().DeclareMetaFunctions(createdType);
	}

	for (auto& [createdType, script] : createdTypes)
	{
		script.get().PostDeclarationRefresh();

		const size_t numOfErrorsBefore = mErrorsFromLastCompilation.size();
		script.get().CollectErrors(std::back_inserter(mErrorsFromLastCompilation));
		script.get().DefineMetaType(createdType, numOfErrorsBefore != mErrorsFromLastCompilation.size());
	}

	PrintCompileErrors();

	LOG(LogScripting, Message, "Compilation completed");
}

void Engine::VirtualMachine::ClearCompilationResult()
{
	DestroyAllTypesCreatedThroughScripts();
	mErrorsFromLastCompilation.clear();
}

std::vector<std::reference_wrapper<const Engine::ScriptError>> Engine::VirtualMachine::GetErrors(
	const ScriptLocation& location) const
{
	std::vector<std::reference_wrapper<const ScriptError>> returnValue{};

	for (const ScriptError& error : mErrorsFromLastCompilation)
	{
		if (error.GetOrigin().IsLocationEqualToOrInsideOf(location))
		{
			returnValue.emplace_back(error);
		}
	}

	return returnValue;
}

Engine::FuncResult Engine::VirtualMachine::ExecuteScriptFunction(MetaFunc::DynamicArgs args,
	MetaFunc::RVOBuffer rvoBuffer,
	const ScriptFunc& func,
	const ScriptNode& firstNode,
	const FunctionEntryScriptNode* entryNode)
{
	// LOG_FMT(LogScripting, Verbose, "Calling {}::{}", func.GetNameOfScriptAsset(), func.GetName());
	VMContext context{ func };

	if (context.mCachedValues == nullptr)
	{
		PrintError({ ScriptError::StackOverflow, { context.mFunc } });
		return "Stack overflow";
	}

	if (!func.IsStatic())
	{
		context.mThis = &args[0];
	}

	if (entryNode != nullptr)
	{
		const Span<const ScriptPin> entryPins = entryNode->GetOutputs(func);

		// Push the arguments we received to the cache. If this is a field function,
		// skip the first argument; there is no pin for it in the entry node
		for (uint32 argNum = !func.IsStatic(), pinIndex = 0; argNum < args.size();)
		{
			ASSERT(pinIndex < entryPins.size());
			const ScriptPin& pin = entryPins[pinIndex];

			if (pin.IsFlow())
			{
				++pinIndex;
				continue;
			}

			MetaAny& arg = args[argNum];

			VMContext::CachedValue& cachedValue = FindCachedValue(context, pin.GetId());

			const MetaType* const type = arg.TryGetType();
			ASSERT(type != nullptr && "Should've been caught at compile time");

			const bool allocationSuccess = AllocateForCache(context, cachedValue, pin, type->GetTypeInfo());

			if (pin.GetTypeForm() == TypeForm::Value)
			{
				if (!allocationSuccess)
				{
					PrintError({ ScriptError::StackOverflow, { context.mFunc } });
					return "Stack overflow";
				}

				// Copy construct
				FuncResult copyResult = type->ConstructAt(cachedValue.mData, arg);

				// TODO can be checked at compile time?
				if (copyResult.HasError())
				{
					return { Format("Failed to copy construct type {} from pin {} on node {} - {} try passing by reference instead",
						type->GetName(),
						pin.GetName(),
						entryNode->GetDisplayName(),
						copyResult.Error()) };
				}
			}
			else
			{
				cachedValue.mData = arg.GetData();
			}

			++argNum;
			++pinIndex;
		}
	}

	Expected<const ScriptNode*, ScriptError> current = { &firstNode };
	uint32 stepNum = 0;

	// We can't execute the entry node, so just skip it and move on to the next one
	if (firstNode.GetType() == ScriptNodeType::FunctionEntry)
	{
		goto nextNode;
	}

	for (; stepNum < sMaxNumOfNodesToExecutePerFunctionBeforeGivingUp; stepNum++)
	{
		// We have reached the end of the function and must return the value
		if (current.GetValue()->GetType() == ScriptNodeType::FunctionReturn)
		{
			for (const ScriptPin& pinToOutputTo : current.GetValue()->GetInputs(func))
			{
				if (pinToOutputTo.IsFlow())
				{
					continue;
				}

				Expected<MetaAny, ScriptError> ret = GetValueToPassAsInput(context, pinToOutputTo);

				if (ret.HasError())
				{
					PrintError(ret.GetError());
					return ret.GetError().ToString(true);
				}

				if (pinToOutputTo.GetTypeForm() == TypeForm::Value
					&& !ret.GetValue().IsOwner())
				{
					const MetaType* const returnType = ret.GetValue().TryGetType();
					ASSERT(returnType != nullptr && "Shouldve been checked during script compilation");

					FuncResult copyConstructResult = rvoBuffer == nullptr ? returnType->Construct(ret.GetValue()) : returnType->ConstructAt(rvoBuffer, ret.GetValue());

					if (copyConstructResult.HasError()) // TODO can be checked during script compilation
					{
						std::string error = Format("Could not copy return value of type {} - {}", returnType->GetName(), copyConstructResult.Error());
						PrintError({ ScriptError::FunctionCallFailed, { context.mFunc, pinToOutputTo }, error });
						return error;
					}
					return std::move(copyConstructResult.GetReturnValue());
				}

				return std::move(ret.GetValue());

			}

			break;
		}

		{
			const Expected<VMContext::CachedValue*, ScriptError> result = ExecuteNode(context, *current.GetValue());

			if (result.HasError())
			{
				PrintError(result.GetError());
				return result.GetError().ToString(true);
			}
		}


	nextNode:
		current = GetNextNodeToExecute(context, *current.GetValue());

		if (current.HasError())
		{
			PrintError(current.GetError());
			return current.GetError().ToString(true);
		}

		// We have reached the end of the function
		if (current.GetValue() == nullptr)
		{
			break;
		}
	}

	if (stepNum == sMaxNumOfNodesToExecutePerFunctionBeforeGivingUp)
	{
		ScriptError error = { ScriptError::ExecutionTimeOut, { context.mFunc, *current.GetValue() } };
		PrintError(error);
		return error.ToString(true);
	}

	if (!func.GetReturnType().has_value())
	{
		return { std::nullopt };
	}

	// No return node encountered, returning default constructed value
	const MetaType* const returnType = func.GetReturnType()->TryGetType();

	ASSERT(returnType != nullptr && "Should've been caught during script compilation");

	FuncResult defaultConstructResult = rvoBuffer == nullptr ? returnType->Construct() : returnType->ConstructAt(rvoBuffer);

	// TODO can be compile-time error
	if (defaultConstructResult.HasError())
	{
		ScriptError error{ ScriptError::Type::ValueWasNull, { context.mFunc, *current.GetValue() },
			Format("No return node encountered, and the return type {} cannot be default constructed - {}",
			returnType->GetName(),
			defaultConstructResult.Error())
		};
		PrintError(error);
		return error.ToString(true);
	}

	return { std::move(defaultConstructResult.GetReturnValue()) };
}

void Engine::VirtualMachine::PrintCompileErrors() const
{
	for (const ScriptError& error : mErrorsFromLastCompilation)
	{
		PrintError(error, true);
	}
}

void Engine::VirtualMachine::PrintError(const ScriptError& error, bool compileError)
{
	Logger::Get().Log(error.ToString(true), compileError ? "ScriptCompileError" : "ScriptRuntimeError", Error, SourceLocation::current(__LINE__, __FILE__),
#ifdef EDITOR
		[loc = error.GetOrigin()]
		{
			loc.NavigateToLocation();
		});
#else
	{});
#endif // EDITOR
}

void Engine::VirtualMachine::DestroyAllTypesCreatedThroughScripts()
{
	LOG(LogScripting, Verbose, "Destroying all types created through scripts");

	MetaManager& manager = MetaManager::Get();

	std::vector<TypeId> typesToRemove{};

	for (MetaType& type : manager.EachType())
	{
		if (WasTypeCreatedByScript(type))
		{
			LOG_FMT(LogScripting, Verbose, "Type {} will be destroyed", type.GetName());
			UnreflectComponentType(type);
			typesToRemove.push_back(type.GetTypeId());
		}
	}

	for (const TypeId typeId : typesToRemove)
	{
		[[maybe_unused]] const bool success = manager.RemoveType(typeId);
		ASSERT(success);
	}
}

Engine::VirtualMachine::VMContext::VMContext(const ScriptFunc& func) :
	mFunc(func)
{
	VirtualMachine& vm = VirtualMachine::Get();

	mStackPtrToFallBackTo = vm.mStackPtr;
	uint32 size = static_cast<uint32>(func.GetNumOfPinsIncludingRemoved() * sizeof(CachedValue));
	mCachedValues = static_cast<CachedValue*>(vm.StackAllocate(size, 8));

	if (mCachedValues != nullptr)
	{
		memset(mCachedValues, 0, size);
	}
}

Engine::VirtualMachine::VMContext::~VMContext()
{
	VirtualMachine& vm = VirtualMachine::Get();

	for (uint32 i = 0; i < static_cast<uint32>(mFunc.GetNumOfPinsIncludingRemoved()); i++)
	{
		CachedValue& cachedValue = mCachedValues[i];

		// The userBit signifies ownership
		if (!vm.DoesCacheValueNeedToBeFreed(cachedValue))
		{
			continue;
		}

		const ScriptPin& pin = mFunc.GetPin(PinId{ i + 1 });
		const MetaType& metaType = *pin.TryGetType();

		vm.FreeCachedValue(cachedValue, metaType);
	}

	vm.mStackPtr = mStackPtrToFallBackTo;
}

void* Engine::VirtualMachine::StackAllocate(uint32 numOfBytes, uint32 alignment)
{
	size_t sizeLeft = reinterpret_cast<uintptr>(&mStack.back()) - reinterpret_cast<uintptr>(mStackPtr) + 1;

	void* stackPtrAsVoid = mStackPtr;
	void* allocatedAt = std::align(alignment, numOfBytes, stackPtrAsVoid, sizeLeft);

	if (allocatedAt != nullptr)
	{
		mStackPtr = static_cast<char*>(allocatedAt) + numOfBytes;
	}
	return allocatedAt;
}

Expected<const Engine::ScriptNode*, Engine::ScriptError> Engine::VirtualMachine::GetNextNodeToExecute(VMContext& context,
	const ScriptNode& start)
{
	ASSERT(!start.IsPure(context.mFunc)
		&& !start.GetOutputs(context.mFunc).empty()
		&& start.GetOutputs(context.mFunc)[0].IsFlow());

	uint32 indexOfOutputFlowPin = 0;
	const ScriptNode* current = &start;

	for (size_t i = 0; i < sMaxNumOfNodesToExecutePerFunctionBeforeGivingUp; i++)
	{
		uint32 indexOfInputFlowPin{};

		{
			ASSERT(indexOfOutputFlowPin < current->GetOutputs(context.mFunc).size());

			const ScriptPin& outputFlowPin = current->GetOutputs(context.mFunc)[indexOfOutputFlowPin];

			const PinId linkedToId = outputFlowPin.GetCachedPinWeAreLinkedWith();

			if (linkedToId == PinId::Invalid) // Not linked to anything
			{
				if (context.mControlNodesToFallBackTo.empty())
				{
					return nullptr;
				}

				std::variant<VMContext::WhileLoopInfo, VMContext::ForLoopInfo> loopInfo = context.mControlNodesToFallBackTo.back();

				if (std::holds_alternative<VMContext::WhileLoopInfo>(loopInfo))
				{
					current = &std::get<VMContext::WhileLoopInfo>(loopInfo).mNode.get();
				}
				else
				{
					ASSERT(std::holds_alternative<VMContext::ForLoopInfo>(loopInfo));
					current = &std::get<VMContext::ForLoopInfo>(loopInfo).mNode.get();
				}

				// We didn't enter through any pins, we just went teleported back to the loop node
				indexOfInputFlowPin = std::numeric_limits<uint32>::max();
			}
			else
			{
				current = nullptr;

				const ScriptPin& linkedTo = context.mFunc.GetPin(linkedToId);
				current = &context.mFunc.GetNode(linkedTo.GetNodeId());

				indexOfInputFlowPin = linkedToId.Get() - current->GetIdOfFirstPin().Get();

				ASSERT(&current->GetInputs(context.mFunc)[indexOfInputFlowPin] == &linkedTo);
			}
		}

		ASSERT(current != nullptr);

		switch (current->GetType())
		{
		case ScriptNodeType::ForLoop:
		{
			Span<const ScriptPin> inputs = current->GetInputs(context.mFunc);
			Span<const ScriptPin> outputs = current->GetOutputs(context.mFunc);

			//ASSERT(inputs.size() == 4
			//	&& inputs[ForLoopScriptNode::sIndexOfEntryPin].IsFlow() // Entry
			//	&& inputs[ForLoopScriptNode::sIndexOfStartPin].GetTypeTraits() == MakeTypeTraits<int32>() // Start
			//	&& inputs[ForLoopScriptNode::sIndexOfEndPin].GetTypeTraits() == MakeTypeTraits<const int32&>() // End
			//	&& inputs[ForLoopScriptNode::sIndexOfBreakPin].IsFlow() // Break
			//	&& outputs.size() == 3
			//	&& outputs[ForLoopScriptNode::sIndexOfLoopPin].IsFlow() // Loop body
			//	&& outputs[ForLoopScriptNode::sIndexOfIndexPin].GetTypeTraits() == MakeTypeTraits<int32>() // Current index
			//	&& outputs[ForLoopScriptNode::sIndexOfExitPin].IsFlow()); // Exit

			const ScriptPin& indexPin = outputs[ForLoopScriptNode::sIndexOfIndexPin];
			VMContext::CachedValue& cachedIndex = FindCachedValue(context, indexPin.GetId());

			if (indexOfInputFlowPin == ForLoopScriptNode::sIndexOfEntryPin) // for (int i = startValue;
			{
				const ScriptPin& startPin = inputs[ForLoopScriptNode::sIndexOfStartPin];
				Expected<MetaAny, ScriptError> startValue = GetValueToPassAsInput(context, startPin);

				if (startValue.HasError())
				{
					UNLIKELY;
					return std::move(startValue.GetError());
				}

				BeginLoop(context, VMContext::ForLoopInfo{ static_cast<const ForLoopScriptNode&>(*current) });

				const bool success = AllocateForCache(context, cachedIndex, indexPin, MakeTypeInfo<int32>());

				if (!success)
				{
					UNLIKELY;
					return ScriptError{ ScriptError::StackOverflow, { context.mFunc, indexPin } };
				}

				*static_cast<int32*>(cachedIndex.mData) = 0;
			}

			int32& index = *static_cast<int32*>(cachedIndex.mData);

			if (indexOfInputFlowPin != ForLoopScriptNode::sIndexOfEntryPin // Don't increment if we just started the for loop
				&& indexOfInputFlowPin != ForLoopScriptNode::sIndexOfBreakPin) // Don't increment if we hit the break pin
			{
				// We're updating a value in the cache, any nodes that are linked to the index will be recalculated as needed
				++context.mNumOfImpureNodesExecutedAtTimeOfCaching;
				++index;
				cachedIndex.mNumOfImpureNodesExecutedAtTimeOfCaching = context.mNumOfImpureNodesExecutedAtTimeOfCaching;
			}

			const ScriptPin& endPin = inputs[ForLoopScriptNode::sIndexOfEndPin];
			Expected<int32, ScriptError> end = GetValueToPassAsInput<int32>(context, endPin);

			if (end.HasError())
			{
				return std::move(end.GetError());
			}

			if (index >= end.GetValue()// If we've finished iterating
				|| indexOfInputFlowPin == ForLoopScriptNode::sIndexOfBreakPin) // If break pin hit 
			{
				EndLoop(context, *current);
				indexOfOutputFlowPin = ForLoopScriptNode::sIndexOfExitPin; // Exit loop
			}
			else
			{
				indexOfOutputFlowPin = ForLoopScriptNode::sIndexOfLoopPin; // Continue loop body
			}

			break;
		}
		case ScriptNodeType::WhileLoop:
		{
			Span<const ScriptPin> inputs = current->GetInputs(context.mFunc);

			//ASSERT(inputs.size() == 3
			//	&& inputs[WhileLoopScriptNode::sIndexOfEntryPin].IsFlow() // Entry
			//	&& inputs[WhileLoopScriptNode::sIndexOfConditionPin].GetTypeTraits() == MakeTypeTraits<const bool&>() // Condition
			//	&& inputs[WhileLoopScriptNode::sIndexOfBreakPin].IsFlow() // Break
			//	&& current->GetOutputs(context.mFunc).size() == 2
			//	&& current->GetOutputs(context.mFunc)[WhileLoopScriptNode::sIndexOfLoopPin].IsFlow() // Loop body
			//	&& current->GetOutputs(context.mFunc)[WhileLoopScriptNode::sIndexOfExitPin].IsFlow()); // Exit

			const ScriptPin& conditionPin = inputs[WhileLoopScriptNode::sIndexOfConditionPin];

			Expected<bool, ScriptError> condition = GetValueToPassAsInput<bool>(context, conditionPin);

			if (condition.HasError())
			{
				return std::move(condition.GetError());
			}

			if (indexOfInputFlowPin == WhileLoopScriptNode::sIndexOfEntryPin)
			{
				BeginLoop(context, VMContext::WhileLoopInfo{ static_cast<const WhileLoopScriptNode&>(*current) });
			}

			if (indexOfInputFlowPin == WhileLoopScriptNode::sIndexOfBreakPin // If break pin hit
				|| !condition.GetValue()) // Or if the condition is false
			{
				EndLoop(context, *current);
				indexOfOutputFlowPin = WhileLoopScriptNode::sIndexOfExitPin; // Exit loop
			}
			else
			{
				indexOfOutputFlowPin = WhileLoopScriptNode::sIndexOfLoopPin; // Continue to loop body
			}

			break;
		}
		case ScriptNodeType::Branch:
		{
			Span<const ScriptPin> inputs = current->GetInputs(context.mFunc);

			//ASSERT(inputs.size() == 2
			//	&& inputs[BranchScriptNode::sIndexOfEntryPin].IsFlow()
			//	&& inputs[BranchScriptNode::sIndexOfConditionPin].GetTypeTraits() == MakeTypeTraits<const bool&>()
			//	&& current->GetOutputs(context.mFunc).size() == 2
			//	&& current->GetOutputs(context.mFunc)[BranchScriptNode::sIndexOfIfPin].IsFlow()
			//	&& current->GetOutputs(context.mFunc)[BranchScriptNode::sIndexOfElsePin].IsFlow());

			const ScriptPin& conditionPin = inputs[BranchScriptNode::sIndexOfConditionPin];

			Expected<bool, ScriptError> condition = GetValueToPassAsInput<bool>(context, conditionPin);

			if (condition.HasError())
			{
				return std::move(condition.GetError());
			}

			if (condition.GetValue())
			{
				indexOfOutputFlowPin = BranchScriptNode::sIndexOfIfPin;
			}
			else
			{
				indexOfOutputFlowPin = BranchScriptNode::sIndexOfElsePin;
			}

			break;
		}
		case ScriptNodeType::Rerout:
		{
			indexOfOutputFlowPin = 0;
			break;
		}
		default:
		{
			return current;
		}
		}
	}

	return ScriptError{ ScriptError::ExecutionTimeOut, { context.mFunc, start } };
}

Expected<Engine::VirtualMachine::VMContext::CachedValue*, Engine::ScriptError> Engine::VirtualMachine::ExecuteNode(VMContext& context, const ScriptNode& node)
{
	Span<const ScriptPin> inputs = node.GetInputs(context.mFunc);
	const size_t numOfInputPins = inputs.size();

	// Raii object that manages the lifetime
	struct InputDeleter
	{
		~InputDeleter()
		{
			for (size_t i = 0; i < mSize; i++)
			{
				mInputForms[i].~MetaAny();
			}
		}

		MetaAny* mInputForms;
		size_t mSize{};
	};

	// Stack allocation
	MetaAny* inputValues = static_cast<MetaAny*>(ENGINE_ALLOCA(numOfInputPins * sizeof(MetaAny)));
	TypeForm* inputForms = static_cast<TypeForm*>(ENGINE_ALLOCA(numOfInputPins * sizeof(TypeForm)));

	InputDeleter inputDeleter{ inputValues };
	ASSERT(inputValues != nullptr && inputForms != nullptr);
	// Collect the inputs that this node needs

	for (size_t i = 0; i < numOfInputPins; i++)
	{
		const ScriptPin& param = inputs[i];

		if (param.IsFlow())
		{
			continue;
		}

		Expected<MetaAny, ScriptError> valueToPassToPureNode = GetValueToPassAsInput(context, param);

		if (valueToPassToPureNode.HasError())
		{
			return std::move(valueToPassToPureNode.GetError());
		}

		new (&inputValues[inputDeleter.mSize])MetaAny(std::move(valueToPassToPureNode.GetValue()));
		inputForms[inputDeleter.mSize] = param.GetTypeForm();
		inputDeleter.mSize++;
	}

	const Span<const ScriptPin> nodeOutputs = node.GetOutputs(context.mFunc);
	ASSERT(nodeOutputs.size() >= 1);

	// If the node returns void, all of these are nullptr.
	const ScriptPin* outputPin = &nodeOutputs[0];
	VMContext::CachedValue* returnAddress{};
	const MetaType* returnType{};

	// If the node is impure
	if (outputPin->IsFlow())
	{
		++context.mNumOfImpureNodesExecutedAtTimeOfCaching;

		if (nodeOutputs.size() == 1)
		{
			// This is an impure node with no return values
			outputPin = nullptr;
		}
		else
		{
			ASSERT(nodeOutputs.size() == 2);
			outputPin++;

			ASSERT(!outputPin->IsFlow());
			ASSERT(outputPin->TryGetType() != nullptr);
		}
	}

	if (outputPin != nullptr)
	{
		returnAddress = &FindCachedValue(context, outputPin->GetId());
		returnType = outputPin->TryGetType();
		ASSERT(returnType != nullptr);

		const bool allocationSuccess = AllocateForCache(context, *returnAddress, *outputPin, returnType->GetTypeInfo());

		if (!allocationSuccess)
		{
			return ScriptError{ ScriptError::StackOverflow, { context.mFunc, *outputPin } };
		}
	}

	FuncResult result{};

	switch (node.GetType())
	{
	default:
	case ScriptNodeType::Comment: // How would we even execute a comment node
	case ScriptNodeType::FunctionReturn: // This node can never be executed
	case ScriptNodeType::FunctionEntry: // This node can never be executed
	case ScriptNodeType::Branch: // This node can never be executed
	{
		ASSERT(false);
		break;
	}
	case ScriptNodeType::Setter:
	case ScriptNodeType::Getter:
	{
		ASSERT(inputDeleter.mSize != 0 && "Getting or setting a field always require a target");
		ASSERT(returnAddress != nullptr && "Does not return void; memory should have been allocated");

		const MetaField* const metaMember = static_cast<const NodeInvolvingMetaMember&>(node).TryGetOriginalMemberData();
		ASSERT(metaMember != nullptr && "Should've been caught during script-compilation");

		ASSERT(&metaMember->GetType() == returnType);

		MetaAny& refToTarget = inputValues[0];
		ASSERT(metaMember->GetOuterType().IsBaseClassOf(refToTarget.GetTypeId()) && "Invalid link, should've been caught during script compilation");

		MetaAny refToMemberInsideTarget = metaMember->MakeRef(refToTarget);

		if (node.GetType() == ScriptNodeType::Setter)
		{
			ASSERT(inputDeleter.mSize == 2 && "setting a field always require two arguments");

			MetaAny& valueToSetItTo = inputValues[1];

			const MetaType* const typeOfArg = valueToSetItTo.TryGetType();
			ASSERT(typeOfArg != nullptr && "Should've been caught during script-compilation");

			// TODO Check if it has the copy assign operator at script-compile time
			// We can't use this setResult, because the assignment operator returns a reference,
			// and we want to cache this value. If the object gets destroyed, the reference
			// will be dangling. We make a copy further along the line, and cache that instead.
			FuncResult setResult = typeOfArg->CallFunction(OperatorType::assign, refToMemberInsideTarget, valueToSetItTo);

			if (setResult.HasError())
			{
				return ScriptError{ ScriptError::FunctionCallFailed, { context.mFunc, node }, setResult.Error() };
			}
		}

		// Make a copy of the value when getting or setting
		// TODO Check if it has the copy-constructor at script-compile time
		result = returnType->ConstructAt(returnAddress->mData, refToMemberInsideTarget);
		break;
	}
	case ScriptNodeType::FunctionCall:
	{
		const MetaFuncScriptNode& metaFuncNode = static_cast<const MetaFuncScriptNode&>(node);

		const MetaFunc* const originalFunc = metaFuncNode.TryGetOriginalFunc();
		ASSERT(originalFunc != nullptr && "Should've been caught during script-compilation");

		// We assume the types match, because that was checked during script-compilation
		// so we only have to check if something was null

		const std::vector<MetaFuncNamedParam>& params = originalFunc->GetParameters();

		for (uint32 i = 0; i < inputDeleter.mSize; i++)
		{
			if (inputValues[i] == nullptr
				&& !CanFormBeNullable(params[i].mTypeTraits.mForm))
			{
				UNLIKELY;
				return ScriptError{ ScriptError::ValueWasNull, { context.mFunc, node } };
			}
		}

		result = originalFunc->InvokeUnchecked({ inputValues, inputDeleter.mSize }, { inputForms, inputDeleter.mSize }, returnAddress == nullptr ? nullptr : returnAddress->mData);
		break;
	}
	}

	if (result.HasError())
	{
		return ScriptError{ ScriptError::FunctionCallFailed, { context.mFunc, node }, result.Error() };
	}

	ASSERT(returnAddress == nullptr == !result.HasReturnValue() && "No memory was allocated, but the result holds a return value, or Memory was allocated, but the result holds no return value");

	if (returnAddress != nullptr)
	{
		// Some functions may return a reference or pointer.
		returnAddress->mData = result.GetReturnValue().GetData();
		return returnAddress;
	}

	return { nullptr };
}

Engine::VirtualMachine::VMContext::WhileLoopInfo& Engine::VirtualMachine::BeginLoop(VMContext& context, VMContext::WhileLoopInfo&& loopInfo)
{
	return std::get<VMContext::WhileLoopInfo>(context.mControlNodesToFallBackTo.emplace_back(std::move(loopInfo)));
}

Engine::VirtualMachine::VMContext::ForLoopInfo& Engine::VirtualMachine::BeginLoop(VMContext& context, VMContext::ForLoopInfo&& loopInfo)
{
	return std::get<VMContext::ForLoopInfo>(context.mControlNodesToFallBackTo.emplace_back(std::move(loopInfo)));
}

void Engine::VirtualMachine::EndLoop(VMContext& context, const ScriptNode& node)
{
	auto nodeToFallBackTo = std::find_if(context.mControlNodesToFallBackTo.crbegin(), context.mControlNodesToFallBackTo.crend(),
		[&node](const std::variant<VMContext::WhileLoopInfo, VMContext::ForLoopInfo>& loopInfo)
		{
			if (std::holds_alternative<VMContext::WhileLoopInfo>(loopInfo))
			{
				return &node == &std::get<VMContext::WhileLoopInfo>(loopInfo).mNode.get();
			}
			ASSERT(std::holds_alternative<VMContext::ForLoopInfo>(loopInfo));
			return &node == &std::get<VMContext::ForLoopInfo>(loopInfo).mNode.get();
		});

	if (nodeToFallBackTo == context.mControlNodesToFallBackTo.crend())
	{
		PrintError(ScriptError{ ScriptError::LinkNotAllowed, { context.mFunc, node }, "Attempted to break out of a loop that was not running", });
	}
	else
	{
		++nodeToFallBackTo;
		context.mControlNodesToFallBackTo.erase(nodeToFallBackTo.base(), context.mControlNodesToFallBackTo.end());
	}
}

Engine::VirtualMachine::VMContext::CachedValue& Engine::VirtualMachine::FindCachedValue(VMContext& context, PinId pinId)
{
	return context.mCachedValues[pinId.Get() - 1];
}

template<>
Expected<Engine::MetaAny, Engine::ScriptError> Engine::VirtualMachine::GetValueToPassAsInput(VMContext& context, const ScriptPin& pinToPassInputInto)
{
	ASSERT(pinToPassInputInto.IsInput());

	// Get the output pin this pin is linked to
	const PinId otherPinId = pinToPassInputInto.GetCachedPinWeAreLinkedWith();

	std::optional<MetaAny> returnValue{};
	const TypeForm form = pinToPassInputInto.GetTypeForm();

	// No output pin? No worries, we'll just use the default value!
	if (otherPinId == PinId::Invalid)
	{
		// There is no link connected to this pin, maybe we can default to 'this'?
		if (pinToPassInputInto.GetTypeName() == context.mFunc.GetNameOfScriptAsset())
		{
			if (context.mFunc.IsStatic())
			{
				UNLIKELY;
				return ScriptError{ ScriptError::ValueWasNull, { context.mFunc, pinToPassInputInto },
					Format("Cannot call non-static functions ({}) from static functions ({})",
						context.mFunc.GetNode(pinToPassInputInto.GetNodeId()).GetDisplayName(),
						context.mFunc.GetName())
				};
			}

			// An instance of the script should have been provided as an argument to this function
			if (context.mThis == nullptr)
			{
				UNLIKELY;
				return ScriptError{ ScriptError::CompilerBug, { context.mFunc, pinToPassInputInto },
					"Compiler error: 'This' was unexpectedly nullptr for a non-static function. This should have been checked earlier." };
			}

			returnValue = MakeRef(*context.mThis);
		}
		else if (!DoesPinRequireLink(context.mFunc, pinToPassInputInto))
		{
			const MetaAny* valueIfNoInputLinked = pinToPassInputInto.TryGetValueIfNoInputLinked();

			if (valueIfNoInputLinked != nullptr)
			{
				ASSERT(valueIfNoInputLinked->GetData() != nullptr && "Not sure how this could have happened");
				returnValue = MakeRef(const_cast<MetaAny&>(*valueIfNoInputLinked));
			}
			else
			{
				const MetaType* const pinType = pinToPassInputInto.TryGetType();
				ASSERT(pinType != nullptr && "Shoulve been caught during script compilation");

				// Construct default
				FuncResult defaultConstructedValue = pinType->Construct();

				// TODO can be checked during script-compilation
				if (defaultConstructedValue.HasError())
				{
					UNLIKELY;
					return ScriptError{ ScriptError::FunctionCallFailed, { context.mFunc, pinToPassInputInto },
						Format("Type {} of input pin {} has no link connected to it, and creating a default value failed - {}",
						pinType->GetName(),
						pinToPassInputInto.GetName(),
						defaultConstructedValue.Error())
					};
				}

				returnValue = std::move(defaultConstructedValue.GetReturnValue());
			}
		}
		else
		{
			UNLIKELY;
			// TODO can be checked during script-compilation
			return ScriptError{ ScriptError::ValueWasNull, { context.mFunc, pinToPassInputInto }, "No link was connected to pin" };
		}
	}
	else
	{
		const ScriptPin& otherPin = context.mFunc.GetPin(otherPinId);
		const ScriptNode& otherNode = context.mFunc.GetNode(otherPin.GetNodeId());

		if (otherNode.GetType() == ScriptNodeType::Rerout)
		{
			return GetValueToPassAsInput(context, otherNode.GetInputs(context.mFunc)[0]);
		}

		// If the pin type is MetaAny, we accept anything. We cannot assign that to returnValue, 
		// because typeId<MetaAny> != typeIdOfWhateverIsConnectedToOurPin
		// A bit of an exception to the rule that makes this part of the code a bit messy
		// At the time of writing, this is only used for the IsNull node
		bool returnImmediately{};

		if (pinToPassInputInto.TryGetType()->GetTypeId() == MakeTypeId<MetaAny>())
		{
			// TODO can be checked during script compilation
			if (pinToPassInputInto.GetTypeForm() == TypeForm::Value)
			{
				return ScriptError{ ScriptError::TypeCannotBeOwnedByScripts, { context.mFunc, pinToPassInputInto }, "Pins accepting 'MetaAny' by value is not allowed; use a reference or a const reference instead" };
			}
			returnImmediately = true;
		}

		VMContext::CachedValue& cachedValue = FindCachedValue(context, otherPin.GetId());

		if (cachedValue.mNumOfImpureNodesExecutedAtTimeOfCaching == context.mNumOfImpureNodesExecutedAtTimeOfCaching // Did we run any impure nodes that could have changed the cached value
			|| !otherNode.IsPure(context.mFunc)) // Impure nodes are not executed again if the cache is 'out of date', this is perfectly valid and expected behaviour
		{
			// Yay! We can use the cached value
			if (returnImmediately)
			{
				return MetaAny{ *otherPin.TryGetType(), cachedValue.mData, false };
			}
			returnValue = MetaAny{ *otherPin.TryGetType(), cachedValue.mData, false };
		}
		else
		{
			// The cache is out of date. We have to recalculate it

			// We can't suddenly execute a non-pure node
			if (!otherNode.IsPure(context.mFunc))
			{
				UNLIKELY;
				return ScriptError{ ScriptError::LinkNotAllowed, { context.mFunc, context.mFunc.GetAllLinksConnectedToPin(pinToPassInputInto.GetId())[0] },
					Format("The input pin {} in node {} is connected to the output pin {} in node {}, but node {} runs before node {}!",
					pinToPassInputInto.GetName(),
					context.mFunc.GetNode(pinToPassInputInto.GetNodeId()).GetDisplayName(),
					otherNode.GetDisplayName(),
					otherNode.GetDisplayName(),
					context.mFunc.GetNode(pinToPassInputInto.GetNodeId()).GetDisplayName(),
					otherNode.GetDisplayName())
				};
			}

			// Run the node
			Expected<VMContext::CachedValue*, ScriptError> nodeResult = ExecuteNode(context, otherNode);

			if (nodeResult.HasError())
			{
				return std::move(nodeResult.GetError());
			}

			VMContext::CachedValue* nodeReturnValue = nodeResult.GetValue();

			if (nodeReturnValue == nullptr)
			{
				return ScriptError{ ScriptError::CompilerBug, { context.mFunc, pinToPassInputInto },
					Format("The node {} unexpectedly returned void", otherNode.GetDisplayName()) };
			}

			
			const TypeInfo returnTypeInfo = otherPin.TryGetType()->GetTypeInfo();

			if (returnImmediately)
			{
				return MetaAny{ returnTypeInfo, nodeReturnValue->mData };
			}

			returnValue = MetaAny{ returnTypeInfo, nodeReturnValue->mData };
		}
	}

	ASSERT(returnValue.has_value());


	// Check if it's nullptr
	if (*returnValue == nullptr
		&& !CanFormBeNullable(form))
	{
		return ScriptError{ ScriptError::ValueWasNull, { context.mFunc, pinToPassInputInto } };
	}

	return std::move(*returnValue);
}

bool Engine::VirtualMachine::AllocateForCache(VMContext& context, VMContext::CachedValue& cachedValue, const ScriptPin& pin, TypeInfo typeInfo)
{
	cachedValue.mNumOfImpureNodesExecutedAtTimeOfCaching = context.mNumOfImpureNodesExecutedAtTimeOfCaching;

	cachedValue.mTypeInfoFlags = typeInfo.mFlags;

	if (pin.GetTypeForm() != TypeForm::RValue
		&& pin.GetTypeForm() != TypeForm::Value)
	{
		cachedValue.mTypeInfoFlags &= ~TypeInfo::UserBit;
		return true;
	}

	// Reuse the existing space if we can
	if (cachedValue.mData != nullptr)
	{
		if (DoesCacheValueNeedToBeFreed(cachedValue))
		{
			FreeCachedValue(cachedValue, *pin.TryGetType());
		}

		return true;
	}

	cachedValue.mData = StackAllocate(typeInfo.GetSize(), typeInfo.GetAlign());
	cachedValue.mTypeInfoFlags |= TypeInfo::UserBit;

	if (cachedValue.mData == nullptr)
	{
		UNLIKELY;
		return false;
	}

	return true;
}

bool Engine::VirtualMachine::DoesCacheValueNeedToBeFreed(VMContext::CachedValue& cachedValue)
{
	return (cachedValue.mTypeInfoFlags & TypeInfo::UserBit) && (cachedValue.mTypeInfoFlags & TypeInfo::IsTriviallyDestructible) == 0 && cachedValue.mData != nullptr;
}

void Engine::VirtualMachine::FreeCachedValue(VMContext::CachedValue& cachedValue, const MetaType& pinType)
{
	ASSERT(DoesCacheValueNeedToBeFreed(cachedValue));
	pinType.Destruct(cachedValue.mData, false);
}

template<typename T>
Expected<T, Engine::ScriptError> Engine::VirtualMachine::GetValueToPassAsInput(VMContext& context, const ScriptPin& pinToPassInputInto)
{
	ASSERT(pinToPassInputInto.IsInput() && pinToPassInputInto.TryGetType()->GetTypeId() == MakeTypeId<T>());

	Expected<MetaAny, ScriptError> valueToPassAsInput = GetValueToPassAsInput(context, pinToPassInputInto);

	if (valueToPassAsInput.HasError())
	{
		return std::move(valueToPassAsInput.GetError());
	}

	ASSERT(valueToPassAsInput.GetValue().IsExactly<T>() && "Shoulve been caught at script-compilation");

	const T* const conditionValue = static_cast<T*>(valueToPassAsInput.GetValue().GetData());

	if (conditionValue == nullptr)
	{
		return ScriptError{ ScriptError::ValueWasNull, { context.mFunc, pinToPassInputInto } };
	}

	return *conditionValue;
}

