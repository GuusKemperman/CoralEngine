#pragma once
#include <string>

namespace Engine
{
	// Essentially a string_view with it's hash precalculated, at compile time if possible
	class Name
	{
#ifdef _DEBUG
		#define INTERNAL_NAME_CHECK_EXPIRATION if (IsOriginalStringExpired()) { LOG(LogCore, Warning, "Original string has been deallocated, may lead to unexpected behaviour"); }
#else 
		#define INTERNAL_NAME_CHECK_EXPIRATION
#endif

	public:
		using HashType = uint32;
		using SizeType = uint32;

		constexpr Name() = default;

		constexpr Name(const char* str, SizeType strLength, HashType hashValue) :
			mData(str),
			mSize(strLength),
			mHash(hashValue)
		{
			assert(HashString(mData, mSize) == mHash);
		}

		constexpr Name(const char* str) :
			Name(Helper::MakeName(str))
		{}

		constexpr Name(const char* str, SizeType strLength) :
			Name(Helper::MakeName(str, strLength))
		{}

		constexpr Name(const std::string_view strView) :
			Name(Helper::MakeName(strView.data(), static_cast<SizeType>(strView.size())))
		{}

		constexpr Name(const std::string& str) :
			Name(Helper::MakeName(str.c_str(), static_cast<SizeType>(str.size())))
		{}

		constexpr Name(const HashType hash) :
			mHash(hash)
		{}

		constexpr bool operator==(const Name& other) const { return mHash == other.mHash; }
		constexpr bool operator<(const Name& other) const { return mHash < other.mHash; }

		constexpr bool IsOriginalStringExpired() const
		{
			return mHash != HashString(mData, mSize);
		}

		constexpr HashType GetHash() const { return mHash; }

		constexpr const char* CString() const
		{
			INTERNAL_NAME_CHECK_EXPIRATION
			return mData;
		}
		constexpr std::string_view StringView() const
		{
			INTERNAL_NAME_CHECK_EXPIRATION
			return std::string_view{ mData, mSize };
		}

		std::string String() const
		{
			INTERNAL_NAME_CHECK_EXPIRATION
			return std::string{ mData, mSize };
		}

		static constexpr HashType HashString(const char* str)
		{
			HashType hash = 2166136261;

			for (SizeType i = 0; str[i]; i++)
			{
				hash = (hash ^ static_cast<HashType>(str[i])) * 16777619;
			}

			return hash;
		}

		static constexpr HashType HashString(const char* str, const SizeType len)
		{
			HashType hash = 2166136261;

			for (SizeType i = 0; i < len; ++i)
			{
				hash = (hash ^ static_cast<HashType>(str[i])) * 16777619;
			}

			return hash;
		}

		static constexpr HashType HashString(const std::string_view str)
		{
			return HashString(str.data(), static_cast<SizeType>(str.size()));
		}

	private:
		struct Helper
		{
			// Slightly faster as it initialized the name length as well
			static constexpr Name MakeName(const char* str)
			{
				Name name{ str, 0, 2166136261 };

				for (; str[name.mSize]; ++name.mSize)
				{
					name.mHash = (name.mHash ^ static_cast<HashType>(str[name.mSize])) * 16777619;
				}

				return name;
				
			}

			static constexpr Name MakeName(const char* str, const SizeType len)
			{
				const Name name{ str, len, HashString(str, len)};

				return name;
			}
		};

		friend Helper;
		const char* mData{};
		SizeType mSize{};
		HashType mHash{};
	};
}

template <>
struct std::hash<Engine::Name>
{
	std::size_t operator()(const Engine::Name& k) const
	{
		return k.GetHash();
	}
};

[[nodiscard]] constexpr Engine::Name operator"" _Name(const char* str, std::size_t length) noexcept
{
	return Engine::Name{ str, static_cast<Engine::Name::SizeType>(length) };
}
