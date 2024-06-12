#include "Precomp.h"
#include "Components/EnvironmentGeneratorComponent.h"

#include "Meta/ReflectedTypes/STD/ReflectVector.h"
#include "Meta/ReflectedTypes/STD/ReflectOptional.h"
#include "Utilities/Reflect/ReflectComponentType.h"
#include "Assets/Prefabs/Prefab.h"

#ifdef EDITOR
void Game::EnvironmentGeneratorComponent::Layer::Object::DisplayWidget(const std::string&)
{
	CE::ShowInspectUI("mPrefab", mPrefab);
	CE::ShowInspectUI("mFrequency", mFrequency);
}
#endif // EDITOR

bool Game::EnvironmentGeneratorComponent::Layer::Object::operator==(const Object& other) const
{
	return mPrefab == other.mPrefab
		&& mFrequency == other.mFrequency;
}

bool Game::EnvironmentGeneratorComponent::Layer::Object::operator!=(const Object& other) const
{
	return !(*this == other);
}

CE::MetaType Game::EnvironmentGeneratorComponent::Layer::Object::Reflect()
{
	CE::MetaType type = CE::MetaType{ CE::MetaType::T<Object>{}, "Object" };
	CE::MetaProps& props = type.GetProperties();
	props.Add(CE::Props::sIsScriptableTag).Add(CE::Props::sIsScriptOwnableTag);

	type.AddField(&Object::mPrefab, "mPrefab").GetProperties().Add(CE::Props::sIsScriptableTag);
	type.AddField(&Object::mFrequency, "mFrequency").GetProperties().Add(CE::Props::sIsScriptableTag);

	CE::ReflectFieldType<Object>(type);
	return type;
}

#ifdef EDITOR
void Game::EnvironmentGeneratorComponent::Layer::NoiseInfluence::DisplayWidget(const std::string&)
{
	CE::ShowInspectUI("mIndexOfOtherLayer", mIndexOfOtherLayer);
	CE::ShowInspectUI("mWeight", mWeight);
}
#endif // EDITOR

bool Game::EnvironmentGeneratorComponent::Layer::NoiseInfluence::operator==(const NoiseInfluence& other) const
{
	return mWeight == other.mWeight
		&& mIndexOfOtherLayer == other.mIndexOfOtherLayer;
}

bool Game::EnvironmentGeneratorComponent::Layer::NoiseInfluence::operator!=(const NoiseInfluence& other) const
{
	return !(*this == other);
}

CE::MetaType Game::EnvironmentGeneratorComponent::Layer::NoiseInfluence::Reflect()
{
	CE::MetaType type = CE::MetaType{ CE::MetaType::T<NoiseInfluence>{}, "NoiseInfluence" };
	CE::MetaProps& props = type.GetProperties();
	props.Add(CE::Props::sIsScriptableTag).Add(CE::Props::sIsScriptOwnableTag);

	type.AddField(&NoiseInfluence::mIndexOfOtherLayer, "mIndexOfOtherLayer").GetProperties().Add(CE::Props::sIsScriptableTag);
	type.AddField(&NoiseInfluence::mWeight, "mWeight").GetProperties().Add(CE::Props::sIsScriptableTag);

	CE::ReflectFieldType<NoiseInfluence>(type);
	return type;
}

#ifdef EDITOR
void Game::EnvironmentGeneratorComponent::Layer::DisplayWidget(const std::string&)
{
	CE::ShowInspectUI("mObjects", mObjects);
	CE::ShowInspectUI("mCellSize", mCellSize);
	CE::ShowInspectUI("mObjectsRadius", mObjectsRadius);

	ImGui::SliderFloat("mMinNoiseValueToSpawn", &mMinNoiseValueToSpawn, 0.0f, 1.0f);
	ImGui::SliderFloat("mMaxNoiseValueToSpawn", &mMaxNoiseValueToSpawn, 0.0f, 1.0f);
	ImGui::SliderFloat("mObjectSpawnChance", &mObjectSpawnChance, 0.0f, 1.0f);

	CE::ShowInspectUI("mNumberOfRandomRotations", mNumberOfRandomRotations);
	CE::ShowInspectUI("mScaleAtNoiseValue", mScaleAtNoiseValue);
	CE::ShowInspectUI("mMaxRandomOffset", mMaxRandomOffset);
	CE::ShowInspectUI("mNoiseScale", mNoiseScale);
	CE::ShowInspectUI("mNoiseNumOfOctaves", mNoiseNumOfOctaves);
	CE::ShowInspectUI("mNoisePersistence", mNoisePersistence);
	CE::ShowInspectUI("mInfluences", mInfluences);
	CE::ShowInspectUI("mWeight", mWeight);
	CE::ShowInspectUI("mIsDebugDrawingEnabled", mIsDebugDrawingEnabled);
}
#endif // EDITOR

bool Game::EnvironmentGeneratorComponent::Layer::operator==(const Layer& other) const
{
	return mObjects == other.mObjects
		&& mCellSize == other.mCellSize
		&& mNumberOfRandomRotations == other.mNumberOfRandomRotations
		&& mScaleAtNoiseValue == other.mScaleAtNoiseValue
		&& mMaxRandomOffset == other.mMaxRandomOffset
		&& mNoiseScale == other.mNoiseScale
		&& mNoiseNumOfOctaves == other.mNoiseNumOfOctaves
		&& mNoisePersistence == other.mNoisePersistence
		&& mInfluences == other.mInfluences
		&& mWeight == other.mWeight
		&& mIsDebugDrawingEnabled == other.mIsDebugDrawingEnabled
		&& mMinNoiseValueToSpawn == other.mMinNoiseValueToSpawn
		&& mMaxNoiseValueToSpawn == other.mMaxNoiseValueToSpawn
		&& mObjectSpawnChance == other.mObjectSpawnChance
		&& mObjectsRadius == other.mObjectsRadius;
}

bool Game::EnvironmentGeneratorComponent::Layer::operator!=(const Layer& other) const
{
	return !(*this == other);
}

CE::MetaType Game::EnvironmentGeneratorComponent::Layer::Reflect()
{
	CE::MetaType type = CE::MetaType{ CE::MetaType::T<Layer>{}, "Layer" };
	CE::MetaProps& props = type.GetProperties();
	props.Add(CE::Props::sIsScriptableTag).Add(CE::Props::sIsScriptOwnableTag);

	type.AddField(&Layer::mObjects, "mObjects").GetProperties().Add(CE::Props::sIsScriptableTag);
	type.AddField(&Layer::mCellSize, "mCellSize").GetProperties().Add(CE::Props::sIsScriptableTag);
	type.AddField(&Layer::mMinNoiseValueToSpawn, "mMinNoiseValueToSpawn").GetProperties().Add(CE::Props::sIsScriptableTag);
	type.AddField(&Layer::mMaxNoiseValueToSpawn, "mMaxNoiseValueToSpawn").GetProperties().Add(CE::Props::sIsScriptableTag);
	type.AddField(&Layer::mObjectSpawnChance, "mObjectSpawnChance").GetProperties().Add(CE::Props::sIsScriptableTag);
	type.AddField(&Layer::mNumberOfRandomRotations, "mNumberOfRandomRotations").GetProperties().Add(CE::Props::sIsScriptableTag);
	type.AddField(&Layer::mScaleAtNoiseValue, "mScaleAtNoiseValue").GetProperties().Add(CE::Props::sIsScriptableTag);
	type.AddField(&Layer::mMaxRandomOffset, "mMaxRandomOffset").GetProperties().Add(CE::Props::sIsScriptableTag);
	type.AddField(&Layer::mNoiseScale, "mNoiseScale").GetProperties().Add(CE::Props::sIsScriptableTag);
	type.AddField(&Layer::mNoiseNumOfOctaves, "mNoiseNumOfOctaves").GetProperties().Add(CE::Props::sIsScriptableTag);
	type.AddField(&Layer::mNoisePersistence, "mNoisePersistence").GetProperties().Add(CE::Props::sIsScriptableTag);
	type.AddField(&Layer::mInfluences, "mInfluences").GetProperties().Add(CE::Props::sIsScriptableTag);
	type.AddField(&Layer::mWeight, "mWeight").GetProperties().Add(CE::Props::sIsScriptableTag);
	type.AddField(&Layer::mIsDebugDrawingEnabled, "mIsDebugDrawingEnabled").GetProperties().Add(CE::Props::sIsScriptableTag);

	CE::ReflectFieldType<Layer>(type);
	return type;
}

CE::MetaType Game::EnvironmentGeneratorComponent::Reflect()
{
	CE::MetaType type = CE::MetaType{ CE::MetaType::T<EnvironmentGeneratorComponent>{}, "EnvironmentGeneratorComponent" };
	CE::MetaProps& props = type.GetProperties();
	props.Add(CE::Props::sIsScriptableTag);

	type.AddField(&EnvironmentGeneratorComponent::mGenerateRadius, "mGenerateRadius").GetProperties().Add(CE::Props::sIsScriptableTag);
	type.AddField(&EnvironmentGeneratorComponent::mDestroyRadius, "mDestroyRadius").GetProperties().Add(CE::Props::sIsScriptableTag);
	type.AddField(&EnvironmentGeneratorComponent::mDistToMoveBeforeRegeneration, "mDistToMoveBeforeRegeneration").GetProperties().Add(CE::Props::sIsScriptableTag);
	type.AddField(&EnvironmentGeneratorComponent::mSeed, "mSeed").GetProperties().Add(CE::Props::sIsScriptableTag).Add(CE::Props::sNoSerializeTag);
	type.AddField(&EnvironmentGeneratorComponent::mLayers, "mLayers").GetProperties().Add(CE::Props::sIsScriptableTag);
	type.AddField(&EnvironmentGeneratorComponent::mDebugDrawNoiseHeight, "mDebugDrawNoiseHeight").GetProperties().Add(CE::Props::sIsScriptableTag);
	type.AddField(&EnvironmentGeneratorComponent::mDebugDrawDistanceBetweenLayers, "mDebugDrawDistanceBetweenLayers").GetProperties().Add(CE::Props::sIsScriptableTag);

	type.AddFunc([](EnvironmentGeneratorComponent& component)
		{
			component.mLastGeneratedAtPosition = glm::vec2{ std::numeric_limits<float>::infinity() };
			component.mShouldGenerateInEditor = true;
			component.mWasClearingRequested = true;
		}, "Regenerate", CE::MetaFunc::ExplicitParams<EnvironmentGeneratorComponent&>{}).GetProperties().Add(CE::Props::sCallFromEditorTag);

	type.AddFunc([](EnvironmentGeneratorComponent& component)
		{
			component.mLastGeneratedAtPosition = glm::vec2{ std::numeric_limits<float>::infinity() };
			component.mShouldGenerateInEditor = true;
			component.mWasClearingRequested = true;
			component.mSeed = CE::Random::Value<uint32>();
		}, "Generate", CE::MetaFunc::ExplicitParams<EnvironmentGeneratorComponent&>{}).GetProperties().Add(CE::Props::sCallFromEditorTag);

	type.AddFunc([](EnvironmentGeneratorComponent& component)
		{
			component.mLastGeneratedAtPosition = glm::vec2{ std::numeric_limits<float>::infinity() };
			component.mShouldGenerateInEditor = false;
			component.mWasClearingRequested = true;
		}, "Clear", CE::MetaFunc::ExplicitParams<EnvironmentGeneratorComponent&>{}).GetProperties().Add(CE::Props::sCallFromEditorTag);

	CE::ReflectComponentType<EnvironmentGeneratorComponent>(type);
	return type;
}
