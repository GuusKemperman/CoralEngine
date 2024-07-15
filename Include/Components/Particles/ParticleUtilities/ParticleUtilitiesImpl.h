#pragma once
#include "Components/TransformComponent.h"
#include "Utilities/Math.h"
#include "Utilities/Random.h"
#include "Utilities/Reflect/ReflectFieldType.h"
#include "Components/Particles/ParticleEmitterComponent.h"
#include "Utilities/Reflect/ReflectComponentType.h"

template <typename T>
CE::ParticleProperty<T>::ParticleProperty() = default;

template <typename T>
CE::ParticleProperty<T>::ParticleProperty(T&& value) :
	mInitialMin(value),
	mInitialMax(value)
{}

template <typename T>
CE::ParticleProperty<T>::ParticleProperty(T&& min, T&& max) :
	mInitialMin(std::forward<T>(min)),
	mInitialMax(std::forward<T>(max))
{}

template <typename T>
constexpr T CE::ParticleProperty<T>::GetValue(const std::vector<typename ChangeOverTime::ValueAtTime>& points, float t)
{
	for (size_t i = 0; i < points.size() - 1; i++)
	{
		if (points[i + 1].mTime < t)
		{
			continue;
		}

		const float weight = Math::lerpInv(points[i].mTime, points[i + 1].mTime, t);
		return Math::lerp(points[i].mValue, points[i + 1].mValue, weight);
	}

	return points.back().mValue;
}

template <typename T>
void CE::ParticleProperty<T>::SetInitialValuesOfNewParticles(const ParticleEmitterComponent& emitter)
{
	mInitialValues.resize(emitter.GetNumOfParticles());

	for (const uint32 particle : emitter.GetParticlesThatSpawnedDuringLastStep())
	{
		const float t = Random::Value<float>();
		mInitialValues[particle] = Math::lerp(mInitialMin, mInitialMax, t);
	}

	if (mChangeOverTime.has_value()
		&& mChangeOverTime->mMax.has_value())
	{
		mChangeOverTime->mMax->mLerpTime.resize(emitter.GetNumOfParticles());

		for (const uint32 particle : emitter.GetParticlesThatSpawnedDuringLastStep())
		{
			mChangeOverTime->mMax->mLerpTime[particle] = Random::Value<float>();
		}
	}
}

template <typename T>
T CE::ParticleProperty<T>::GetValue(const ParticleEmitterComponent& emitter, size_t particleIndex) const
{
	const T& initialValue = mInitialValues[particleIndex];

	if (!mChangeOverTime.has_value()
		|| mChangeOverTime->mMinPoints.empty())
	{
		return initialValue;
	}
	const float sampleTime = emitter.GetParticleLifeTimesAsPercentage()[particleIndex];

	const T minSample = GetValue(mChangeOverTime->mMinPoints, sampleTime);

	if (!mChangeOverTime->mMax.has_value()
		|| mChangeOverTime->mMax->mPoints.empty())
	{
		return minSample * initialValue;
	}

	const T maxSample = GetValue(mChangeOverTime->mMax->mPoints, sampleTime);

	const float lerpValue = mChangeOverTime->mMax->mLerpTime[particleIndex];
	return Math::lerp(minSample, maxSample, lerpValue) * initialValue;
}

#ifdef EDITOR
template <typename T>
void CE::ParticleProperty<T>::DisplayWidget(const std::string& name)
{
	if (!ImGui::TreeNode(name.c_str()))
	{
		return;
	}

	ShowInspectUI("mInitialMin", mInitialMin);
	ShowInspectUI("mInitialMax", mInitialMax);

	bool hasValue = mChangeOverTime.has_value();

	const auto& showPoints = [](auto& points)
		{
			ImGui::Separator();
			ImGui::TextUnformatted("Points");
			ImGui::Indent();

			for (int i = 0; i < static_cast<int>(points.size()); i++)
			{
				ImGui::PushID(i);

				ImGui::SliderFloat("mTime", &points[i].mTime, 0.0f, 1.0f);
				ShowInspectUI("mValue", points[i].mValue);

				ImGui::PopID();
			}

			ImGui::Unindent();

			if (ImGui::Button("+"))
			{
				points.emplace_back();
			}

			ImGui::SameLine();

			if (ImGui::Button("-"))
			{
				if (!points.empty())
				{
					points.pop_back();
				}
			}

			ImGui::Separator();
		};

	if (ImGui::TreeNode("mChangeOverTime"))
	{
		if (ImGui::Checkbox("HasValue", &hasValue))
		{
			if (mChangeOverTime.has_value())
			{
				mChangeOverTime.reset();
			}
			else
			{
				mChangeOverTime.emplace();
			}
		}

		if (mChangeOverTime.has_value())
		{
			showPoints(mChangeOverTime->mMinPoints);

			if (ImGui::TreeNode("RandomInRange"))
			{
				hasValue = mChangeOverTime->mMax.has_value();

				if (ImGui::Checkbox("HasValue", &hasValue))
				{
					if (mChangeOverTime->mMax.has_value())
					{
						mChangeOverTime->mMax.reset();
					}
					else
					{
						mChangeOverTime->mMax.emplace();
					}
				}

				if (mChangeOverTime->mMax.has_value())
				{
					showPoints(mChangeOverTime->mMax->mPoints);
				}

				ImGui::TreePop();
			}
		}

		ImGui::TreePop();
	}

	static float previewTime = .5f;

	T minValue = mInitialMin;
	T avgValue = Math::lerp(mInitialMin, mInitialMax, .5f);
	T maxValue = mInitialMax;

	// Some code repetition with our GetValue function,
	// but extracting the common parts would lead to a
	// slight performance cost.
	const auto& scaleValue = [this](T& value, float lerpValue)
		{
			if (!mChangeOverTime.has_value()
				|| mChangeOverTime->mMinPoints.empty())
			{
				return;
			}

			const T minSample = GetValue(mChangeOverTime->mMinPoints, previewTime);

			if (!mChangeOverTime->mMax.has_value()
				|| mChangeOverTime->mMax->mPoints.empty())
			{
				value *= minSample;
				return;
			}

			const T maxSample = GetValue(mChangeOverTime->mMax->mPoints, previewTime);
			value *= Math::lerp(minSample, maxSample, lerpValue);
		};

	scaleValue(minValue, 0.0f);
	scaleValue(avgValue, 0.5f);
	scaleValue(maxValue, 1.0f);

	if (ImGui::TreeNode("Preview"))
	{
		ImGui::SliderFloat("Preview time", &previewTime, 0.0f, 1.0f);

		ImGui::BeginDisabled();

		ShowInspectUIReadOnly("MinValue", minValue);
		ShowInspectUIReadOnly("AvgValue", avgValue);
		ShowInspectUIReadOnly("MaxValue", maxValue);

		ImGui::EndDisabled();

		ImGui::TreePop();
	}
	ImGui::TreePop();
}
#endif // EDITOR

template <typename T>
bool CE::ParticleProperty<T>::operator==(const ParticleProperty& other) const
{
	return mInitialMin == other.mInitialMin
		&& mInitialMax == other.mInitialMax
		&& mChangeOverTime == other.mChangeOverTime;
}

template <typename T>
bool CE::ParticleProperty<T>::operator!=(const ParticleProperty& other) const
{
	return !(*this == other);
}

template <typename T>
bool CE::ParticleProperty<T>::ChangeOverTime::operator==(const ChangeOverTime& other) const
{
	return mMinPoints == other.mMinPoints
		&& mMax == other.mMax;
}

template <typename T>
bool CE::ParticleProperty<T>::ChangeOverTime::ValueAtTime::operator==(const ValueAtTime& other) const
{
	return mTime == other.mTime
		&& mValue == other.mValue;
}

template <typename T>
template <class Archive>
void CE::ParticleProperty<T>::ChangeOverTime::ValueAtTime::serialize(Archive& ar)
{
	ar(mTime, mValue);
}

template <typename T>
bool CE::ParticleProperty<T>::ChangeOverTime::WithLerp::operator==(const WithLerp& other) const
{
	return mPoints == other.mPoints;
}

template <typename T>
template <class Archive>
void CE::ParticleProperty<T>::ChangeOverTime::WithLerp::serialize(Archive& ar)
{
	uint8 version = 0;
	ar(version, mPoints);
}

template <typename T>
template <class Archive>
void CE::ParticleProperty<T>::ChangeOverTime::serialize(Archive& ar)
{
	uint8 version = 0;
	ar(version, mMinPoints, mMax);
}

#ifdef EDITOR
IMGUI_AUTO_DEFINE_INLINE(template<typename T>, CE::ParticleProperty<T>, var.DisplayWidget(name); )
#endif // EDITOR

namespace cereal
{
	template<class Archive, typename T>
	void serialize(Archive& ar, CE::ParticleProperty<T>& value)
	{
		uint8 version = 0;
		ar(version, value.mInitialMin, value.mInitialMax, value.mChangeOverTime);
	}
}

template <typename T>
CE::MetaType CE::ParticleProperty<T>::Reflect()
{
	const MetaType& basedOnType = MetaManager::Get().GetType<T>();
	MetaType type{ MetaType::T<ParticleProperty<T>>{}, Format("Particle property {}", basedOnType.GetName()) };

	type.GetProperties().Add(Props::sIsScriptableTag).Add(Props::sIsScriptOwnableTag);

	type.AddField(&ParticleProperty<T>::mInitialMin, "mInitialMin").GetProperties().Add(Props::sIsScriptableTag);
	type.AddField(&ParticleProperty<T>::mInitialMax, "mInitialMax").GetProperties().Add(Props::sIsScriptableTag);

	ReflectFieldType<ParticleProperty<T>>(type);

	return type;
}

namespace CE::Internal
{
	void OnParticleComponentDestruct(World& world, entt::entity oldEntity);
	static constexpr std::string_view sTransferOwnershipName = "__TransferOwnership";
}

template<typename ComponentType>
void CE::ReflectParticleComponentType(MetaType& type)
{
	type.AddFunc([](Registry& reg, entt::entity oldEntity, entt::entity newEntity)
		{
			ComponentType* component = reg.TryGet<ComponentType>(oldEntity);

			if (component != nullptr)
			{
				reg.AddComponent<ComponentType>(newEntity, std::move(*component));
			}
		}, Internal::sTransferOwnershipName, MetaFunc::ExplicitParams<Registry&, entt::entity, entt::entity>{});

	BindEvent(type, sOnEndPlay, &Internal::OnParticleComponentDestruct);
	ReflectComponentType<ComponentType>(type);
}
