#pragma once

namespace Engine
{
	struct KeyPosition
	{
		glm::vec3 position;
		float timeStamp;
	};

	struct KeyRotation
	{
		glm::quat orientation;
		float timeStamp;
	};

	struct KeyScale
	{
		glm::vec3 scale;
		float timeStamp;
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

		glm::mat4x4 GetInterpolatedTransform(float timeStamp) const;

		std::string mName{};

		std::optional<std::vector<KeyPosition>> mPositions{};
		std::optional<std::vector<KeyRotation>> mRotations{};
		std::optional<std::vector<KeyScale>> mScales{};

		int GetPositionIndex(float timeStamp) const;
		int GetRotationIndex(float timeStamp) const;
		int GetScaleIndex(float timeStamp) const;

	private:
		float GetScaleFactor(float lastTimeStamp, float nextTimeStamp, float currentTimeStamp) const;
		
		glm::mat4x4 InterpolatePosition(float timeStamp) const;
		glm::mat4x4 InterpolateRotation(float timeStamp) const;
		glm::mat4x4 InterpolateScale(float timeStamp) const;
	};
}

#include <cereal/types/vector.hpp>
#include "cereal/archives/binary.hpp"

