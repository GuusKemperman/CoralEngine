#pragma once
#include <string>
#include <vector>

namespace Engine
{
	class StringFunctions
	{
	public:
		// BinaryDest should be atleast hex.size/2.
		static void HexToBinary(const std::string_view hex, Span<char> binaryDest)
		{
			constexpr int nibbles[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 0, 0, 0, 0, 0, 0, 10, 11, 12, 13, 14, 15 };

			const size_t binarySize = hex.size() >> 1;
			ASSERT(binaryDest.size_bytes() >= binarySize);

			for (size_t i = 0; i < binarySize; i++)
			{
				const size_t doubleIndex = i << 1;
				const int hex1 = static_cast<int>(hex[doubleIndex]);
				const int hex2 = static_cast<int>(hex[doubleIndex + 1]);

#ifdef ASSERTS_ENABLED
				ASSERT_LOG(std::isxdigit(hex1) && std::isxdigit(hex2), "\'Hex\' string contained none-hex characters");
				ASSERT_LOG((std::isdigit(hex1) || std::isupper(hex1)) && (std::isdigit(hex2) || std::isupper(hex2)), "Hex contained lower characters, make the input string upper or make a seperate hextobinary func that accounts for this");
#endif // ASSERTS_ENABLED

				constexpr int zeroChar = static_cast<int>('0');
				binaryDest[i] = static_cast<char>(((nibbles[hex1 - zeroChar]) << 4) + nibbles[hex2 - zeroChar]);
			}
		}

		static std::string HexToBinary(const std::string_view hex)
		{
			std::string binary{""};
			const size_t binarySize = hex.size() >> 1;
			binary.resize(binarySize);

#ifdef ASSERTS_ENABLED
			ASSERT_LOG((hex.size() & 1) == 0, "Hex size was not even, hex must be invalid.");
#endif // ASSERTS_ENABLED

			HexToBinary(hex, { binary.data(), binarySize });

			return binary;
		}

		// Make sure hexDest.size is atleast binary.size * 2
		static void BinaryToHex(const std::string_view binary, Span<char> hexDest)
		{
			ASSERT(hexDest.size_bytes() >= binary.size() * 2);
			constexpr char syms[] = "0123456789ABCDEF";

			for (size_t i = 0; i < binary.size(); i++)
			{
				const char ch = binary[i];
				const size_t doubleIndex = i << 1;
				hexDest[doubleIndex] = syms[((ch >> 4) & 0xf)];
				hexDest[doubleIndex + 1] = syms[ch & 0xf];
			}
		}

		static std::string BinaryToHex(const std::string_view binary)
		{
			std::string hex{ "" };
			const size_t hexSize = binary.size() << 1;
			hex.resize(hexSize);
			BinaryToHex(binary, { hex.data(), hexSize });
			return hex;
		}

		static std::vector<std::string> SplitString(const std::string& stringToSplit, const std::string& splitOn)
		{
			std::string copy = stringToSplit;
			size_t pos = 0;
			std::vector<std::string> returnValue{};

			while ((pos = copy.find(splitOn)) != std::string::npos)
			{
				returnValue.emplace_back(copy.substr(0, pos));
				copy.erase(0, pos + splitOn.length());
			}
			returnValue.push_back(std::move(copy));
			return returnValue;
		}

		static std::string StreamToString(std::istream& stream)
		{
			// https://stackoverflow.com/a/4976611
			std::string ret;
			char mBuffers[4096];
			while (stream.read(mBuffers, sizeof(mBuffers)))
			{
				ret.append(mBuffers, sizeof(mBuffers));
			}
			ret.append(mBuffers, stream.gcount());
			return ret;
		}

		/// Replace all occurrences of the search string with the replacement string.
		///
		/// @param subject The string being searched and replaced on, otherwise known as the haystack.
		/// @param search The value being searched for, otherwise known as the needle.
		/// @param replace The replacement value that replaces found search values.
		/// @return a new string with all occurrences replaced.
		///
		static std::string StringReplace(const std::string& subject, const std::string& search, const std::string& replace)
		{
			std::string result(subject);
			size_t pos = 0;

			while ((pos = subject.find(search, pos)) != std::string::npos)
			{
				result.replace(pos, search.length(), replace);
				pos += search.length();
			}

			return result;
		}

		/// Determine whether or not a string ends with the given suffix. Does
		/// not create an internal copy.
		///
		/// @param subject The string being searched in.
		/// @param prefix The string to search for.
		/// @return a boolean indicating if the suffix was found.
		///
		static bool StringEndsWith(const std::string& subject, const std::string& suffix)
		{
			// Early out test:
			if (suffix.length() > subject.length()) return false;

			// Resort to difficult to read C++ logic:
			return subject.compare(subject.length() - suffix.length(), suffix.length(), suffix) == 0;
		}

		/// Determine whether or not a string starts with the given prefix. Does
		/// not create an internal copy.
		///
		/// @param subject The string being searched in.
		/// @param prefix The string to search for.
		/// @return a boolean indicating if the prefix was found.
		///
		static bool StringStartsWith(const std::string& subject, const std::string& prefix)
		{
			// Early out, prefix is longer than the subject:
			if (prefix.length() > subject.length()) return false;

			// Compare per character:
			for (size_t i = 0; i < prefix.length(); ++i)
				if (subject[i] != prefix[i]) return false;

			return true;
		}

		static bool AreStreamsEqual(std::istream& streamA, std::istream& streamB)
		{
			static constexpr std::streamsize blockSize = 512;
			char blockA[blockSize];
			char blockB[blockSize];

			while (1)
			{
				streamA.read(blockA, blockSize);
				streamB.read(blockB, blockSize);

				auto numRead = streamA.gcount();

				if (numRead != streamB.gcount())
				{
					return false;
				}

				if (memcmp(blockA, blockB, numRead) != 0)
				{
					return false;
				}

				if (numRead != blockSize)
				{
					return true;
				}
			}
		}
	};
}