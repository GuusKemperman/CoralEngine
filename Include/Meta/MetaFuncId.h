#pragma once
#include "Meta/MetaTypeTraits.h"

namespace Engine
{
	using FuncId = uint32;

	namespace Internal
	{
		static constexpr uint32 CombineHashes(uint32 hash1, uint32 hash2)
		{
			hash1 ^= hash2 + 0x9e3779b9 + (hash1 << 6) + (hash1 >> 2);
			return hash1;
		}

		template<typename T>
		struct FunctionHasher
		{
			static_assert(AlwaysFalse<T>);
		};

		template<typename Ret, typename... Types>
		struct FunctionHasher<Ret(Types...)>
		{
			static CONSTEVAL uint32 value()
			{
				uint32 current = MakeTypeTraits<Ret>().Hash();
				(
					[&]
					{
						current = CombineHashes(current, MakeTypeTraits<Types>().Hash());
					}
				(), ...);

				return current;
			}
		};
	}

	template<typename T>
	static CONSTEVAL FuncId MakeFuncId()
	{
		return Internal::FunctionHasher<T>::value();
	}

	FuncId MakeFuncId(const TypeTraits returnType, const std::vector<TypeTraits>& parameters);
}
