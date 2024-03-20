#include "Precomp.h"
#include "Components/UI/UISprite.h"
#include "Assets/Texture.h"
#include "Meta/ReflectedTypes/STD/ReflectSmartPtr.h"
#include "Utilities/Reflect/ReflectComponentType.h"

Engine::MetaType Engine::UISprite::Reflect()
{
	MetaType type = MetaType{ MetaType::T<UISprite>{}, "UISpriteComponent" };
	MetaProps& props = type.GetProperties();
	props.Add(Props::sIsScriptableTag);
	type.AddField(&UISprite::mColor, "mColor").GetProperties().Add(Props::sIsScriptableTag);
	type.AddField(&UISprite::mTexture, "mTexture").GetProperties().Add(Props::sIsScriptableTag);
	ReflectComponentType<UISprite>(type);
	return type;
}