#pragma once
#include <memory>

namespace Engine
{
	template<typename CastTo, typename CastFrom>
	std::unique_ptr<CastTo> UniqueCast(std::unique_ptr<CastFrom> castFrom)
	{
		return std::unique_ptr<CastTo>(static_cast<CastTo*>(castFrom.release()));
	}

	template<typename T, bool UseAlignedFree = false>
	struct InPlaceDeleter
	{
		void operator()(T* obj)
		{
			if (obj == nullptr)
			{
				return;
			}

			obj->~T();

			if constexpr (UseAlignedFree)
			{
				_aligned_free(obj);
			}
			else
			{
				free(obj);
			}
		}
	};

	template<typename T, typename... Args, std::enable_if_t<std::is_constructible_v<T, Args...>, bool> = true>
	std::unique_ptr<T, InPlaceDeleter<T, true>> MakeUniqueInPlace(Args&& ...args)
	{
		return std::unique_ptr<T, InPlaceDeleter<T, true>>{ new (_aligned_malloc(sizeof(T), alignof(T))) T(std::forward<Args>(args)...) };
	}
}
