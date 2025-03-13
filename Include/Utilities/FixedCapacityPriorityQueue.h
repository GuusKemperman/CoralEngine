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
			for (auto it = begin(), otherIt = other.begin(); it != end(); ++it, ++otherIt)
			{
				new(&*it)T(*otherIt);
			}
		}

		FixedCapacityPriorityQueue(FixedCapacityPriorityQueue&& other) noexcept :
			mSize(other.mSize),
			mComparer(std::move(other.mComparer))
		{
			for (auto it = begin(), otherIt = other.begin(); it != end(); ++it, ++otherIt)
			{
				new(&*it)T(std::move(*otherIt));
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

			for (auto it = begin(), otherIt = other.begin(); it != end(); ++it, ++otherIt)
			{
				new(&*it)T(*otherIt);
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

			for (auto it = begin(), otherIt = other.begin(); it != end(); ++it, ++otherIt)
			{
				new(&*it)T(*otherIt);
			}

			other.clear();

			return *this;
		}

		~FixedCapacityPriorityQueue()
		{
			clear();
		}

		SizeType size() const
		{
			return mSize;
		}

		bool empty() const
		{
			return mSize == 0;
		}

		bool full() const
		{
			return mSize == Capacity;
		}

		const T& top() const
		{
			ASSERT(!empty());
			return *begin();
		}

		T& top()
		{
			ASSERT(!empty());
			return *begin();
		}

		void push(const T& v)
		{
			emplace(v);
		}

		void push(T&& v)
		{
			emplace(std::move(v));
		}

		template<typename... Args>
		void emplace(Args&&... args)
		{
			if (full())
			{
				throw std::out_of_range{ "Queue has reached max capacity" };
			}
			new (&*end())T(std::forward<Args>(args)...);
			++mSize;
			std::push_heap(begin(), end(), mComparer);
		}

		void pop()
		{
			ASSERT(!empty());
			std::pop_heap(begin(), end(), mComparer);
			--mSize;
			end()->~T();
		}

		void clear()
		{
			for (T& value : *this)
			{
				value.~T();
			}
			mSize = 0;
		}

	private:
		auto begin()
		{
			return reinterpret_cast<T*>(mData.data());
		}

		auto end()
		{
			auto it = begin();
			std::advance(it, mSize);
			return it;
		}

		std::array<char, Capacity * sizeof(T)> mData;
		SizeType mSize{};
		Comparer mComparer{};
	};
}