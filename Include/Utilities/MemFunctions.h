#pragma once

namespace Engine
{
	/**
	 * \brief Uses the fastest available option on the target platform for allocating aligned memory.
	 *
	 * \note Objects allocated using FastMalloc must be freed using FastFree.
	 * 
	 * \param size The desired size of the buffer. 
	 * \param alignHint The desired alignment of the buffer. On some platforms, specifying alignment may not be available. The alignHint is then ignored, and a call is made to regular old malloc instead. 
	 * \return Nullptr if operation has failed, otherwise a buffer of *size* bytes. There is no guarantee that the buffer will be aligned to alignHint.
	 */
	void* FastAlloc(size_t size, size_t alignHint);

	/**
	 * \brief Frees a buffer allocated using FastAlloc.
	 * \param buffer May be null
	 */
	void FastFree(void* buffer);

	template<typename T, bool UseFastFree = false>
	struct InPlaceDeleter
	{
		void operator()(T* obj)
		{
			if (obj == nullptr)
			{
				return;
			}

			obj->~T();

			if constexpr (UseFastFree)
			{
				FastFree(obj);
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
		return std::unique_ptr<T, InPlaceDeleter<T, true>>{ new (FastAlloc(sizeof(T), alignof(T))) T(std::forward<Args>(args)...) };
	}
}
