#pragma once
#include <array>
#include <algorithm>

namespace CE
{
	template<typename T,
		size_t Capacity,
		class Comparer = std::less<T>,
		typename SizeType = size_t>
	class FixedCapacityPriorityQueue
	{
	public:
		FixedCapacityPriorityQueue() = default;

		FixedCapacityPriorityQueue(const FixedCapacityPriorityQueue& other) :
			mSize(other.mSize),
			mComparer(other.mComparer)
		{
			for (size_t i = 0; i < mSize; i++)
			{
				new(&mData[i])T(other.mData[i]);
			}
		}

		FixedCapacityPriorityQueue(FixedCapacityPriorityQueue&& other) noexcept :
			mSize(other.mSize),
			mComparer(std::move(other.mComparer))
		{
			for (size_t i = 0; i < mSize; i++)
			{
				new(&mData[i])T(std::move(other.mData[i]));
			}
		}

		FixedCapacityPriorityQueue& operator=(const FixedCapacityPriorityQueue& other)
		{
			if (&other == this)
			{
				return *this;
			}

			clear();

			mSize = other.mSize;
			mComparer = other.mComparer;

			for (size_t i = 0; i < mSize; i++)
			{
				mData[i] = other.mData[i];
			}

			return *this;
		}

		FixedCapacityPriorityQueue& operator=(FixedCapacityPriorityQueue&& other) noexcept
		{
			if (&other == this)
			{
				return *this;
			}

			clear();

			mSize = other.mSize;
			mComparer = std::move(other.mComparer);

			for (size_t i = 0; i < mSize; i++)
			{
				mData[i] = std::move(other.mData[i]);
			}

			return *this;
		}

		~FixedCapacityPriorityQueue()
		{
			clear();
		}

		bool empty() const
		{
			return mSize == 0;
		}

		const T& top() const
		{
			ASSERT(!empty());
			return mData[0];
		}

		T& top()
		{
			ASSERT(!empty());
			return mData[0];
		}

		void push(const T& v)
		{
			ASSERT(mSize < Capacity);
			new (&mData[mSize++])T(v);
			std::push_heap(begin(), end(), mComparer);
		}

		void pop()
		{
			ASSERT(!empty());
			std::pop_heap(begin(), end(), mComparer);
			mData[--mSize].~T();
		}

		void clear()
		{
			for (T& value : *this)
			{
				value.~T();
			}
		}

	private:
		auto begin()
		{
			return mData.begin();
		}

		auto end()
		{
			auto it = mData.begin();
			std::advance(it, mSize);
			return it;
		}

		std::array<T, Capacity> mData;
		SizeType mSize{};
		Comparer mComparer{};
	};
}