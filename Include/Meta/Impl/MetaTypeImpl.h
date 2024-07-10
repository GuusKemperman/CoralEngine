#pragma once
#include "Meta/Fwd/MetaTypeFwd.h"

#include "Meta/MetaFunc.h"
#include "Meta/MetaField.h"
#include "Meta/MetaTypeTraits.h"
#include "Meta/MetaTypeId.h"
#include "Meta/MetaManager.h"

template <typename TypeT, typename ... Args>
CE::MetaType::MetaType(T<TypeT>, const std::string_view name, Args&&... args) :
	MetaType(MakeTypeInfo<TypeT>(), name)
{
	(
		[&]
		{
			AddFromArg<TypeT>(args);
		}
	(), ...);

	if constexpr (std::is_default_constructible_v<TypeT>)
	{
		AddFromArg<TypeT>(Ctor{});
		ASSERT(TryGetDefaultConstructor() != nullptr);
	}

	if constexpr (std::is_copy_constructible_v<TypeT>)
	{
		AddFromArg<TypeT>(Ctor<const TypeT&>{});
		ASSERT(TryGetCopyConstructor() != nullptr);
	}

	if constexpr (std::is_move_constructible_v<TypeT>)
	{
		AddFromArg<TypeT>(Ctor<TypeT&&>{});
		ASSERT(TryGetMoveConstructor() != nullptr);
	}

	if constexpr (std::is_move_assignable_v<TypeT>)
	{
		AddFunc(
			[](TypeT& lhs, TypeT&& rhs) -> TypeT&
			{
				lhs = std::move(rhs);
				return lhs;
			}, OperatorType::assign, MetaFunc::ExplicitParams<TypeT&, TypeT&&>{});
		ASSERT(TryGetMoveAssign() != nullptr);
	}

	if constexpr (std::is_copy_assignable_v<TypeT>)
	{
		AddFunc(
			[](TypeT& lhs, const TypeT& rhs) -> TypeT&
			{
				lhs = rhs;
				return lhs;
			}, OperatorType::assign, MetaFunc::ExplicitParams<TypeT&, const TypeT&>{});
		ASSERT(TryGetCopyAssign() != nullptr);
	}

	AddFunc(
		[](void* addr)
		{
			static_cast<TypeT*>(addr)->~TypeT();
		}, OperatorType::destructor, MetaFunc::ExplicitParams<void*>{});
}

template<typename FuncPtr, typename... Args>
CE::MetaFunc& CE::MetaType::AddFunc(FuncPtr&& funcPtr, const MetaFunc::NameOrTypeInit nameOrType, Args&& ...args)
{
	const auto result = mFunctions.emplace(nameOrType, MetaFunc{ std::forward<FuncPtr>(funcPtr), nameOrType, std::forward<Args>(args)... });
	return result->second;
}

template<typename ...Args>
CE::MetaField& CE::MetaType::AddField([[maybe_unused]] Args&& ... args)
{
	MetaField& returnValue = mFields.emplace_back(*this, std::forward<Args>(args)...);

#ifdef ASSERTS_ENABLED
	auto it = std::find_if(mFields.begin(), mFields.end() - 1,
		[&returnValue](const MetaField& field)
		{
			return field.GetName() == returnValue.GetName();
		});
	ASSERT_LOG(it == mFields.end() - 1, "{}::{} has already been added", GetName(), returnValue.GetName());
#endif // ASSERTS_ENABLED

	return returnValue;
}

namespace CE::Internal
{
	template <typename TypeT, typename... ParamsT, size_t... Indices>
	void ConstructObjectImpl(MetaFunc::DynamicArgs& runtimeArgs,
		MetaFunc::RVOBuffer rvoBuffer,
		std::index_sequence<Indices...>)
	{
		new (rvoBuffer) TypeT(UnpackSingle<ParamsT, Indices>(runtimeArgs)...);
	}
}

template<typename TypeT, typename... Args>
void CE::MetaType::AddFromArg(Ctor<Args...>)
{
	AddFunc(
		[](MetaFunc::DynamicArgs args, MetaFunc::RVOBuffer buffer) -> FuncResult
		{
			ASSERT(buffer != nullptr && "The address provided to a constructor may never be nullptr");
			std::make_index_sequence<sizeof...(Args)> sequence{};
			Internal::ConstructObjectImpl<TypeT, Args...>(args, buffer, sequence);
			return std::nullopt;
		},
		OperatorType::constructor,
		MetaFunc::Return{ MakeTypeTraits<void>() },
		MetaFunc::Params{ MakeTypeTraits<Args>()... }
	);
}

template <typename ... Args>
CE::FuncResult CE::MetaType::CallFunction(const std::variant<Name, OperatorType>& funcNameOrType, Args&&... args) const
{
	return CallFunctionWithRVO(funcNameOrType, nullptr, std::forward<Args>(args)...);
}

template <typename ... Args>
CE::FuncResult CE::MetaType::CallFunctionWithRVO(const std::variant<Name, OperatorType>& funcNameOrType,
	MetaFunc::RVOBuffer rvoBuffer, Args&&... args) const
{
	static constexpr uint32 numOfArgs = sizeof...(Args);
	std::pair<std::array<MetaAny, numOfArgs>, std::array<TypeForm, numOfArgs>> packedArgs = MetaFunc::Pack(std::forward<Args>(args)...);
	return CallFunction(funcNameOrType, Span<MetaAny>{packedArgs.first}, Span<const TypeForm>{packedArgs.second}, rvoBuffer);
}

template <typename ... Args>
bool CE::MetaType::IsConstructible() const
{
	return IsConstructible({ MakeTypeTraits<Args>()... });
}

template <typename ... Args>
CE::FuncResult CE::MetaType::Construct(Args&&... args) const
{
	void* buffer = Malloc();

	FuncResult constructResult = ConstructInternal(true, buffer, std::forward<Args>(args)...);

	if (constructResult.HasError())
	{
		Free(buffer);
	}
	return constructResult;
}

template <typename ... Args>
CE::FuncResult CE::MetaType::ConstructAt(void* atAddress, Args&&... args) const
{
	ASSERT(atAddress != nullptr);
	ASSERT(reinterpret_cast<uintptr>(atAddress) % GetAlignment() == 0 && "Address was not aligned");

	return ConstructInternal(false, atAddress, std::forward<Args>(args)...);
}

template <typename ... Args>
CE::FuncResult CE::MetaType::ConstructInternalGeneric(bool isOwner, void* address, Args&&... args) const
{
	FuncResult result = CallFunctionWithRVO(OperatorType::constructor, address, std::forward<Args>(args)...);

	if (result.HasError())
	{
		return result;
	}

	return MetaAny{ *this, address, isOwner };
}

template <typename TypeT>
CE::FuncResult CE::MetaType::ConstructInternal(bool isOwner, void* address, const TypeT& args) const
{
	if (MakeTypeId<TypeT>() != GetTypeId())
	{
		return ConstructInternalGeneric(isOwner, address, args);
	}

	if constexpr (std::is_copy_constructible_v<TypeT>)
	{
		return MetaAny{ *this, new (address) TypeT(args), isOwner };
	}
	else
	{
		return { "Type is not copy-constructible" };
	}
}

template <typename TypeT>
CE::FuncResult CE::MetaType::ConstructInternal(bool isOwner, void* address, TypeT& args) const
{
	if (MakeTypeId<TypeT>() != GetTypeId())
	{
		return ConstructInternalGeneric(isOwner, address, args);
	}

	if constexpr (std::is_copy_constructible_v<TypeT>)
	{
		return MetaAny{ *this, new (address) TypeT(args), isOwner };
	}
	else
	{
		return { "Type is not copy-constructible" };
	}
}

template <typename TypeT, std::enable_if_t<std::is_rvalue_reference_v<TypeT>, bool>>
CE::FuncResult CE::MetaType::ConstructInternal(bool isOwner, void* address, TypeT&& args) const
{
	if (MakeTypeId<TypeT>() != GetTypeId())
	{
		return ConstructInternalGeneric(isOwner, address, std::move(args));
	}

	if constexpr (std::is_move_constructible_v<TypeT>)
	{
		return MetaAny{ *this, new (address) TypeT(std::move(args)), isOwner };
	}
	else
	{
		return { "Type is not copy-constructible" };
	}
}

template<typename TypeT>
void CE::MetaType::AddBaseClass()
{
	const MetaType& base = MetaManager::Get().GetType<TypeT>();
	AddBaseClass(base);
}