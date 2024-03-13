#pragma once

namespace Engine
{
	struct KeyPosition
	{
		glm::vec3 position;
		double timeStamp;
	};

	struct KeyRotation
	{
		glm::quat orientation;
		double timeStamp;
	};

	struct KeyScale
	{
		glm::vec3 scale;
		double timeStamp;
	};

	struct AnimData
	{
		std::optional<std::vector<KeyPosition>> mPositions;
		std::optional<std::vector<KeyRotation>> mRotations;
		std::optional<std::vector<KeyScale>> mScales;
	};

	class Bone
	{
	public:
		Bone(const std::string_view name, const AnimData& animData);
		Bone(){}

		std::string mName{};

		std::optional<std::vector<KeyPosition>> mPositions{};
		std::optional<std::vector<KeyRotation>> mRotations{};
		std::optional<std::vector<KeyScale>> mScales{};
	private:
	};
}

#include <cereal/types/vector.hpp>
#include "cereal/archives/binary.hpp"

