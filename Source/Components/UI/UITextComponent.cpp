#include "Precomp.h"
#include "Components/UI/UITextComponent.h"
#include "Assets/Font.h"
#include "Utilities/Reflect/ReflectComponentType.h"

CE::MetaType CE::UITextComponent::Reflect()
{
	MetaType type = MetaType{ MetaType::T<UITextComponent>{}, "UITextComponent" };
	MetaProps& props = type.GetProperties();
	props.Add(Props::sIsScriptableTag);

	type.AddField(&UITextComponent::mText, "mText").GetProperties().Add(Props::sIsScriptableTag);
	type.AddField(&UITextComponent::mColor, "mColor").GetProperties().Add(Props::sIsScriptableTag);
	type.AddField(&UITextComponent::mSpacing, "mSpacing").GetProperties().Add(Props::sIsScriptableTag);
	type.AddField(&UITextComponent::mFont, "mFont").GetProperties().Add(Props::sIsScriptableTag);

	ReflectComponentType<UITextComponent>(type);
	return type;
}