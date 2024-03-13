#include "Precomp.h"
#include "Components/Animation/Bone.h"

Engine::Bone::Bone(const std::string_view name, const AnimData& animData)
{
	mName = name;

	mPositions = animData.mPositions;
	mRotations = animData.mRotations;
	mScales = animData.mScales;
}
