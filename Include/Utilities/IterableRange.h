#pragma once

namespace Engine
{
	template<typename It>
	class IterableRange
	{
	public:
		IterableRange(It&& begin, It&& end) :
			mBegin(std::move(begin)),
			mEnd(std::move(end)) {}

		const It& begin() const { return mBegin; }
		const It& end() const { return mEnd; }

	private:
		It mBegin;
		It mEnd;
	};

	// See EncapsulatingForwardIterator.
	// If you have a container of key-value pairs (or any other form of pairs),
	// this iterator will only expose the value.
	struct AlwaysSecondEncapsulator
	{
		template<typename It>
		static auto& DereferenceOperator(It it) { return it->second; }

		template<typename It>
		static auto& ArrowOperator(It it) { return it->second; }
	};

	// See EncapsulatingForwardIterator.
	// If you have a container of key-value pairs (or any other form of pairs),
	// this iterator will only expose the key.
	struct AlwaysFirstEncapsulator
	{
		template<typename It>
		static auto& DereferenceOperator(It it) { return it->first; }

		template<typename It>
		static auto& ArrowOperator(It it) { return it->first; }
	};

	// See EncapsulatingForwardIterator.
	// If you have a container of pointer-like types,
	// this encapsulator can be used to only expose references.
	// This let's your user know that you promise the value will never be nullptr.
	struct DereferencePointersEncapsulator
	{
		template<typename It>
		static auto& DereferenceOperator(It it) { return **it; }

		template<typename It>
		static auto& ArrowOperator(It it) { return **it; }
	};

	template<typename Encapsulator, typename UnderlyingIt>
	class EncapsulingForwardIterator
	{
	public:
		template<typename... Args>
		EncapsulingForwardIterator(Args&&... args) :
			mIt(std::forward<Args>(args)...)
		{
		}

		decltype(auto) operator*() const { return Encapsulator::DereferenceOperator(mIt); }
		decltype(auto) operator->() const { return Encapsulator::ArrowOperator(mIt); }

		EncapsulingForwardIterator& operator++() { ++mIt; return *this; }
		EncapsulingForwardIterator operator++(int) { EncapsulingForwardIterator tmp = *this; ++(*this); return tmp; }

		constexpr bool operator==(const EncapsulingForwardIterator& b) const { return mIt == b.mIt; }
		constexpr bool operator!=(const EncapsulingForwardIterator& b) const { return mIt != b.mIt; }

	private:
		UnderlyingIt mIt;
	};
}