#include "Precomp.h"
#include "Assets/Animation/Bone.h"

CE::Bone::Bone(const std::string_view name, const AnimData& animData)
{
	mName = name;

	mPositions = animData.mPositions;
	mRotations = animData.mRotations;
	mScales = animData.mScales;
}

glm::mat4x4 CE::Bone::GetInterpolatedTransform(float timeStamp) const
{
	return InterpolatePositionMatrix(timeStamp) * InterpolateRotationMatrix(timeStamp) * InterpolateScaleMatrix(timeStamp);
}

int CE::Bone::GetPositionIndex(float timeStamp) const
{
	unsigned int numPositions = static_cast<unsigned int>(mPositions.size());

	for (unsigned int i = 0; i < numPositions - 1; i++)
	{
		if (timeStamp <= mPositions[i + 1].timeStamp)
		{
			return i;
		}
	}
	return 0;
}

int CE::Bone::GetRotationIndex(float timeStamp) const
{
	unsigned int numRotations = static_cast<unsigned int>(mRotations.size());

	for (unsigned int i = 0; i < numRotations - 1; i++)
	{
		if (timeStamp <= mRotations[i + 1].timeStamp)
		{
			return i;
		}
	}
	return 0;
}

int CE::Bone::GetScaleIndex(float timeStamp) const
{
	unsigned int numScalings = static_cast<unsigned int>(mScales.size());

	for (unsigned int i = 0; i < numScalings - 1; i++)
	{
		if (timeStamp <= mScales[i + 1].timeStamp)
		{
			return i;
		}
	}
	return 0;
}

float CE::Bone::GetScaleFactor(float lastTimeStamp, float nextTimeStamp, float currentTimeStamp) const
{
	float midWayLength = currentTimeStamp - lastTimeStamp;
	float framesDiff = nextTimeStamp - lastTimeStamp;
	return midWayLength / framesDiff;
}

glm::vec3 CE::Bone::InterpolatePosition(float timeStamp) const
{
	if (mPositions.size() == 1)
	{
		return mPositions[0].position;
	}

	int p0Index = GetPositionIndex(timeStamp);
	int p1Index = p0Index + 1;

	float scaleFactor = GetScaleFactor(mPositions[p0Index].timeStamp, mPositions[p1Index].timeStamp, timeStamp);

	glm::vec3 finalPosition = glm::mix(mPositions[p0Index].position, mPositions[p1Index].position, scaleFactor);

	return finalPosition;
}

glm::quat CE::Bone::InterpolateRotation(float timeStamp) const
{
	if (mRotations.size() == 1)
	{
		return glm::normalize(mRotations[0].orientation);
	}

	int p0Index = GetRotationIndex(timeStamp);
	int p1Index = p0Index + 1;

	float scaleFactor = GetScaleFactor(mRotations[p0Index].timeStamp, mRotations[p1Index].timeStamp, timeStamp);

	glm::quat finalRotation = glm::slerp(mRotations[p0Index].orientation, mRotations[p1Index].orientation, scaleFactor);

	return glm::normalize(finalRotation);
}

glm::vec3 CE::Bone::InterpolateScale(float timeStamp) const
{
	if (mScales.size() == 1)
	{
		return mScales[0].scale;
	}
	int p0Index = GetScaleIndex(timeStamp);
	int p1Index = p0Index + 1;

	float scaleFactor = GetScaleFactor(mScales[p0Index].timeStamp, mScales[p1Index].timeStamp, timeStamp);

	glm::vec3 finalScale = glm::mix(mScales[p0Index].scale, mScales[p1Index].scale, scaleFactor);

	return finalScale;
}

glm::mat4x4 CE::Bone::InterpolatePositionMatrix(float timeStamp) const
{
	return glm::translate(glm::mat4x4(1.0f), InterpolatePosition(timeStamp));
}

glm::mat4x4 CE::Bone::InterpolateRotationMatrix(float timeStamp) const
{
	return toMat4(InterpolateRotation(timeStamp));
}

glm::mat4x4 CE::Bone::InterpolateScaleMatrix(float timeStamp) const
{
	return glm::scale(glm::mat4x4(1.0f), InterpolateScale(timeStamp));
}
