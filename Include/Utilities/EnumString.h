#pragma once

namespace CE
{
    // Specialize this struct to have enum-string conversions for your custom enum.
    template<typename EnumType>
    struct EnumStringPairsImpl
    {
    };

    template<typename EnumType>
    using EnumStringPair = std::pair<EnumType, std::string_view>;

    template<typename EnumType, size_t NumOfPairs>
    using EnumStringPairs = std::array<EnumStringPair<EnumType>, NumOfPairs>;

    namespace Internal
    {
        struct IsEnumReflectedHelper
        {
        private:
            template <class T>
            static auto test(int) -> decltype(EnumStringPairsImpl<T>::value, std::true_type());

            template <class> static std::false_type test(...);
        public:
            template<typename T>
            static constexpr bool value() { return std::is_same<decltype(test<T>(0)), std::true_type>::value; }
        };
    }

    template<typename EnumType>
    static constexpr bool sIsEnumReflected = Internal::IsEnumReflectedHelper::value<EnumType>();

    template<typename EnumType>
    static constexpr auto sEnumStringPairs = EnumStringPairsImpl<EnumType>::value;

    template<typename EnumType>
    static constexpr std::string_view EnumToString(const EnumType& value);

    //**************************//
	//		Implementation		//
	//**************************//

    template<typename EnumType>
	constexpr std::string_view EnumToString(const EnumType& value)
    {
        static_assert(sIsEnumReflected<EnumType>);

        for (const auto& [enumVal, name] : sEnumStringPairs<EnumType>)
        {
            if (enumVal == value)
            {
                return name;
            }
        }

        return "Unreflected value";
    }
}