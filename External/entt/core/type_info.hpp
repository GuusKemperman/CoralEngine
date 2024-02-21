#ifndef ENTT_CORE_TYPE_INFO_HPP
#define ENTT_CORE_TYPE_INFO_HPP

#include <string_view>
#include <type_traits>
#include <utility>
#include "../config/config.h"
#include "../core/attribute.h"
#include "fwd.hpp"
#include "hashed_string.hpp"

namespace entt {

/**
 * @cond TURN_OFF_DOXYGEN
 * Internal details not to be documented.
 */

namespace internal {

struct ENTT_API type_index final {
    [[nodiscard]] static id_type next() noexcept {
        static ENTT_MAYBE_ATOMIC(id_type) value{};
        return value++;
    }
};

template<typename Type>
[[nodiscard]] constexpr auto stripped_type_name() noexcept {
#if defined ENTT_PRETTY_FUNCTION
    std::string_view pretty_function{ENTT_PRETTY_FUNCTION};
    auto first = pretty_function.find_first_not_of(' ', pretty_function.find_first_of(ENTT_PRETTY_FUNCTION_PREFIX) + 1);
    auto value = pretty_function.substr(first, pretty_function.find_last_of(ENTT_PRETTY_FUNCTION_SUFFIX) - first);
    return value;
#else
    return std::string_view{""};
#endif
}

template<typename Type, auto = stripped_type_name<Type>().find_first_of('.')>
[[nodiscard]] static constexpr std::string_view type_name(int) noexcept {
    constexpr auto value = stripped_type_name<Type>();
    return value;
}

template<typename Type>
[[nodiscard]] static std::string_view type_name(char) noexcept {
    static const auto value = stripped_type_name<Type>();
    return value;
}

template<typename Type, auto = stripped_type_name<Type>().find_first_of('.')>
[[nodiscard]] static constexpr id_type type_hash(int) noexcept {
    constexpr auto stripped = stripped_type_name<Type>();
    constexpr auto value = hashed_string::value(stripped.data(), stripped.size());
    return value;
}

template<typename Type>
[[nodiscard]] static id_type type_hash(char) noexcept {
    static const auto value = [](const auto stripped) {
        return hashed_string::value(stripped.data(), stripped.size());
    }(stripped_type_name<Type>());
    return value;
}

} // namespace internal

/**
 * Internal details not to be documented.
 * @endcond
 */

/**
 * @brief Type sequential identifier.
 * @tparam Type Type for which to generate a sequential identifier.
 */
template<typename Type, typename = void>
struct ENTT_API type_index final {
    /**
     * @brief Returns the sequential identifier of a given type.
     * @return The sequential identifier of a given type.
     */
    [[nodiscard]] static id_type value() noexcept {
        static const id_type value = internal::type_index::next();
        return value;
    }

    /*! @copydoc value */
    [[nodiscard]] constexpr operator id_type() const noexcept {
        return value();
    }
};

/**
 * @brief Type hash.
 * @tparam Type Type for which to generate a hash value.
 */
template<typename Type, typename = void>
struct type_hash final {
    /**
     * @brief Returns the numeric representation of a given type.
     * @return The numeric representation of the given type.
     */
#if defined ENTT_PRETTY_FUNCTION
    [[nodiscard]] static constexpr id_type value() noexcept {
        return internal::type_hash<Type>(0);
#else
    [[nodiscard]] static constexpr id_type value() noexcept {
        return type_index<Type>::value();
#endif
    }

    /*! @copydoc value */
    [[nodiscard]] constexpr operator id_type() const noexcept {
        return value();
    }
};

/**
 * @brief Type name.
 * @tparam Type Type for which to generate a name.
 */
template<typename Type, typename = void>
struct type_name final {
    /**
     * @brief Returns the name of a given type.
     * @return The name of the given type.
     */
    [[nodiscard]] static constexpr std::string_view value() noexcept {
        return internal::type_name<Type>(0);
    }

    /*! @copydoc value */
    [[nodiscard]] constexpr operator std::string_view() const noexcept {
        return value();
    }
};

/*! @brief Implementation specific information about a type. */
struct type_info final {
    /**
     * @brief Constructs a type info object for a given type.
     * @tparam Type Type for which to construct a type info object.
     */
    template<typename Type>
    constexpr type_info(std::in_place_type_t<Type>) noexcept
        : seq{ type_index<std::remove_cv_t<std::remove_reference_t<Type>>>::value() },
        identifier{ type_hash<std::remove_cv_t<std::remove_reference_t<Type>>>::value() },
          mIsTriviallyDefaultConstructible(std::is_trivially_default_constructible_v<std::remove_cv_t<std::remove_reference_t<Type>>>),
	       mIsTriviallyMoveConstructible(std::is_trivially_move_constructible_v<std::remove_cv_t<std::remove_reference_t<Type>>>),
	       mIsTriviallyCopyConstructible(std::is_trivially_copy_constructible_v<std::remove_cv_t<std::remove_reference_t<Type>>>),
	       mIsTriviallyCopyAssignable(std::is_trivially_copy_assignable_v<std::remove_cv_t<std::remove_reference_t<Type>>>),
	       mIsTriviallyMoveAssignable(std::is_trivially_move_assignable_v<std::remove_cv_t<std::remove_reference_t<Type>>>),
	       mIsDefaultConstructible(std::is_default_constructible_v<std::remove_cv_t<std::remove_reference_t<Type>>>),
	       mIsMoveConstructible(std::is_move_constructible_v<std::remove_cv_t<std::remove_reference_t<Type>>>),
	       mIsCopyConstructible(std::is_copy_constructible_v<std::remove_cv_t<std::remove_reference_t<Type>>>),
	       mIsCopyAssignable(std::is_copy_assignable_v<std::remove_cv_t<std::remove_reference_t<Type>>>),
	       mIsMoveAssignable(std::is_move_assignable_v<std::remove_cv_t<std::remove_reference_t<Type>>>),
	       mIsTriviallyDestructible(std::is_trivially_destructible_v<std::remove_cv_t<std::remove_reference_t<Type>>>),
          alias{type_name<std::remove_cv_t<std::remove_reference_t<Type>>>::value()}
    {
        if constexpr (!std::is_same_v<void, Type>)
        {
            mSize = sizeof(Type);
            mAlign = alignof(Type);
        }
    }

    template<typename GuusMetaTypeInfo>
    constexpr type_info(id_type index, GuusMetaTypeInfo guusTypeInfo, std::string_view name) :
        seq(index),
        identifier(guusTypeInfo.mTypeId),
		mSize(guusTypeInfo.GetSize()),
		mAlign((guusTypeInfo.GetAlign())),
        mIsTriviallyDefaultConstructible(guusTypeInfo.mFlags & GuusMetaTypeInfo::IsTriviallyDefaultConstructible),
        mIsTriviallyMoveConstructible(guusTypeInfo.mFlags & GuusMetaTypeInfo::IsTriviallyMoveConstructible),
        mIsTriviallyCopyConstructible(guusTypeInfo.mFlags & GuusMetaTypeInfo::IsTriviallyCopyConstructible),
        mIsTriviallyCopyAssignable(guusTypeInfo.mFlags & GuusMetaTypeInfo::IsTriviallyCopyAssignable),
        mIsTriviallyMoveAssignable(guusTypeInfo.mFlags & GuusMetaTypeInfo::IsTriviallyMoveAssignable),
        mIsDefaultConstructible(guusTypeInfo.mFlags & GuusMetaTypeInfo::IsDefaultConstructible),
        mIsMoveConstructible(guusTypeInfo.mFlags & GuusMetaTypeInfo::IsMoveConstructible),
        mIsCopyConstructible(guusTypeInfo.mFlags & GuusMetaTypeInfo::IsCopyConstructible),
        mIsCopyAssignable(guusTypeInfo.mFlags & GuusMetaTypeInfo::IsCopyAssignable),
        mIsMoveAssignable(guusTypeInfo.mFlags & GuusMetaTypeInfo::IsMoveAssignable),
        mIsTriviallyDestructible(guusTypeInfo.mFlags & GuusMetaTypeInfo::IsTriviallyDestructible),
        alias(name)
    {
    };

    /**
     * @brief Type index.
     * @return Type index.
     */
    [[nodiscard]] constexpr id_type index() const noexcept {
        return seq;
    }

    /**
     * @brief Type hash.
     * @return Type hash.
     */
    [[nodiscard]] constexpr id_type hash() const noexcept {
        return identifier;
    }

    /**
     * @brief Type name.
     * @return Type name.
     */
    [[nodiscard]] constexpr std::string_view name() const noexcept {
        return alias;
    }

    template<typename T>
    constexpr T ToGuusTypeInfo() const noexcept
    {
        return T
        {
            identifier,
            mSize,
            mAlign,
            mIsTriviallyDefaultConstructible,
            mIsTriviallyMoveConstructible,
            mIsTriviallyCopyConstructible,
            mIsTriviallyCopyAssignable,
            mIsTriviallyMoveAssignable,
            mIsDefaultConstructible,
            mIsMoveConstructible,
            mIsCopyConstructible,
            mIsCopyAssignable,
            mIsMoveAssignable,
            mIsTriviallyDestructible
        };
    }

private:
    id_type seq;
    id_type identifier;
    id_type mSize{}/* : NUM_OF_BITS_FOR_SIZE*/;
    id_type mAlign{}/* : NUM_OF_BITS_FOR_ALIGN*/;
    bool mIsTriviallyDefaultConstructible/* : 1*/;
    bool mIsTriviallyMoveConstructible/* : 1*/;
    bool mIsTriviallyCopyConstructible/* : 1*/;
    bool mIsTriviallyCopyAssignable/* : 1*/;
    bool mIsTriviallyMoveAssignable/* : 1*/;
    bool mIsDefaultConstructible/* : 1*/;
    bool mIsMoveConstructible/* : 1*/;
    bool mIsCopyConstructible/* : 1*/;
    bool mIsCopyAssignable/* : 1*/;
    bool mIsMoveAssignable/* : 1*/;
    bool mIsTriviallyDestructible/* : 1*/;
    std::string_view alias;
};


#undef NUM_OF_BITS_FOR_SIZE
#undef NUM_OF_BITS_FOR_ALIGN

/**
 * @brief Compares the contents of two type info objects.
 * @param lhs A type info object.
 * @param rhs A type info object.
 * @return True if the two type info objects are identical, false otherwise.
 */
[[nodiscard]] inline constexpr bool operator==(const type_info &lhs, const type_info &rhs) noexcept {
    return lhs.hash() == rhs.hash();
}

/**
 * @brief Compares the contents of two type info objects.
 * @param lhs A type info object.
 * @param rhs A type info object.
 * @return True if the two type info objects differ, false otherwise.
 */
[[nodiscard]] inline constexpr bool operator!=(const type_info &lhs, const type_info &rhs) noexcept {
    return !(lhs == rhs);
}

/**
 * @brief Compares two type info objects.
 * @param lhs A valid type info object.
 * @param rhs A valid type info object.
 * @return True if the first element is less than the second, false otherwise.
 */
[[nodiscard]] constexpr bool operator<(const type_info &lhs, const type_info &rhs) noexcept {
    return lhs.index() < rhs.index();
}

/**
 * @brief Compares two type info objects.
 * @param lhs A valid type info object.
 * @param rhs A valid type info object.
 * @return True if the first element is less than or equal to the second, false
 * otherwise.
 */
[[nodiscard]] constexpr bool operator<=(const type_info &lhs, const type_info &rhs) noexcept {
    return !(rhs < lhs);
}

/**
 * @brief Compares two type info objects.
 * @param lhs A valid type info object.
 * @param rhs A valid type info object.
 * @return True if the first element is greater than the second, false
 * otherwise.
 */
[[nodiscard]] constexpr bool operator>(const type_info &lhs, const type_info &rhs) noexcept {
    return rhs < lhs;
}

/**
 * @brief Compares two type info objects.
 * @param lhs A valid type info object.
 * @param rhs A valid type info object.
 * @return True if the first element is greater than or equal to the second,
 * false otherwise.
 */
[[nodiscard]] constexpr bool operator>=(const type_info &lhs, const type_info &rhs) noexcept {
    return !(lhs < rhs);
}

/**
 * @brief Returns the type info object associated to a given type.
 *
 * The returned element refers to an object with static storage duration.<br/>
 * The type doesn't need to be a complete type. If the type is a reference, the
 * result refers to the referenced type. In all cases, top-level cv-qualifiers
 * are ignored.
 *
 * @tparam Type Type for which to generate a type info object.
 * @return A reference to a properly initialized type info object.
 */
template<typename Type>
[[nodiscard]] const type_info &type_id() noexcept {
    if constexpr(std::is_same_v<Type, std::remove_cv_t<std::remove_reference_t<Type>>>) {
        static type_info instance{std::in_place_type<Type>};
        return instance;
    } else {
        return type_id<std::remove_cv_t<std::remove_reference_t<Type>>>();
    }
}

/*! @copydoc type_id */
template<typename Type>
[[nodiscard]] const type_info &type_id(Type &&) noexcept {
    return type_id<std::remove_cv_t<std::remove_reference_t<Type>>>();
}

} // namespace entt

#endif
