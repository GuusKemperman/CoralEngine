#pragma once
#include <string>
#include <vector>

namespace CE
{
	class StringFunctions
	{
	public:
		// BinaryDest should be atleast hex.size/2.
		static void HexToBinary(const std::string_view hex, Span<char> binaryDest);

		static std::string HexToBinary(const std::string_view hex);

		// Make sure hexDest.size is atleast binary.size * 2
		static void BinaryToHex(const std::string_view binary, Span<char> hexDest);

		static std::string BinaryToHex(const std::string_view binary);

		static std::vector<std::string> SplitString(const std::string& stringToSplit, const std::string& splitOn);

		static std::string StreamToString(std::istream& stream);

		/// Replace all occurrences of the search string with the replacement string.
		///
		/// @param subject The string being searched and replaced on, otherwise known as the haystack.
		/// @param search The value being searched for, otherwise known as the needle.
		/// @param replace The replacement value that replaces found search values.
		/// @return a new string with all occurrences replaced.
		///
		static std::string StringReplace(const std::string& subject, const std::string& search, const std::string& replace);

		/// Determine whether or not a string ends with the given suffix. Does
		/// not create an internal copy.
		///
		/// @param subject The string being searched in.
		/// @param prefix The string to search for.
		/// @return a boolean indicating if the suffix was found.
		///
		static bool StringEndsWith(const std::string& subject, const std::string& suffix);

		/// Determine whether or not a string starts with the given prefix. Does
		/// not create an internal copy.
		///
		/// @param subject The string being searched in.
		/// @param prefix The string to search for.
		/// @return a boolean indicating if the prefix was found.
		///
		static bool StringStartsWith(const std::string& subject, const std::string& prefix);

		static bool AreStreamsEqual(std::istream& streamA, std::istream& streamB);

		// CreateUniqueName("Hello!") returns "Hello! (1)" if "Hello!" is not available,
		// CreateUniqueName("Hello! (1)") returns "Hello! (2)", etc.
		static std::string CreateUniqueName(std::string_view desiredName, const std::function<bool(std::string_view)>& isNameAvailable);
	};
}