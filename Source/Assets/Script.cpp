#include "Precomp.h"
#include "Assets/Script.h"

#include "Meta/MetaType.h"
#include "Meta/MetaProps.h"
#include "Meta/MetaManager.h"
#include "Scripting/Nodes/EntryAndReturnScriptNode.h"
#include "Scripting/ScriptErrors.h"
#include "GSON/GSONBinary.h"
#include "Assets/Core/AssetLoadInfo.h"
#include "Assets/Core/AssetSaveInfo.h"
#include "Meta/MetaTools.h"
#include "Utilities/ClassVersion.h"
#include "Utilities/Reflect/ReflectAssetType.h"
#include "Utilities/Reflect/ReflectComponentType.h"

Engine::Script::Script(std::string_view name) :
	Asset(name, MakeTypeId<Script>())
{
	
}

Engine::Script::Script(AssetLoadInfo& loadInfo) :
	Asset(loadInfo)
{
	BinaryGSONObject obj{};
	const bool success = obj.LoadFromBinary(loadInfo.GetStream());

	if (!success)
	{
		LOG(LogAssets, Message, "Loading of script {} failed, possible because this is a new script. Adding an OnTick function...", GetName());
		AddFunc("OnTick").SetParameters({ {  MakeTypeTraits<float>(), "DeltaTime" } });
		return;
	}

	const BinaryGSONObject* functions = obj.TryGetGSONObject("functions");

	if (functions != nullptr)
	{
		for (const BinaryGSONObject& serializedFunc : functions->GetChildren())
		{
			std::optional<ScriptFunc> func = ScriptFunc::DeserializeFrom(serializedFunc, *this, loadInfo.GetVersion());

			if (func.has_value())
			{
				mFunctions.emplace_back(std::move(*func));
			}
		}
	}
	else
	{
		UNLIKELY;
		LOG_TRIVIAL(LogScripting, Warning, "No functions object serialized");
	}

	const BinaryGSONObject* members = obj.TryGetGSONObject("members");

	if (members != nullptr)
	{
		for (const BinaryGSONObject& serializedMember : members->GetChildren())
		{
			std::optional<ScriptField> field = ScriptField::DeserializeFrom(serializedMember, *this, loadInfo.GetVersion());

			if (field.has_value())
			{
				mFields.emplace_back(std::move(*field));
			}
		}
	}
	else
	{
		UNLIKELY;
		LOG_TRIVIAL(LogScripting, Warning, "No members object serialized");
	}
}

Engine::ScriptFunc& Engine::Script::AddFunc(const std::string_view name)
{
	ScriptFunc* existingFunc = TryGetFunc(name);

	if (existingFunc != nullptr)
	{
		LOG(LogScripting, Error, "There is already a function with the name {} in {}. Returning existing function",
			name,
			GetName());
		return *existingFunc;
	}

	auto& result = mFunctions.emplace_back(*this, name);
	result.AddNode<FunctionEntryScriptNode>(result, *this);

	return result;
}

void Engine::Script::RemoveFunc(Name name)
{
	const auto it = GetByName(mFunctions, name);

	if (it == mFunctions.end())
	{
		LOG(LogScripting, Warning, "Attempted to remove function {} in {}, but theres no function with this name",
			name.StringView(),
			GetName());
	}
	else
	{
		mFunctions.erase(it);
	}
}

Engine::ScriptFunc* Engine::Script::TryGetFunc(Name name)
{
	const auto it = GetByName(mFunctions, name);
	return it == mFunctions.end() ? nullptr : &*it;
}

const Engine::ScriptFunc* Engine::Script::TryGetFunc(Name name) const
{
	return const_cast<Script*>(this)->TryGetFunc(name);
}

Engine::ScriptField& Engine::Script::AddField(const std::string_view name)
{
	ScriptField* existingMember = TryGetField(name);

	if (existingMember != nullptr)
	{
		LOG(LogScripting, Error, "There is already a field with the name {} in {}. Returning existing function",
			name,
			GetName());
		return *existingMember;
	}

	return mFields.emplace_back(*this, name);
}

void Engine::Script::RemoveField(const Name name)
{
	const auto it = GetByName(mFields, name);

	if (it == mFields.end())
	{
		LOG(LogScripting, Warning, "Attempted to remove field {} in {}, but theres no field with this name",
			name.StringView(),
			GetName());
	}
	else
	{
		mFields.erase(it);
	}
}

Engine::ScriptField* Engine::Script::TryGetField(const Name name)
{
	const auto it = GetByName(mFields, name);
	return it == mFields.end() ? nullptr : &*it;
}

const Engine::ScriptField* Engine::Script::TryGetField(const Name name) const
{
	return const_cast<Script*>(this)->TryGetField(name);
}

Engine::MetaType* Engine::Script::DeclareMetaType()
{
	TypeInfo ourTypeInfo{
		/*.mTypeId = */ Name::HashString(GetName()),
		/*.mSize = */ 0,
		/*.mAlignment = */ 0,
		/*.mIsTriviallyDefaultConstructible = */ 1,
		/*.mIsTriviallyMoveConstructible = */ 1,
		/*.mIsTriviallyCopyConstructible = */ 1,
		/*.mIsTriviallyCopyAssignable = */ 1,
		/*.mIsTriviallyMoveAssignable = */ 1,
		/*.mIsDefaultConstructible = */ 1,
		/*.mIsMoveConstructible = */ 1,
		/*.mIsCopyConstructible = */ 1,
		/*.mIsCopyAssignable = */ 1,
		/*.mIsMoveAssignable = */ 1,
		/*.mIsTriviallyDestructible = */ 1,
		/*.mUserBit = */ 0
	};

	if (const MetaType* existingType = MetaManager::Get().TryGetType(ourTypeInfo.mTypeId);
		existingType != nullptr)
	{
		static_assert(sIsTypeIdTheSameAsNameHash);

		UNLIKELY;
		LOG(LogScripting, Error, "By pure chance, the hash of a script ({}) and an existing C++ class ({}) are the same! (the chance is 1 in 2^32). Rename the script or the C++ class",
			GetName(), existingType->GetName());

		return nullptr;
	}

	if (const MetaType* existingType = MetaManager::Get().TryGetType(GetName());
		existingType != nullptr)
	{
		UNLIKELY;
		LOG(LogScripting, Error, "There is already a C++ class with the name {}. Rename the script or the C++ class",
			GetName(), existingType->GetName());

		return nullptr;
	}

	const MetaType* const entityType = MetaManager::Get().TryGetType<entt::entity>();

	if (entityType == nullptr)
	{
		LOG(LogScripting, Warning, "Making a metatype from {} failed - Type entt::entity was not reflected", GetName());
		return nullptr;
	}

	LOG(LogScripting, Verbose, "Making type from script {}", GetName());

	struct MemberToAdd
	{
		MemberToAdd(const MetaType& type, const std::string& name) : mType(type), mName(name) {};
		std::reference_wrapper<const MetaType> mType;
		std::string mName{};
		uint32 mOffset{};
	};

	std::vector<MemberToAdd> membersToAdd{ { *entityType, sNameOfOwnerField.String() } };

	for (const ScriptField& field : mFields)
	{
		const MetaType* const fieldType = field.TryGetType();

		if (fieldType == nullptr)
		{
			LOG(LogScripting, Error, "Could not add field {} to {} - The field type was not reflected",
				field.GetName(), GetName());
			continue;
		}

		if (!ScriptField::CanTypeBeUsedForFields(*fieldType))
		{
			LOG(LogScripting, Error, "Could not add field {} to {} - The type {} cannot be used as a data field",
				field.GetName(), GetName(), fieldType->GetName());
			continue;
		}

		if (std::find_if(membersToAdd.begin(), membersToAdd.end(),
			[&field](const MemberToAdd& existingMember)
			{
				return field.GetName() == existingMember.mName;
			}) != membersToAdd.end())
		{
			LOG(LogScripting, Error, "Could not add field {} to {} - There is already a field with that name",
				field.GetName(), GetName());
			continue;
		}

		const TypeInfo fieldTypeInfo = fieldType->GetTypeInfo();

		ourTypeInfo.mFlags &= fieldTypeInfo.mFlags;

		if (!field.WasValueDefaultConstructed())
		{
			ourTypeInfo.mFlags &= ~TypeInfo::IsTriviallyDefaultConstructible;
		}

		membersToAdd.emplace_back(*fieldType, field.GetName());
	}

	// Set the alignment to be that of the largest field
	const uint32 alignment = std::max(std::max_element(membersToAdd.begin(), membersToAdd.end(),
		[](const MemberToAdd& lhs, const MemberToAdd& rhs)
		{
			return lhs.mType.get().GetAlignment() > rhs.mType.get().GetAlignment();
		})->mType.get().GetAlignment(), 1u);

	ASSERT(alignment < TypeInfo::sMaxAlign);

	ourTypeInfo.mFlags |= alignment << TypeInfo::sAlignShift;

	// Sort by size to reduce padding
	std::sort(membersToAdd.begin(), membersToAdd.end(),
		[](const MemberToAdd& lhs, const MemberToAdd& rhs)
		{
			return lhs.mType.get().GetSize() > rhs.mType.get().GetSize();
		});

	// Calculate our size and padding
	uint32 size{};

	for (MemberToAdd& field : membersToAdd)
	{
		const MetaType& memberType = field.mType;

		// Insert padding bytes infront of it to satisfy alignment
		size += size % memberType.GetAlignment();

		field.mOffset = size;

		size += memberType.GetSize();
	}

	// Insert padding bytes to make sure the alignment is correct in arrays
	size += !membersToAdd.empty() ? size % membersToAdd[0].mType.get().GetAlignment() : 0;
	size = std::max(size, 1u);

	ourTypeInfo.mFlags |= size;
	ASSERT(size < TypeInfo::sMaxSize);

	MetaType& metaType = MetaManager::Get().AddType({ ourTypeInfo, GetName() });

	metaType.GetProperties().Set(sWasTypeCreatedFromScriptProperty, true);
	metaType.GetProperties().Add(Props::sIsScriptableTag);
	metaType.GetProperties().Set(Props::sComponentTag, true);

	for (MemberToAdd& memberToAdd : membersToAdd)
	{
		MetaField& newMember = metaType.AddField(memberToAdd.mType, memberToAdd.mOffset, memberToAdd.mName);
		MetaProps& memberProperties = newMember.GetProperties();

		memberProperties.Add(Props::sIsScriptableTag);

		if (newMember.GetName() == sNameOfOwnerField.StringView())
		{
			// Don't allow the user to inspect it
			memberProperties.Set(Props::sNoInspectTag, true);
			memberProperties.Set(Props::sNoSerializeTag, true);
		}
	}

	AddDefaultConstructor(metaType, false);
	AddMoveConstructor(metaType, false);
	AddCopyConstructor(metaType, false);
	AddMoveAssign(metaType, false);
	AddCopyAssign(metaType, false);
	AddDestructor(metaType, false);

	ReflectComponentType(metaType);

	LOG(LogScripting, Verbose, "Finished making type from script {}", GetName());

	return &metaType;
}

void Engine::Script::PostDeclarationRefresh()
{
	for (ScriptFunc& func : mFunctions)
	{
		func.PostDeclarationRefresh();
	}
}

void Engine::Script::DeclareMetaFunctions(MetaType& type)
{
	// Add all the other functions
	for (ScriptFunc& func : mFunctions)
	{
		func.DeclareMetaFunc(type);
	}
}

void Engine::Script::DefineMetaType(MetaType& type, bool OnlyDefineBigFiveAndDestructor)
{
	AddDefaultConstructor(type, true);
	AddMoveConstructor(type, true);
	AddCopyConstructor(type, true);
	AddMoveAssign(type, true);
	AddCopyAssign(type, true);
	AddDestructor(type, true);

	if (OnlyDefineBigFiveAndDestructor)
	{
		return;
	}

	for (ScriptFunc& scriptFunc : mFunctions)
	{
		MetaFunc* metaFunc = type.TryGetFunc(scriptFunc.GetName());

		if (metaFunc == nullptr)
		{
			LOG(LogScripting, Error, "Expected to find function {} in {} while defining script type",
				scriptFunc.GetName(),
				type.GetName());
			continue;
		}

		scriptFunc.DefineMetaFunc(*metaFunc);
	}
}

void Engine::Script::CollectErrors(ScriptErrorInserter inserter) const
{
	for (const ScriptField& field : mFields)
	{
		field.CollectErrors(inserter, *this);
	}

	for (const ScriptFunc& func : mFunctions)
	{
		func.CollectErrors(inserter, *this);
	}
}

void Engine::Script::OnSave(AssetSaveInfo& saveInfo) const
{
	BinaryGSONObject obj{};

	BinaryGSONObject& functions = obj.AddGSONObject("functions");
	for (const ScriptFunc& func : mFunctions)
	{
		func.SerializeTo(functions.AddGSONObject(""));
	}

	BinaryGSONObject& members = obj.AddGSONObject("members");
	for (const ScriptField& field : mFields)
	{
		field.SerializeTo(members.AddGSONObject(""));
	}

	obj.SaveToBinary(saveInfo.GetStream());
}

template<typename T>
typename std::vector<T>::iterator Engine::Script::GetByName(std::vector<T>& vector, const Name name)
{
	return std::find_if(vector.begin(), vector.end(),
		[name](const T& item)
		{
			return name.GetHash() == Name::HashString(item.GetName());
		});
}

void Engine::Script::AddDefaultConstructor(MetaType& toType, bool define) const
{
	static constexpr OperatorType op = OperatorType::constructor;
	static constexpr TypeTraits ret = { MakeTypeTraits<void>() };
	const std::vector<TypeTraits> params = { };

	if (!define)
	{
		toType.AddFunc(
			[](MetaFunc::DynamicArgs, MetaFunc::RVOBuffer) -> FuncResult
			{
				ABORT;
				return std::nullopt;
			},
			op,
			MetaFunc::Return{ ret }, // Return value
			MetaFunc::Params{ params.data(), params.data() + params.size() }// Parameters
		);
		ASSERT(toType.TryGetDefaultConstructor() != nullptr);
		return;
	}

	MetaFunc& func = *toType.TryGetFunc(op, MakeFuncId(ret, params));

	std::vector<std::optional<BinaryGSONMember>> serializedDefaultValues{};

	for (const MetaField& metaField : toType.EachField())
	{
		std::optional<BinaryGSONMember>& serializedNonDefaultValue = serializedDefaultValues.emplace_back();

		if (metaField.GetName() == sNameOfOwnerField.StringView())
		{
			continue;
		}

		const ScriptField& scriptField = *TryGetField(metaField.GetName());
		const MetaType& fieldType = metaField.GetType();

		if (!scriptField.WasValueDefaultConstructed())
		{
			// Remove std::nullopt
			serializedNonDefaultValue = BinaryGSONMember{};

			FuncResult serializeResult = fieldType.CallFunction(sSerializeMemberFuncName, *serializedNonDefaultValue, scriptField.GetDefaultValue());
			ASSERT_LOG(!serializeResult.HasError(), "{}", serializeResult.Error());
		}
	}

	func.RedirectFunction(
		[&toType, serializedDefaultValues](MetaFunc::DynamicArgs, MetaFunc::RVOBuffer addr) -> FuncResult
		{
			ASSERT(addr != nullptr);

			const uintptr addrAsInt = reinterpret_cast<uintptr>(addr);

			int i = -1;
			for (const MetaField& field : toType.EachField())
			{
				++i;
				const MetaType& memberType = field.GetType();

				const uintptr memberOffset = field.GetOffset();
				void* memberAddress = reinterpret_cast<void*>(addrAsInt + memberOffset);

				[[maybe_unused]] FuncResult constructedMemberResult = memberType.ConstructAt(memberAddress);
				ASSERT_LOG(!constructedMemberResult.HasError(), "{}", constructedMemberResult.Error());

				const std::optional<BinaryGSONMember>& defaultValue = serializedDefaultValues[i];

				if (!defaultValue.has_value())
				{
					continue;
				}

				[[maybe_unused]] FuncResult deserializeResult = memberType.CallFunction(sDeserializeMemberFuncName, *defaultValue, constructedMemberResult.GetReturnValue());
				ASSERT_LOG(!deserializeResult.HasError(), "{}", deserializeResult.Error());
			}

			return std::nullopt;
		}
	);
}

void Engine::Script::AddMoveConstructor(MetaType& toType, bool define) const
{
	static constexpr OperatorType op = OperatorType::constructor;
	static constexpr TypeTraits ret = MakeTypeTraits<void>();
	const std::vector<TypeTraits> params = { { toType.GetTypeId(), TypeForm::RValue } };

	if (!define)
	{
		toType.AddFunc(
			[](MetaFunc::DynamicArgs, MetaFunc::RVOBuffer) -> FuncResult
			{
				ABORT;
				return std::nullopt;
			},
			op,
			MetaFunc::Return{ ret }, // Return value
			MetaFunc::Params{ params.data(), params.data() + params.size() }// Parameters
		);
		ASSERT(toType.TryGetMoveConstructor() != nullptr);

		return;
	}

	MetaFunc& func = *toType.TryGetFunc(op, MakeFuncId(ret, params));

	func.RedirectFunction(
		[&toType](MetaFunc::DynamicArgs args, MetaFunc::RVOBuffer addr) -> FuncResult
		{
			MetaAny& moveOutOf = args[0];
			ASSERT(addr != nullptr);

			const uintptr addrAsInt = reinterpret_cast<uintptr>(addr);

			for (const MetaField& field : toType.EachField())
			{
				const MetaType& memberType = field.GetType();
				const uintptr memberOffset = field.GetOffset();

				void* memberAddress = reinterpret_cast<void*>(addrAsInt + memberOffset);

				[[maybe_unused]] FuncResult result = memberType.ConstructAt(memberAddress, field.MakeRef(moveOutOf));
				ASSERT_LOG(!result.HasError(), "{}", result.Error());
			}

			return MetaAny{ toType, addr, false };
		}
	);
}

void Engine::Script::AddCopyConstructor(MetaType& toType, bool define) const
{
	static constexpr OperatorType op = OperatorType::constructor;
	static constexpr TypeTraits ret = MakeTypeTraits<void>();
	const std::vector<TypeTraits> params = { { toType.GetTypeId(), TypeForm::ConstRef } };

	if (!define)
	{
		toType.AddFunc(
			[](MetaFunc::DynamicArgs, MetaFunc::RVOBuffer) -> FuncResult
			{
				ABORT;
				return std::nullopt;
			},
			op,
			MetaFunc::Return{ ret }, // Return value
			MetaFunc::Params{ params.data(), params.data() + params.size() }// Parameters
		);
		ASSERT(toType.TryGetCopyConstructor() != nullptr);

		return;
	}

	MetaFunc& func = *toType.TryGetFunc(op, MakeFuncId(ret, params));

	func.RedirectFunction(
		[&toType](MetaFunc::DynamicArgs args, MetaFunc::RVOBuffer addr) -> FuncResult
		{
			MetaAny& copyFrom = args[0];
			ASSERT(addr != nullptr);

			const uintptr addrAsInt = reinterpret_cast<uintptr>(addr);

			for (const MetaField& field : toType.EachField())
			{
				const MetaType& memberType = field.GetType();
				const uintptr memberOffset = field.GetOffset();
				void* memberAddress = reinterpret_cast<void*>(addrAsInt + memberOffset);

				[[maybe_unused]] FuncResult result = memberType.ConstructAt(memberAddress, field.MakeRef(copyFrom));
				ASSERT_LOG(!result.HasError(), "{}", result.Error());
			}

			return MetaAny{ toType, addr, false };
		}
	);
}

void Engine::Script::AddMoveAssign(MetaType& toType, bool define) const
{
	static constexpr OperatorType op = OperatorType::assign;
	const TypeTraits ret = { toType.GetTypeId(), TypeForm::Ref };
	const std::vector<TypeTraits> params = { { toType.GetTypeId(), TypeForm::Ref }, { toType.GetTypeId(), TypeForm::RValue } };

	if (!define)
	{
		toType.AddFunc(
			[](MetaFunc::DynamicArgs, MetaFunc::RVOBuffer) -> FuncResult
			{
				ABORT;
				return std::nullopt;
			},
			op,
			MetaFunc::Return{ ret }, // Return value
			MetaFunc::Params{ params.data(), params.data() + params.size() }// Parameters
		);

		return;
	}

	MetaFunc& func = *toType.TryGetFunc(op, MakeFuncId(ret, params));

	func.RedirectFunction(
		[&toType](MetaFunc::DynamicArgs args, MetaFunc::RVOBuffer) -> FuncResult
		{
			MetaAny& lhs = args[0];
			MetaAny& rhs = args[1];
			ASSERT(lhs.IsExactly(toType.GetTypeId()));
			ASSERT(rhs.IsExactly(toType.GetTypeId()));

			for (const MetaField& field : toType.EachField())
			{
				MetaAny valueToAssignTo = field.MakeRef(lhs);
				MetaAny valueToMove = field.MakeRef(rhs);

				const MetaType& memberType = field.GetType();

				[[maybe_unused]] FuncResult result = memberType.CallFunction(OperatorType::assign, valueToAssignTo, std::move(valueToMove));
				ASSERT_LOG(!result.HasError(), "{}", result.Error());
			}

			return MakeRef(lhs);
		}
	);
}

void Engine::Script::AddCopyAssign(MetaType& toType, bool define) const
{
	static constexpr OperatorType op = OperatorType::assign;
	const TypeTraits ret = { toType.GetTypeId(), TypeForm::Ref };
	const std::vector<TypeTraits> params = { { toType.GetTypeId(), TypeForm::Ref }, { toType.GetTypeId(), TypeForm::ConstRef } };

	if (!define)
	{
		toType.AddFunc(
			[](MetaFunc::DynamicArgs, MetaFunc::RVOBuffer) -> FuncResult
			{
				ABORT;
				return std::nullopt;
			},
			op,
			MetaFunc::Return{ ret }, // Return value
			MetaFunc::Params{ params.data(), params.data() + params.size() }// Parameters
		);

		return;
	}

	MetaFunc& func = *toType.TryGetFunc(op, MakeFuncId(ret, params));

	func.RedirectFunction(
		[&toType](MetaFunc::DynamicArgs args, MetaFunc::RVOBuffer) -> FuncResult
		{
			MetaAny& lhs = args[0];
			MetaAny& rhs = args[1];
			ASSERT(lhs.IsExactly(toType.GetTypeId()));
			ASSERT(rhs.IsExactly(toType.GetTypeId()));

			for (const MetaField& field : toType.EachField())
			{
				MetaAny valueToAssignTo = field.MakeRef(lhs);
				MetaAny valueToCopy = field.MakeRef(rhs);

				const MetaType& memberType = field.GetType();

				[[maybe_unused]] FuncResult result = memberType.CallFunction(OperatorType::assign, valueToAssignTo, valueToCopy);
				ASSERT_LOG(!result.HasError(), "{}", result.Error());
			}

			return MakeRef(lhs);
		}
	);
}

void Engine::Script::AddDestructor(MetaType& toType, bool define) const
{
	static constexpr OperatorType op = OperatorType::destructor;
	static constexpr TypeTraits ret = MakeTypeTraits<void>();
	static constexpr TypeTraits voidPtr = MakeTypeTraits<void>();

	const std::vector<TypeTraits> params = { voidPtr };

	if (!define)
	{
		toType.AddFunc(
			[](MetaFunc::DynamicArgs, MetaFunc::RVOBuffer) -> FuncResult
			{
				ABORT;
				return std::nullopt;
			},
			op,
			MetaFunc::Return{ ret }, // Return value
			MetaFunc::Params{ params.data(), params.data() + params.size() }// Parameters
		);

		return;
	}

	MetaFunc& func = *toType.TryGetFunc(op, MakeFuncId(ret, params));

	func.RedirectFunction(
		[&toType](MetaFunc::DynamicArgs args, MetaFunc::RVOBuffer) -> FuncResult
		{
			const uintptr addrAsInt = reinterpret_cast<uintptr>(args[0].GetData());

			for (const MetaField& field : toType.EachField())
			{
				const MetaType& memberType = field.GetType();

				const uintptr memberOffset = field.GetOffset();
				void* memberAddress = reinterpret_cast<void*>(addrAsInt + memberOffset);
				memberType.Destruct(memberAddress, false);
			}

			return std::nullopt;
		}
	);
}

Engine::MetaType Engine::Script::Reflect()
{
	MetaType type = MetaType{ MetaType::T<Script>{}, "Script", MetaType::Base<Asset>{}, MetaType::Ctor<AssetLoadInfo&>{} };

	SetClassVersion(type, 1);

	ReflectAssetType<Script>(type);
	return type;
}