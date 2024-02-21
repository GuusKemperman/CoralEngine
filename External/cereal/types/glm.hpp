#pragma once
#include "cereal/types/array.hpp"

namespace cereal
{
	template<class Archive, glm::length_t L, typename T>
	void save(Archive& ar, const glm::vec<L, T>& value)
	{
		save(ar, reinterpret_cast<const std::array<T, L>&>(value));
	}

	template<class Archive, glm::length_t L, typename T>
	void load(Archive& ar, glm::vec<L, T>& value)
	{
		load(ar, reinterpret_cast<std::array<T, L>&>(value));
	}

	template<class Archive, glm::length_t X, glm::length_t Y, typename T>
	void save(Archive& ar, const glm::mat<X, Y, T>& value)
	{
		save(ar, reinterpret_cast<const std::array<T, X * Y>&>(value));
	}

	template<class Archive, glm::length_t X, glm::length_t Y, typename T>
	void load(Archive& ar, glm::mat<X, Y, T>& value)
	{
		load(ar, reinterpret_cast<std::array<T, X * Y>&>(value));
	}

	template<class Archive>
	void save(Archive& ar, const glm::quat& value)
	{
		save(ar, reinterpret_cast<const std::array<float, 4>&>(value));
	}

	template<class Archive>
	void load(Archive& ar, glm::quat& value)
	{
		load(ar, reinterpret_cast<std::array<float, 4>&>(value));
	}
}
