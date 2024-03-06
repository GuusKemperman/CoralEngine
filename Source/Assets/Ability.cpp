#include "Precomp.h"
#include "Assets/Ability.h"

#include "Meta/MetaType.h"
#include "Meta/MetaProps.h"
#include "Assets/Core/AssetLoadInfo.h"
#include "Meta/Fwd/MetaTypeFwd.h"
#include "Utilities/Reflect/ReflectAssetType.h"

Engine::Ability::Ability(std::string_view name) :
	Asset(name, MakeTypeId<Ability>())
{
}

Engine::Ability::Ability(AssetLoadInfo& loadInfo) :
	Asset(loadInfo)
{
	BinaryGSONObject obj{};
	const bool success = obj.LoadFromBinary(loadInfo.GetStream());

	if (!success)
	{
		LOG(LogAssets, Error, "Could not load ability {}, GSON parsing failed.", GetName());
		return;
	}

	const BinaryGSONMember* serializedIconTextureName = obj.TryGetGSONMember("IconTextureName");
	const BinaryGSONMember* serializedDescription = obj.TryGetGSONMember("Description");
	const BinaryGSONMember* serializedGlobalCooldown = obj.TryGetGSONMember("GlobalCooldown");
	const BinaryGSONMember* serializedRequirementType = obj.TryGetGSONMember("RequirementType");
	const BinaryGSONMember* serializedRequirementToUse = obj.TryGetGSONMember("RequirementToUse");
	const BinaryGSONMember* serializedCharges = obj.TryGetGSONMember("Charges");

	if (serializedIconTextureName == nullptr
		|| serializedDescription == nullptr
		|| serializedGlobalCooldown == nullptr
		|| serializedRequirementType == nullptr
		|| serializedRequirementToUse == nullptr
		|| serializedCharges == nullptr)
	{
		LOG(LogAssets, Error, "Could not load ability {}, as there were missing values.", GetName());
		return;
	}

	*serializedIconTextureName >> mIconTextureName;
	*serializedDescription >> mDescription;
	*serializedGlobalCooldown >> mGlobalCooldown;
	*serializedRequirementType >> mRequirementType;
	*serializedRequirementToUse >> mRequirementToUse;
	*serializedCharges >> mCharges;
}

bool Engine::Ability::operator==(const Ability& other) const
{
	return mIconTextureName == other.mIconTextureName &&
		mDescription == other.mDescription &&
		mGlobalCooldown == other.mGlobalCooldown &&
		mRequirementType == other.mRequirementType &&
		mRequirementToUse - other.mRequirementToUse <= 1e-9 &&
		mCharges == other.mCharges;
}

bool Engine::Ability::operator!=(const Ability& other) const
{
	return mIconTextureName != other.mIconTextureName ||
		mDescription != other.mDescription ||
		mGlobalCooldown != other.mGlobalCooldown ||
		mRequirementType != other.mRequirementType ||
		mRequirementToUse - other.mRequirementToUse > 1e-9 ||
		mCharges != other.mCharges;
}

void Engine::Ability::OnSave(AssetSaveInfo& saveInfo) const
{
	BinaryGSONObject obj{};

	obj.AddGSONMember("IconTextureName") << mIconTextureName;
	obj.AddGSONMember("Description") << mDescription;
	obj.AddGSONMember("GlobalCooldown") << mGlobalCooldown;
	obj.AddGSONMember("RequirementType") << mRequirementType;
	obj.AddGSONMember("RequirementToUse") << mRequirementToUse;
	obj.AddGSONMember("Charges") << mCharges;

	obj.SaveToBinary(saveInfo.GetStream());
}

Engine::MetaType Engine::Ability::Reflect()
{
	MetaType type = MetaType{ MetaType::T<Ability>{}, "Ability", MetaType::Base<Asset>{}, MetaType::Ctor<AssetLoadInfo&>{}, MetaType::Ctor<std::string_view>{} };

	type.AddField(&Ability::mIconTextureName, "mIconTextureName");
	type.AddField(&Ability::mDescription, "mDescription");
	type.AddField(&Ability::mGlobalCooldown, "mGlobalCooldown");
	type.AddField(&Ability::mRequirementType, "mRequirementType");
	type.AddField(&Ability::mRequirementToUse, "mRequirementToUse");
	type.AddField(&Ability::mCharges, "mCharges");

	ReflectAssetType<Ability>(type);
	return type;
}

Engine::MetaType Reflector<Engine::Ability::RequirementType>::Reflect()
{
	using namespace Engine;
	using T = Ability::RequirementType;
	MetaType type{ MetaType::T<T>{}, "Ability RequirementType" };

	type.GetProperties().Add(Props::sIsScriptableTag).Add(Props::sIsScriptOwnableTag);
	type.AddFunc(std::equal_to<T>(), OperatorType::equal, MetaFunc::ExplicitParams<const T&, const T&>{}).GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc(std::not_equal_to<T>(), OperatorType::inequal, MetaFunc::ExplicitParams<const T&, const T&>{}).GetProperties().Add(Props::sIsScriptableTag);
	ReflectFieldType<T>(type);

	return type;
}