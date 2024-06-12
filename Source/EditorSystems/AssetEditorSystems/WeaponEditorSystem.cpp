#include "Precomp.h"
#include "EditorSystems/AssetEditorSystems/WeaponEditorSystem.h"

#include "Utilities/Imgui/ImguiInspect.h"
#include "Assets/Script.h"
#include "Assets/Texture.h"
#include "Meta/MetaManager.h"

CE::WeaponEditorSystem::WeaponEditorSystem(Weapon&& asset)
	: AssetEditorSystem(std::move(asset))
{
}

void CE::WeaponEditorSystem::Tick(float deltaTime)
{
	if (!Begin(ImGuiWindowFlags_MenuBar))
	{
		End();
		return;
	}

	AssetEditorSystem::Tick(deltaTime);

	if (ImGui::BeginMenuBar())
	{
		ShowSaveButton();
		ImGui::EndMenuBar();
	}
	ImGui::PushItemWidth(100.f);
	ShowInspectUI("Shot Delay", mAsset.mShotDelay);
	ImGui::SameLine();
	ImGui::BeginDisabled();
	ShowInspectUI("Fire Speed", mAsset.mFireSpeed);
	ImGui::EndDisabled();
	ImGui::SameLine();
	ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
	ImGui::Text("Fire Rate: %.2f", 1.f / (mAsset.mShotDelay * mAsset.mFireSpeed));
	ImGui::PopStyleColor();

	ShowInspectUI("Reload Time", mAsset.mRequirementToUse);
	ImGui::SameLine();
	ImGui::BeginDisabled();
	ShowInspectUI("Reload Speed", mAsset.mReloadSpeed);
	ImGui::EndDisabled();

	ShowInspectUI("Projectile Count", mAsset.mProjectileCount);
	ShowInspectUI("Projectile Spread", mAsset.mSpread);
	ImGui::SameLine();
	ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
	ImGui::Text("Projectile Angles: %.2f", mAsset.mSpread / static_cast<float>(mAsset.mProjectileCount));
	ImGui::PopStyleColor();

	ShowInspectUI("Ammo Count", mAsset.mCharges);
	ShowInspectUI("Projectile Effect", mAsset.mEffects);
	ShowInspectUI("Projectile Size", mAsset.mProjectileSize);
	ShowInspectUI("Projectile Speed", mAsset.mProjectileSpeed);
	ShowInspectUI("Projectile Range", mAsset.mProjectileRange);
	ShowInspectUI("Pierce Count", mAsset.mPierceCount);
	ShowInspectUI("Knockback", mAsset.mKnockback);

	ShowInspectUI("Shooting Slowdown", mAsset.mShootingSlowdown);
	mAsset.mShootingSlowdown = std::clamp(mAsset.mShootingSlowdown, 0.f, 100.f);

	ShowInspectUI("Shoot On Release", mAsset.mShootOnRelease);

	ImGui::Separator();

	ShowInspectUI("On Ability Activate Script", mAsset.mOnAbilityActivateScript);
	ShowInspectUI("Icon Texture", mAsset.mIconTexture);
	ShowInspectUI("Description", mAsset.mDescription);
	ShowInspectUI("Global Cooldown", mAsset.mGlobalCooldown);

	ImGui::PopItemWidth();
	End();
}

CE::MetaType CE::WeaponEditorSystem::Reflect()
{
	return { MetaType::T<WeaponEditorSystem>{}, "WeaponEditorSystem",
		MetaType::Base<AssetEditorSystem<Weapon>>{},
		MetaType::Ctor<Weapon&&>{} };
}

