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
	ShowInspectUI("Shot Delay", mAsset.mTimeBetweenShots);
	ImGui::SameLine();
	ShowInspectUI("Fire Speed", mAsset.mFireSpeed);
	ImGui::SameLine();
	ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
	ImGui::Text("Fire Rate: %.2f", 1.f / (mAsset.mTimeBetweenShots * mAsset.mFireSpeed));
	ImGui::PopStyleColor();

	ShowInspectUI("Reload Time", mAsset.mRequirementToUse);
	ImGui::SameLine();
	ShowInspectUI("Reload Speed", mAsset.mReloadSpeed);

	ShowInspectUI("Projectile Count", mAsset.mProjectileCount);
	ShowInspectUI("Projectile Spread", mAsset.mSpread);
	ImGui::SameLine();
	ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
	ImGui::Text("Projectile Angles: %.2f", static_cast<float>(mAsset.mProjectileCount) / mAsset.mSpread);
	ImGui::PopStyleColor();

	ShowInspectUI("Ammo Count", mAsset.mCharges);
	ShowInspectUI("Projectile Effect", mAsset.mEffects);
	ShowInspectUI("Projectile Size", mAsset.mProjectileSize);
	ShowInspectUI("Projectile Speed", mAsset.mProjectileSpeed);
	ShowInspectUI("Projectile Range", mAsset.mProjectileRange);
	ShowInspectUI("Pierce Count", mAsset.mPiercing);
	ShowInspectUI("Knockback", mAsset.mKnockback);

	ImGui::Separator();

	ShowInspectUI("OnAbilityActivateScript", mAsset.mOnAbilityActivateScript);
	ShowInspectUI("IconTexture", mAsset.mIconTexture);
	ShowInspectUI("Description", mAsset.mDescription);
	ShowInspectUI("GlobalCooldown", mAsset.mGlobalCooldown);

	ImGui::PopItemWidth();
	End();
}

CE::MetaType CE::WeaponEditorSystem::Reflect()
{
	return { MetaType::T<WeaponEditorSystem>{}, "WeaponEditorSystem",
		MetaType::Base<AssetEditorSystem<Weapon>>{},
		MetaType::Ctor<Weapon&&>{} };
}

