#include "Precomp.h"
#include "Utilities/Time.h"

#include "Meta/MetaType.h"
#include "Meta/Fwd/MetaPropsFwd.h"
#include "Utilities/Reflect/ReflectFieldType.h"

bool CE::Cooldown::IsReady(float dt)
{
	mAmountOfTimePassed += dt;

	if (mAmountOfTimePassed >= mCooldown)
	{
		mAmountOfTimePassed = 0.0f;
		return true;
	}
	return false;
}

#ifdef EDITOR
void CE::Cooldown::DisplayWidget(const std::string& name)
{
	ImGui::PushID(name.c_str());
	ImGui::Indent();
	ShowInspectUI("mAmountOfTimePassed", mAmountOfTimePassed);
	ShowInspectUI("mCooldown", mCooldown);
	ImGui::Unindent();
	ImGui::PopID();
}
#endif // EDITOR

bool CE::Cooldown::operator==(const Cooldown& other) const
{
	return other.mCooldown == mCooldown && mAmountOfTimePassed == other.mAmountOfTimePassed;
}

bool CE::Cooldown::operator!=(const Cooldown& other) const
{
	return other.mCooldown != mCooldown || mAmountOfTimePassed != other.mAmountOfTimePassed;
}

CE::MetaType CE::Cooldown::Reflect()
{
	MetaType type{ MetaType::T<Cooldown>{}, "Cooldown" };
	type.GetProperties().Add(Props::sIsScriptableTag).Add(Props::sIsScriptOwnableTag);

	type.AddField(&Cooldown::mCooldown, "mCooldown").GetProperties().Add(Props::sIsScriptableTag);
	type.AddField(&Cooldown::mAmountOfTimePassed, "mAmountOfTimePassed").GetProperties().Add(Props::sIsScriptableTag);

	type.AddFunc(&Cooldown::IsReady, "IsReady", "", "DeltaTime").GetProperties().Add(Props::sIsScriptableTag);

	ReflectFieldType<Cooldown>(type);
	return type;
}

CE::Timer::Timer() :
	mStart(std::chrono::high_resolution_clock::now())
{}

float CE::Timer::GetSecondsElapsed() const
{
	return std::chrono::duration_cast<std::chrono::duration<float>>(std::chrono::high_resolution_clock::now() - mStart).count();
}

void CE::Timer::Reset()
{
	mStart = std::chrono::high_resolution_clock::now();
}

void cereal::save(BinaryOutputArchive& ar, const CE::Cooldown& value)
{
	ar(value.mAmountOfTimePassed, value.mCooldown);
}

void cereal::load(BinaryInputArchive& ar, CE::Cooldown& value)
{
	ar(value.mAmountOfTimePassed, value.mCooldown);
}
