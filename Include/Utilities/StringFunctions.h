#pragma once
#include <string>
#include <vector>

namespace CE
{
	class StringFunctions
	{
	public:
		// BinaryDest should be atleast hex.size/2.
		static void HexToBinary(std::string_view hex, std::span<char> binaryDest);

		static std::string HexToBinary(std::string_view hex);

		// Make sure hexDest.size is atleast binary.size * 2
		static void BinaryToHex(std::string_view binary, std::span<char> hexDest);

		static std::string BinaryToHex(std::string_view binary);

		static std::vector<std::string_view> SplitString(std::string_view stringToSplit, std::string_view splitOn);

		static std::string StreamToString(std::istream& stream);

		[[nodiscard]] static std::string StringReplace(std::string_view subject, std::string_view search, std::string_view replace);

		static bool StringEndsWith(std::string_view subject, std::string_view suffix);

		static bool StringStartsWith(std::string_view subject, std::string_view prefix);

		static bool AreStreamsEqual(std::istream& streamA, std::istream& streamB);

		// CreateUniqueName("Hello!") returns "Hello! (1)" if "Hello!" is not available,
		// CreateUniqueName("Hello! (1)") returns "Hello! (2)", etc.
		static std::string CreateUniqueName(std::string_view desiredName, const std::function<bool(std::string_view)>& isNameAvailable);
	};
}