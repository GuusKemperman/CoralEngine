#include "Precomp.h"
#include "Utilities/StringFunctions.h"

void CE::StringFunctions::HexToBinary(const std::string_view hex, Span<char> binaryDest)
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

std::string CE::StringFunctions::HexToBinary(const std::string_view hex)
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

void CE::StringFunctions::BinaryToHex(const std::string_view binary, Span<char> hexDest)
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

std::string CE::StringFunctions::BinaryToHex(const std::string_view binary)
{
	std::string hex{ "" };
	const size_t hexSize = binary.size() << 1;
	hex.resize(hexSize);
	BinaryToHex(binary, { hex.data(), hexSize });
	return hex;
}

std::vector<std::string_view> CE::StringFunctions::SplitString(std::string_view stringToSplit, std::string_view splitOn)
{
	size_t lastPos{};
	const size_t length = stringToSplit.length();

	std::vector<std::string_view> tokens{};

	while (lastPos < length + 1)
	{
		size_t pos = stringToSplit.find_first_of(splitOn, lastPos);
		if (pos == std::string::npos)
		{
			pos = length;
		}

		tokens.emplace_back(stringToSplit.data() + lastPos, pos - lastPos);
		lastPos = pos + 1;
	}

	return tokens;
}

std::string CE::StringFunctions::StreamToString(std::istream& stream)
{
	// https://stackoverflow.com/a/4976611
	std::string ret;
	char buffers[4096];
	while (stream.read(buffers, sizeof(buffers)))
	{
		ret.append(buffers, sizeof(buffers));
	}
	ret.append(buffers, stream.gcount());
	return ret;
}

std::string CE::StringFunctions::StringReplace(std::string_view subject, std::string_view search,
	std::string_view replace)
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

bool CE::StringFunctions::StringEndsWith(std::string_view subject, std::string_view suffix)
{
	// Early out test:
	if (suffix.length() > subject.length()) return false;

	// Resort to difficult to read C++ logic:
	return subject.compare(subject.length() - suffix.length(), suffix.length(), suffix) == 0;
}

bool CE::StringFunctions::StringStartsWith(std::string_view subject, std::string_view prefix)
{
	// Early out, prefix is longer than the subject:
	if (prefix.length() > subject.length()) return false;

	// Compare per character:
	for (size_t i = 0; i < prefix.length(); ++i)
		if (subject[i] != prefix[i]) return false;

	return true;
}

bool CE::StringFunctions::AreStreamsEqual(std::istream& streamA, std::istream& streamB)
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

std::string CE::StringFunctions::CreateUniqueName(std::string_view desiredName, const std::function<bool(std::string_view)>& isNameAvailable)
{
	if (isNameAvailable(desiredName))
	{
		return std::string{ desiredName };
	}

	const size_t firstOpeningBracket = desiredName.find('(');

	if (firstOpeningBracket == std::string::npos)
	{
		std::string availName = std::string{ desiredName }.append(" (1)");
		return isNameAvailable(availName) ? availName : CreateUniqueName(availName, isNameAvailable);
	}

	size_t lastOpeningBracket = desiredName.find(')', firstOpeningBracket);

	if (lastOpeningBracket == std::string::npos)
	{
		lastOpeningBracket = desiredName.size() - 1;
	}

	const std::string_view justNumber = desiredName.substr(firstOpeningBracket + 1, lastOpeningBracket - firstOpeningBracket - 1);
	int prevNumber = std::stoi(justNumber.data());
	std::string availName = std::string{ desiredName.substr(0, firstOpeningBracket) };
	availName.append(Format("({})", ++prevNumber));
	return isNameAvailable(availName) ? availName : CreateUniqueName(availName, isNameAvailable);
}
