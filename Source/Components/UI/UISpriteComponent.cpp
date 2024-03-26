#include "Precomp.h"
#include "Components/UI/UISpriteComponent.h"
#include "Assets/Texture.h"
#include "Meta/ReflectedTypes/STD/ReflectSmartPtr.h"
#include "Utilities/Reflect/ReflectComponentType.h"

Engine::MetaType Engine::UISpriteComponent::Reflect()
{
	MetaType type = MetaType{ MetaType::T<UISpriteComponent>{}, "UISpriteComponent" };
	MetaProps& props = type.GetProperties();
	props.Add(Props::sIsScriptableTag);
	type.AddField(&UISpriteComponent::mColor, "mColor").GetProperties().Add(Props::sIsScriptableTag);
	type.AddField(&UISpriteComponent::mTexture, "mTexture").GetProperties().Add(Props::sIsScriptableTag);
	ReflectComponentType<UISpriteComponent>(type);
	return type;
}