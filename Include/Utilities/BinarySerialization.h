#pragma once
#include "Containers/view_istream.h"

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 5054)
#endif

#include "cereal/cereal.hpp"
#include "cereal/types/pair.hpp"
#include "cereal/types/glm.hpp"
#include "cereal/types/vector.hpp"
#include "cereal/types/array.hpp"
#include "cereal/types/atomic.hpp"
#include "cereal/types/bitset.hpp"
#include "cereal/types/chrono.hpp"
#include "cereal/types/deque.hpp"
#include "cereal/types/forward_list.hpp"
#include "cereal/types/functional.hpp"
#include "cereal/types/list.hpp"
#include "cereal/types/unordered_set.hpp"
#include "cereal/types/map.hpp"
#include "cereal/types/unordered_map.hpp"
#include "cereal/types/optional.hpp"
#include "cereal/types/queue.hpp"
#include "cereal/types/set.hpp"
#include "cereal/types/stack.hpp"
#include "cereal/types/string.hpp"
#include "cereal/types/valarray.hpp"
#include "cereal/archives/binary.hpp"

#ifdef _MSC_VER
#pragma warning(pop)
#endif

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
		catch (cereal::Exception)
		{
			LOG(LogCore, Warning, "Invalid value serialized");
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
