#include "Precomp.h"
#include "Meta/MetaType.h"

#include <forward_list>

#include "Meta/MetaProps.h"
#include "Utilities/MemFunctions.h"

CE::MetaType::MetaType(const TypeInfo typeInfo,
	const std::string_view name) :
	mTypeInfo(typeInfo),
	mName(name),
	mProperties(std::make_unique<MetaProps>())
{
}

CE::MetaType::MetaType(MetaType&& other) noexcept :
	mTypeInfo(other.mTypeInfo),
	mFunctions(std::move(other.mFunctions)),
	mFields(std::move(other.mFields)),
	mDirectBaseClasses(std::move(other.mDirectBaseClasses)),
	mDirectDerivedClasses(std::move(other.mDirectDerivedClasses)),
	mName(std::move(other.mName)),
	mProperties(std::move(other.mProperties))
{
	for (MetaField& field : mFields)
	{
		field.mOuterType = *this;
	}

	for (const MetaType& base : mDirectBaseClasses)
	{
		auto ourselves = std::find_if(base.mDirectDerivedClasses.begin(), base.mDirectDerivedClasses.end(), 
			[&other](const MetaType& derived) { return derived == other; });
		ASSERT(ourselves != base.mDirectDerivedClasses.end());
		*ourselves = *this;
	}

	for (const MetaType& derived : mDirectDerivedClasses)
	{
		auto ourselves = std::find_if(derived.mDirectBaseClasses.begin(), derived.mDirectBaseClasses.end(),
			[&other](const MetaType& base) { return base == other; });
		ASSERT(ourselves != derived.mDirectBaseClasses.end());
		*ourselves = *this;
	}
}

CE::MetaType::~MetaType() = default;

bool CE::MetaType::IsDerivedFrom(const TypeId baseClassTypeId) const
{
	if (GetTypeId() == baseClassTypeId)
	{
		return true;
	}

	for (const MetaType& base : mDirectBaseClasses)
	{
		if (base.IsDerivedFrom(baseClassTypeId))
		{
			return true;
		}
	}

	return false;
}

bool CE::MetaType::IsBaseClassOf(const TypeId derivedClassTypeId) const
{
	if (GetTypeId() == derivedClassTypeId)
	{
		return true;
	}

	for (const MetaType& child : mDirectDerivedClasses)
	{
		if (child.IsBaseClassOf(derivedClassTypeId))
		{
			return true;
		}
	}

	return false;
}

void CE::MetaType::AddBaseClass(const MetaType& baseClass)
{
	mDirectBaseClasses.push_back(baseClass);
	baseClass.mDirectDerivedClasses.push_back(*this);
}

size_t CE::MetaType::RemoveFunc(const std::variant<Name, OperatorType>& nameOrType)
{
	const auto candidates = mFunctions.equal_range(FuncKey{ nameOrType });

	size_t numRemoved{};
	for (auto i = candidates.first; i != candidates.second; ++numRemoved)
	{
		i = mFunctions.erase(i);
	}

	return numRemoved;
}

size_t CE::MetaType::RemoveFunc(const std::variant<Name, OperatorType>& nameOrType, uint32 funcId)
{
	const auto candidates = mFunctions.equal_range(FuncKey{ nameOrType });

	size_t numRemoved{};
	for (auto i = candidates.first; i != candidates.second;)
	{
		if (i->second.GetFuncId() == funcId)
		{
			i = mFunctions.erase(i);
			++numRemoved;
		}
		else
		{
			++i;
		}
	}

	return numRemoved;
}

void* CE::MetaType::Malloc(uint32 amount) const
{
	return FastAlloc(GetSize() * amount, GetAlignment());
}

void CE::MetaType::Free(void* buffer)
{
	FastFree(buffer);
}

const CE::MetaFunc* CE::MetaType::TryGetConstructor(const std::vector<TypeTraits>& parameters) const
{
	return TryGetFunc(OperatorType::constructor, MakeFuncId(MakeTypeTraits<void>(), parameters));
}

const CE::MetaFunc* CE::MetaType::TryGetDefaultConstructor() const
{
	return IsDefaultConstructible() ? TryGetFunc(OperatorType::constructor, MakeFuncId<void()>()) : nullptr;
}

const CE::MetaFunc* CE::MetaType::TryGetCopyConstructor() const
{
	return IsCopyConstructible() ? TryGetFunc(OperatorType::constructor,
		MakeFuncId(MakeTypeTraits<void>(), { TypeTraits{ GetTypeId(), TypeForm::ConstRef } })) : nullptr;
}

const CE::MetaFunc* CE::MetaType::TryGetMoveConstructor() const
{
	return IsMoveConstructible() ? TryGetFunc(OperatorType::constructor,
		MakeFuncId(MakeTypeTraits<void>(), { TypeTraits{ GetTypeId(), TypeForm::RValue } })) : nullptr;
}

const CE::MetaFunc* CE::MetaType::TryGetCopyAssign() const
{
	return IsCopyAssignable() ? TryGetFunc(OperatorType::assign,
		MakeFuncId(TypeTraits{ GetTypeId(), TypeForm::Ref }, { TypeTraits{ GetTypeId(), TypeForm::Ref }, TypeTraits{GetTypeId(), TypeForm::ConstRef} })) : nullptr;
}

const CE::MetaFunc* CE::MetaType::TryGetMoveAssign() const
{
	return IsMoveAssignable() ? TryGetFunc(OperatorType::assign,
		MakeFuncId(TypeTraits{ GetTypeId(), TypeForm::Ref }, { TypeTraits{ GetTypeId(), TypeForm::Ref }, TypeTraits{GetTypeId(), TypeForm::RValue} })) : nullptr;
}

const CE::MetaFunc& CE::MetaType::GetDestructor() const
{
	auto it = mFunctions.find(FuncKey{ OperatorType::destructor });
	ASSERT(it != mFunctions.end());
	return it->second;
}

void CE::MetaType::Destruct(void* ptrToObjectOfThisType, bool freeBuffer) const
{
	ASSERT(ptrToObjectOfThisType != nullptr);

	if ((mTypeInfo.mFlags & TypeInfo::IsTriviallyDestructible) == 0)
	{
		// Cast is there to prevent the function from interpreting it as void*&
		GetDestructor().InvokeUncheckedUnpacked(static_cast<void*>(ptrToObjectOfThisType));
	}

	if (freeBuffer)
	{
		Free(ptrToObjectOfThisType);
	}
}

std::vector<std::reference_wrapper<const CE::MetaField>> CE::MetaType::EachField() const
{
	return EachField<const MetaField>(*this);
}

std::vector<std::reference_wrapper<CE::MetaField>> CE::MetaType::EachField()
{
	return EachField<MetaField>(*this);
}

template<typename FieldType, typename Self>
std::vector<std::reference_wrapper<FieldType>> CE::MetaType::EachField(Self& self)
{
	std::vector<std::reference_wrapper<FieldType>> eachMember{};

	std::function<void(Self&)> addBaseClassesThenSelf = [&eachMember, &addBaseClassesThenSelf](Self& type)
		{
			for (const MetaType& baseClass : type.GetDirectBaseClasses())
			{
				addBaseClassesThenSelf(const_cast<MetaType&>(baseClass));
			}

			for (FieldType& field : type.GetDirectFields())
			{
				eachMember.push_back(field);
			}
		};
	addBaseClassesThenSelf(self);

	return eachMember;
}

std::vector<std::reference_wrapper<const CE::MetaFunc>> CE::MetaType::EachFunc() const
{
	return EachFunc<const MetaFunc>(*this);
}

std::vector<std::reference_wrapper<CE::MetaFunc>> CE::MetaType::EachFunc()
{
	return EachFunc<MetaFunc>(*this);
}

template <typename FuncType, typename Self>
std::vector<std::reference_wrapper<FuncType>> CE::MetaType::EachFunc(Self& self)
{
	std::vector<std::reference_wrapper<FuncType>> eachFunc{};

	std::function<void(Self&)> addBaseClassesThenSelf = [&eachFunc, &addBaseClassesThenSelf](Self& type)
		{
			for (const MetaType& baseClass : type.GetDirectBaseClasses())
			{
				addBaseClassesThenSelf(const_cast<MetaType&>(baseClass));
			}

			std::vector funcs = type.GetDirectFuncs();
			eachFunc.insert(eachFunc.end(), funcs.begin(), funcs.end());
		};
	addBaseClassesThenSelf(self);

	return eachFunc;
}

std::vector<std::reference_wrapper<CE::MetaFunc>> CE::MetaType::GetDirectFuncs()
{
	return GetDirectFuncs<MetaFunc>(*this);
}

std::vector<std::reference_wrapper<const CE::MetaFunc>> CE::MetaType::GetDirectFuncs() const
{
	return GetDirectFuncs<const MetaFunc>(*this);
}

template <typename FuncType, typename Self>
std::vector<std::reference_wrapper<FuncType>> CE::MetaType::GetDirectFuncs(Self& self)
{
	std::vector<std::reference_wrapper<FuncType>> returnValue{};
	returnValue.reserve(self.mFunctions.size());

	for (auto& [nameHash, func] : self.mFunctions)
	{
		returnValue.emplace_back(func);
	}

	return returnValue;
}

CE::MetaField* CE::MetaType::TryGetField(const Name name)
{
	return const_cast<MetaField*>(const_cast<const MetaType*>(this)->TryGetField(name));
}

const CE::MetaField* CE::MetaType::TryGetField(const Name name) const
{
	const auto it = std::find_if(mFields.begin(), mFields.end(),
	                             [name](const MetaField& field)
	                             {
		                             return name == field.GetName();
	                             });

	if (it == mFields.end())
	{
		for (const MetaType& baseClass : mDirectBaseClasses)
		{
			const MetaField* const memberInParent = baseClass.TryGetField(name);

			if (memberInParent != nullptr)
			{
				return memberInParent;
			}
		}
		return nullptr;
	}
	return &*it;
}

CE::MetaFunc* CE::MetaType::TryGetFunc(const std::variant<Name, OperatorType>& nameOrType)
{
	return const_cast<MetaFunc*>(const_cast<const MetaType*>(this)->TryGetFunc(nameOrType));
}

const CE::MetaFunc* CE::MetaType::TryGetFunc(const std::variant<Name, OperatorType>& nameOrType) const
{
	const auto it = mFunctions.find(FuncKey{ nameOrType });

	if (it == mFunctions.end())
	{
		for (const MetaType& baseClass : mDirectBaseClasses)
		{
			const MetaFunc* const funcInParent = baseClass.TryGetFunc(nameOrType);

			if (funcInParent != nullptr)
			{
				return funcInParent;
			}
		}

		return nullptr;
	}

	return &it->second;
}

// TODO does not work if the function is in a baseclass
CE::FuncResult CE::MetaType::CallFunction(const std::variant<Name, OperatorType>& funcNameOrType, std::span<MetaAny> args, 
	std::span<const TypeForm> formOfArgs, MetaFunc::RVOBuffer rvoBuffer) const
{
	const auto candidates = mFunctions.equal_range(FuncKey{ funcNameOrType });

	std::forward_list<std::string> failedFuncResults{};

	for (auto it = candidates.first; it != candidates.second; ++it)
	{
		std::optional<std::string> reasonWhyWeCannotInvoke = MetaFunc::CanArgBePassedIntoParam(args, formOfArgs, it->second.GetParameters());

		if (reasonWhyWeCannotInvoke.has_value())
		{
			failedFuncResults.emplace_front(std::move(*reasonWhyWeCannotInvoke));
			continue;
		}

		return it->second.InvokeUnchecked(args, formOfArgs, rvoBuffer);
	}

	std::string errorMessage = Format("There is no function {}, or the arguments could not be converted to a valid overload", 
		MetaFunc::GetDesignerFriendlyName(std::holds_alternative<Name>(funcNameOrType) ?
			MetaFunc::NameOrType{ std::get<Name>(funcNameOrType).String() }
			: MetaFunc::NameOrType{ std::get<OperatorType>(funcNameOrType) }));

	uint32 overloadNum = 0;
	for (const std::string& str : failedFuncResults)
	{
		errorMessage.append(Format("\nCould not invoke overload {} - {}", overloadNum++, str));
	}

	return std::move(errorMessage);
}

CE::MetaFunc* CE::MetaType::TryGetFunc(const std::variant<Name, OperatorType>& nameOrType, const uint32 funcId)
{
	return const_cast<MetaFunc*>(const_cast<const MetaType*>(this)->TryGetFunc(nameOrType, funcId));
}

const CE::MetaFunc* CE::MetaType::TryGetFunc(const std::variant<Name, OperatorType>& nameOrType, const uint32 funcId) const
{
	const auto candidates = mFunctions.equal_range(FuncKey{ nameOrType });

	for (auto i = candidates.first; i != candidates.second; ++i)
	{
		if (i->second.GetFuncId() == funcId)
		{
			return &i->second;
		}
	}

	for (const MetaType& baseClass : mDirectBaseClasses)
	{
		const MetaFunc* const funcInParent = baseClass.TryGetFunc(nameOrType, funcId);

		if (funcInParent != nullptr)
		{
			return funcInParent;
		}
	}

	return nullptr;
}

CE::FuncResult CE::MetaType::ConstructInternal(bool isOwner, void* address) const
{
	if (mTypeInfo.mFlags & TypeInfo::IsTriviallyDefaultConstructible)
	{
		LIKELY;
		memset(address, 0, GetSize());
		return MetaAny{ *this, address, isOwner };
	}

	if (mTypeInfo.mFlags & TypeInfo::IsDefaultConstructible)
	{
		FuncResult result = TryGetDefaultConstructor()->InvokeUncheckedUnpackedWithRVO(address);

		if (result.HasError())
		{
			return result;
		}
		return MetaAny{ *this, address, isOwner };
	}
	UNLIKELY;
	return { "Type is not default constructible" };
}

CE::FuncResult CE::MetaType::ConstructInternal(bool isOwner, void* address, const MetaAny& args) const
{
	if (args.GetTypeId() != GetTypeId())
	{
		UNLIKELY;
		return ConstructInternalGeneric(isOwner, address, args);
	}

	if (mTypeInfo.mFlags & TypeInfo::IsTriviallyCopyConstructible)
	{
		LIKELY;
		memcpy(address, args.GetData(), GetSize());
		return MetaAny{ *this, address, isOwner };
	}

	if (mTypeInfo.mFlags & TypeInfo::IsCopyConstructible)
	{
		FuncResult result = TryGetCopyConstructor()->InvokeUncheckedUnpackedWithRVO(address, args);

		if (result.HasError())
		{
			return result;
		}
		return MetaAny{ *this, address, isOwner };
	}
	UNLIKELY;
	return { "Type is not copy constructible" };
}

CE::FuncResult CE::MetaType::ConstructInternal(bool isOwner, void* address, MetaAny& args) const
{
	return ConstructInternal(isOwner, address, const_cast<const MetaAny&>(args));
}

CE::FuncResult CE::MetaType::ConstructInternal(bool isOwner, void* address, MetaAny&& args) const
{
	if (args.GetTypeId() != GetTypeId())
	{
		UNLIKELY;
		return ConstructInternalGeneric(isOwner, address, std::move(args));
	}

	if (mTypeInfo.mFlags & TypeInfo::IsTriviallyMoveConstructible)
	{
		LIKELY;
		memmove(address, args.GetData(), GetSize());
		return MetaAny{ *this, address, isOwner };
	}

	if (mTypeInfo.mFlags & TypeInfo::IsMoveConstructible)
	{
		FuncResult result = TryGetMoveConstructor()->InvokeUncheckedUnpackedWithRVO(address, std::move(args));

		if (result.HasError())
		{
			return result;
		}
		return MetaAny{ *this, address, isOwner };
	}
	UNLIKELY;
	return { "Type is not move constructible" };
}

CE::FuncResult CE::MetaType::AssignInternal(MetaAny& destination, const MetaAny& args) const
{
	if (args.GetTypeId() != GetTypeId())
	{
		UNLIKELY;
		return AssignInternalGeneric(destination, args);
	}

	if (mTypeInfo.mFlags & TypeInfo::IsTriviallyCopyAssignable)
	{
		LIKELY;
		memcpy(destination.GetData(), args.GetData(), GetSize());
		return MetaAny{ *this, destination.GetData(), false };
	}

	if (mTypeInfo.mFlags & TypeInfo::IsCopyAssignable)
	{
		return TryGetCopyAssign()->InvokeUncheckedUnpacked(destination, args);
	}
	UNLIKELY;
	return { "Type is not copy assignable" };
}

CE::FuncResult CE::MetaType::AssignInternal(MetaAny& destination, MetaAny& args) const
{
	return AssignInternal(destination, const_cast<const MetaAny&>(args));
}

CE::FuncResult CE::MetaType::AssignInternal(MetaAny& destination, MetaAny&& args) const
{
	if (args.GetTypeId() != GetTypeId())
	{
		UNLIKELY;
		return AssignInternalGeneric(destination, std::move(args));
	}

	if (mTypeInfo.mFlags & TypeInfo::IsTriviallyMoveAssignable)
	{
		LIKELY;
		// Assumes the data doesnt overlap
		memmove(destination.GetData(), args.GetData(), GetSize());
		return MetaAny{ *this, destination.GetData(), false };
	}

	if (mTypeInfo.mFlags & TypeInfo::IsMoveAssignable)
	{
		return TryGetMoveAssign()->InvokeUncheckedUnpacked(destination, args);
	}
	UNLIKELY;
	return { "Type is not move assignable" };
}

CE::MetaType::FuncKey::FuncKey(const Name::HashType name) :
	mOperatorType(OperatorType::none),
	mNameHash(name)
{
}

CE::MetaType::FuncKey::FuncKey(const OperatorType type) :
	mOperatorType(type),
	mNameHash(0)
{
}

CE::MetaType::FuncKey::FuncKey(const MetaFunc::NameOrType& nameOrType) :
	mOperatorType(std::holds_alternative<OperatorType>(nameOrType) ? std::get<OperatorType>(nameOrType) : OperatorType::none),
	mNameHash(std::holds_alternative<std::string>(nameOrType) ? Name::HashString(std::get<std::string>(nameOrType)) : 0)
{
}

CE::MetaType::FuncKey::FuncKey(const std::variant<std::string_view, OperatorType>& nameOrType) :
	mOperatorType(std::holds_alternative<OperatorType>(nameOrType) ? std::get<OperatorType>(nameOrType) : OperatorType::none),
	mNameHash(std::holds_alternative<std::string_view>(nameOrType) ? Name::HashString(std::get<std::string_view>(nameOrType)) : 0)
{
}

CE::MetaType::FuncKey::FuncKey(const std::variant<Name, OperatorType>& nameOrType) :
	mOperatorType(std::holds_alternative<OperatorType>(nameOrType) ? std::get<OperatorType>(nameOrType) : OperatorType::none),
	mNameHash(std::holds_alternative<Name>(nameOrType) ? std::get<Name>(nameOrType).GetHash() : 0)
{
}
