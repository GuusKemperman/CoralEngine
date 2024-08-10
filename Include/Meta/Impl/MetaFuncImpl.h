#pragma once
#include "Meta/Fwd/MetaFuncFwd.h"

#include "Meta/MetaAny.h"
#include "Meta/MetaTypeTraits.h"
#include "Meta/MetaFuncId.h"

namespace CE::Internal
{
	template<typename T>
	struct ValidNumOfParams
	{
		static_assert(AlwaysFalse<T>, "Not a function signature");
	};

	template<typename Ret, typename... Params>
	struct ValidNumOfParams<Ret(Params...)>
	{
		template<typename... ParamAndRetNames>
		static constexpr bool Value()
		{
			// The return value may also have a name
			return sizeof...(Params) + (std::is_same_v<Ret, void> ? 0 : 1) >= sizeof...(ParamAndRetNames);
		}
	};

	template<typename FuncSig, typename... ParamAndRetNames>
	static constexpr bool ValidNumOfParamsV = ValidNumOfParams<FuncSig>::template Value<ParamAndRetNames...>();
}

template<typename Ret, typename... ParamsT, typename... ParamAndRetNames>
CE::MetaFunc::MetaFunc(std::function<Ret(ParamsT...)>&& func,
	const NameOrTypeInit typeOrName,
	ParamAndRetNames&&... paramAndRetNames) :
	MetaFunc(
		InvokeT
		{
			[func](DynamicArgs args, RVOBuffer buffer) -> FuncResult
			{
				return DefaultInvoke<Ret, ParamsT...>(func, args, buffer);
			}
		},
		typeOrName,
		[&]
		{
			std::array<TypeTraits, sizeof...(ParamsT) + 1> paramTraits{ MakeTypeTraits<ParamsT>()..., MakeTypeTraits<Ret>() };
			std::array<std::string, sizeof...(ParamAndRetNames)> paramNames{ std::forward<ParamAndRetNames>(paramAndRetNames)... };

			std::vector<MetaFuncNamedParam> namedParams(sizeof...(ParamsT) + 1);

			for (size_t i = 0; i < sizeof...(ParamsT) + 1; i++)
			{
				namedParams[i].mTypeTraits = paramTraits[i];
			}

			for (size_t i = 0; i < sizeof...(ParamAndRetNames); i++)
			{
				namedParams[i].mName = std::move(paramNames[i]);
			}

			return namedParams;
		}(),
			MakeFuncId<Ret(ParamsT...)>())
{
	static_assert(Internal::ValidNumOfParamsV<Ret(ParamsT...), ParamAndRetNames...>, "Too many names provided; First one name for each parameter, and if the function does not return void, one for the return value.");
}

template <typename Ret, typename Obj, typename ... ParamsT, typename ... ParamAndRetNames>
CE::MetaFunc::MetaFunc(Ret(Obj::* func)(ParamsT...), const NameOrTypeInit typeOrName,
	ParamAndRetNames&&... paramAndRetNames) :
	MetaFunc(std::function<Ret(Obj&, ParamsT...)>{ func }, typeOrName, std::forward<ParamAndRetNames>(paramAndRetNames)...)
{}

template <typename Ret, typename Obj, typename ... ParamsT, typename ... ParamAndRetNames>
CE::MetaFunc::MetaFunc(Ret(Obj::* func)(ParamsT...) const, const NameOrTypeInit typeOrName,
	ParamAndRetNames&&... paramAndRetNames) :
	MetaFunc(std::function<Ret(const Obj&, ParamsT...)>{ func }, typeOrName, std::forward<ParamAndRetNames>(paramAndRetNames)...)
{}

template <typename Ret, typename ... ParamsT, typename ... ParamAndRetNames>
CE::MetaFunc::MetaFunc(Ret(*func)(ParamsT...), const NameOrTypeInit typeOrName, ParamAndRetNames&&... paramAndRetNames) :
	MetaFunc(std::function<Ret(ParamsT...)>{ func }, typeOrName, std::forward<ParamAndRetNames>(paramAndRetNames)...)
{}

template <typename T, typename ... ParamAndRetNames, std::enable_if_t<std::is_invocable_v<T>, bool>>
CE::MetaFunc::MetaFunc(const T& functor, const NameOrTypeInit typeOrName, ParamAndRetNames&&... paramAndRetNames) :
	MetaFunc(std::function<decltype(functor())()>{ functor }, typeOrName, std::forward<ParamAndRetNames>(paramAndRetNames)...)
{}

template <typename T, typename ... ParamsT, typename ... ParamAndRetNames>
CE::MetaFunc::MetaFunc(const T& functor, const NameOrTypeInit typeOrName, const ExplicitParams<ParamsT...>,
                           ParamAndRetNames&&... paramAndRetNames) :
	MetaFunc(std::function<std::invoke_result_t<T, ParamsT...>(ParamsT...)>{ functor }, typeOrName, std::forward<ParamAndRetNames>(paramAndRetNames)...)
{
}

namespace CE::Internal
{
	template<typename T, size_t I>
	T UnpackSingle(std::span<MetaAny>& anies)
	{
		MetaAny& any = anies[I];
		static constexpr TypeTraits traits = MakeTypeTraits<T>();

		if constexpr (std::is_pointer_v<T>)
		{
			if constexpr (traits.mStrippedTypeId == MakeTypeId<MetaAny>())
			{
				return &any;
			}
			else
			{
				return static_cast<T>(any.GetData());
			}
		}
		else if constexpr (std::is_rvalue_reference_v<T>)
		{
			if constexpr (traits.mStrippedTypeId == MakeTypeId<MetaAny>())
			{
				return std::move(any);
			}
			else
			{
				void* data = any.Release();
				return std::move(*static_cast<std::remove_reference_t<T>*>(data));
			}
		}
		else
		{
			if constexpr (traits.mStrippedTypeId == MakeTypeId<MetaAny>())
			{
				return any;
			}
			else
			{
				return *static_cast<std::remove_reference_t<T>*>(any.GetData());
			}
		}
	}

	template <typename Ret, typename... ParamsT, size_t... Indices>
	FuncResult DefaultInvokeImpl(const std::function<Ret(ParamsT...)>& functionToInvoke,
		MetaFunc::DynamicArgs& runtimeArgs, 
		MetaFunc::RVOBuffer rvoBuffer, 
		std::index_sequence<Indices...>)
	{
		if constexpr (std::is_same_v<Ret, void>)
		{
			functionToInvoke(UnpackSingle<ParamsT, Indices>(runtimeArgs)...);
			return std::nullopt;
		}
		else
		{
			return MetaAny{ functionToInvoke(UnpackSingle<ParamsT, Indices>(runtimeArgs)...), rvoBuffer };
		}
	}
}

template <typename ... Args>
CE::FuncResult CE::MetaFunc::operator()(Args&&... args) const
{
	return InvokeCheckedUnpackedWithRVO(nullptr, std::forward<Args>(args)...);
}

template <typename ... Args>
CE::FuncResult CE::MetaFunc::InvokeCheckedUnpacked(Args&&... args) const
{
	return InvokeCheckedUnpackedWithRVO(nullptr, std::forward<Args>(args)...);
}

template <typename ... Args>
CE::FuncResult CE::MetaFunc::InvokeCheckedUnpackedWithRVO(RVOBuffer rvoBuffer, Args&&... args) const
{
	static constexpr uint32 numOfArgs = sizeof...(Args);
	std::pair<std::array<MetaAny, numOfArgs>, std::array<TypeForm, numOfArgs>> packedArgs = Pack(std::forward<Args>(args)...);
	return InvokeChecked(packedArgs.first, packedArgs.second, rvoBuffer);
}

template <typename ... Args>
CE::FuncResult CE::MetaFunc::InvokeUncheckedUnpacked(Args&&... args) const
{
	return InvokeUncheckedUnpackedWithRVO(nullptr, std::forward<Args>(args)...);
}

template <typename ... Args>
CE::FuncResult CE::MetaFunc::InvokeUncheckedUnpackedWithRVO(RVOBuffer rvoBuffer, Args&&... args) const
{
	static constexpr uint32 numOfArgs = sizeof...(Args);
	std::pair<std::array<MetaAny, numOfArgs>, std::array<TypeForm, numOfArgs>> packedArgs = Pack(std::forward<Args>(args)...);
	return InvokeUnchecked(packedArgs.first, packedArgs.second, rvoBuffer);
}

template<typename Ret, typename... ParamsT>
CE::FuncResult CE::MetaFunc::DefaultInvoke(const std::function<Ret(ParamsT...)>& functionToInvoke,
	DynamicArgs runtimeArgs, RVOBuffer rvoBuffer)
{
	std::make_index_sequence<sizeof...(ParamsT)> sequence{};
	return Internal::DefaultInvokeImpl(functionToInvoke, runtimeArgs, rvoBuffer, sequence);
}

template <typename Arg>
CE::MetaAny CE::MetaFunc::PackSingle(Arg&& arg)
{
	return MetaAny{ std::forward<Arg>(arg) };
}

template <typename ... Args>
std::pair<std::array<CE::MetaAny, sizeof...(Args)>, std::array<CE::TypeForm, sizeof...(Args)>> CE::MetaFunc::Pack(Args&&... args)
{
	return {
		std::array<MetaAny, sizeof...(Args)>{ PackSingle<Args>(std::forward<Args>(args))... },
		std::array<TypeForm, sizeof...(Args)>{ MakeTypeForm<Args>()... }
	};
}
