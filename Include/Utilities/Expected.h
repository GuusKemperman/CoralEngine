#pragma once
#include <variant>

template<typename ValidType, typename ErrorType>
class Expected
{
public:
	template<typename... T>
	constexpr Expected(T&&... args) :
		mData(std::forward<T>(args)...) {}

	constexpr bool HasValue() const { return mData.index() == 0; }
	constexpr bool HasError() const { return mData.index() == 1; }

	constexpr ValidType& GetValue() { return std::get<ValidType>(mData); }
	constexpr const ValidType& GetValue() const { return std::get<ValidType>(mData); }

	constexpr ErrorType& GetError() { return std::get<ErrorType>(mData); }
	constexpr const ErrorType& GetError() const { return std::get<ErrorType>(mData); }

private:
	std::variant<ValidType, ErrorType> mData{};
};