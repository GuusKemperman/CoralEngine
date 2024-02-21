#include "cereal/types/tuple.hpp"

namespace cereal
{
	template<class Archive, typename T1, typename T2>
	void save(Archive& ar, const std::pair<T1, T2>& value)
	{
		ar(value.first, value.second);
	}

	template<class Archive, typename T1, typename T2>
	void load(Archive& ar, std::pair<T1, T2>& value)
	{
		ar(value.first, value.second);
	}
}