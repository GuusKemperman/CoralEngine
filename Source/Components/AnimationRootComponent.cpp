#include "Precomp.h"
#include "Components/AnimationRootComponent.h"

#include "Assets/Animation/Animation.h"

#include "Meta/MetaType.h"
#include "Meta/MetaProps.h"
#include "Utilities/Reflect/ReflectComponentType.h"
#include "Meta/ReflectedTypes/STD/ReflectSmartPtr.h"

void CE::AnimationRootComponent::SwitchAnimation()
{
	mSwitchAnimation = true;
}

CE::MetaType CE::AnimationRootComponent::Reflect()
{
	auto type = MetaType{MetaType::T<AnimationRootComponent>{}, "AnimationRootComponent"};
	MetaProps& props = type.GetProperties();
	props.Add(Props::sIsScriptableTag);

	type.AddField(&AnimationRootComponent::mWantedAnimation, "mWantedAnimation").GetProperties().Add(Props::sIsScriptableTag);
	type.AddField(&AnimationRootComponent::mWantedTimeStamp, "mWantedTimeStamp").GetProperties().Add(Props::sIsScriptableTag);

	type.AddFunc(&AnimationRootComponent::SwitchAnimation, "SwitchAnimation").GetProperties().Add(Props::sIsScriptableTag).Add(Props::sCallFromEditorTag);
	
	ReflectComponentType<AnimationRootComponent>(type);
	return type;
}
