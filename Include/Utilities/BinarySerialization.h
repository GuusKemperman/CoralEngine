#pragma once
#include "Utilities/view_istream.h"

namespace CE
{
	template<typename T>
	std::string ToBinary(const T& value)
	{
		std::stringstream ss{};
		
		{
			cereal::BinaryOutputArchive outArchive{ ss };
			outArchive(value);
		}

		return ss.str();
	}

	template<typename T>
	void FromBinary(std::string_view binaryString, T& out)
	{
		try
		{
			view_istream view{ binaryString };
			cereal::BinaryInputArchive inArchive{ view };
			inArchive(out);
		}
		catch ([[maybe_unused]] const std::exception& e)
		{
			LOG(LogCore, Warning, "Invalid value serialized - {}", e.what());
		}
	}

	template<typename T>
	T FromBinary(std::string_view readableString)
	{
		T tmp{};
		FromBinary(readableString, tmp);
		return tmp;
	}

	static constexpr Name sSerializeMemberFuncName = "SerializeAsMember";
	static constexpr Name sDeserializeMemberFuncName = "DeserializeAsMember";
}
