#include "Precomp.h"
#include "Assets/Animation/Bone.h"

Engine::Bone::Bone(const std::string_view name, const AnimData& animData)
{
	mName = name;

	mPositions = animData.mPositions;
	mRotations = animData.mRotations;
	mScales = animData.mScales;
}

glm::mat4x4 Engine::Bone::GetInterpolatedTransform(float timeStamp) const
{
	return InterpolatePosition(timeStamp) * InterpolateRotation(timeStamp) * InterpolateScale(timeStamp);
}

int Engine::Bone::GetPositionIndex(float timeStamp) const
{
	unsigned int numPositions = static_cast<unsigned int>(mPositions.size());

	for (unsigned int i = 0; i < numPositions - 1; i++)
	{
		if (timeStamp < mPositions.at(i + 1).timeStamp)
		{
			return i;
		}
	}
	return 0;
}

int Engine::Bone::GetRotationIndex(float timeStamp) const
{
	unsigned int numRotations = static_cast<unsigned int>(mRotations.size());

	for (unsigned int i = 0; i < numRotations - 1; i++)
	{
		if (timeStamp < mRotations.at(i + 1).timeStamp)
		{
			return i;
		}
	}
	return 0;
}

int Engine::Bone::GetScaleIndex(float timeStamp) const
{
	unsigned int numScalings = static_cast<unsigned int>(mScales.size());

	for (unsigned int i = 0; i < numScalings - 1; i++)
	{
		if (timeStamp < mScales.at(i + 1).timeStamp)
		{
			return i;
		}
	}
	return 0;
}

float Engine::Bone::GetScaleFactor(float lastTimeStamp, float nextTimeStamp, float currentTimeStamp) const
{
	float midWayLength = currentTimeStamp - lastTimeStamp;
	float framesDiff = nextTimeStamp - lastTimeStamp;
	return midWayLength / framesDiff;
}

glm::mat4x4 Engine::Bone::InterpolatePosition(float timeStamp) const
{
	if (mPositions.size() == 1)
	{
		return glm::translate(glm::mat4x4(1.0f), mPositions.at(0).position);
	}

	int p0Index = GetPositionIndex(timeStamp);
	int p1Index = p0Index + 1;

	float scaleFactor = GetScaleFactor(mPositions.at(p0Index).timeStamp, mPositions.at(p1Index).timeStamp, timeStamp);

	glm::vec3 finalPosition = glm::mix(mPositions.at(p0Index).position, mPositions.at(p1Index).position, scaleFactor);

	return glm::translate(glm::mat4x4(1.0f),finalPosition);
}

glm::mat4x4 Engine::Bone::InterpolateRotation(float timeStamp) const
{
	if (mRotations.size() == 1)
	{
		return glm::toMat4(glm::normalize(mRotations.at(0).orientation));
	}

	int p0Index = GetRotationIndex(timeStamp);
	int p1Index = p0Index + 1;

	float scaleFactor = GetScaleFactor(mRotations.at(p0Index).timeStamp, mRotations.at(p1Index).timeStamp, timeStamp);

	glm::quat finalRotation = glm::slerp(mRotations.at(p0Index).orientation, mRotations.at(p1Index).orientation, scaleFactor);

	return glm::toMat4(glm::normalize(finalRotation));
}

glm::mat4x4 Engine::Bone::InterpolateScale(float timeStamp) const
{
	if (mScales.size() == 1)
	{
		return glm::scale(glm::mat4x4(1.0f), mScales.at(0).scale);
	}
	int p0Index = GetScaleIndex(timeStamp);
	int p1Index = p0Index + 1;

	float scaleFactor = GetScaleFactor(mScales.at(p0Index).timeStamp, mScales.at(p1Index).timeStamp, timeStamp);

	glm::vec3 finalScale = glm::mix(mScales.at(p0Index).scale, mScales.at(p1Index).scale, scaleFactor);

	return glm::scale(glm::mat4x4(1.0f), finalScale);
}
