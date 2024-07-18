#include "Precomp.h"
#include "Components/MaterialRootComponent.h"

#include "Assets/Material.h"
#include "Components/SkinnedMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TransformComponent.h"
#include "World/World.h"
#include "World/Registry.h"
#include "Meta/MetaType.h"
#include "Meta/MetaProps.h"
#include "Utilities/Reflect/ReflectComponentType.h"

void CE::MaterialRootComponent::SwitchMaterialRecursive(Registry& reg, const entt::entity entity, const AssetHandle<Material>& material)
{
	auto skinnedMesh = reg.TryGet<SkinnedMeshComponent>(entity);

	if (skinnedMesh != nullptr)
	{
		skinnedMesh->mMaterial = material;
	}

	auto staticMesh = reg.TryGet<StaticMeshComponent>(entity);

	if (staticMesh != nullptr)
	{
		staticMesh->mMaterial = material;
	}

	auto transform = reg.TryGet<TransformComponent>(entity);

	if (transform == nullptr)
	{
		return;
	}

	for (const TransformComponent& child : transform->GetChildren())
	{
		SwitchMaterialRecursive(reg, child.GetOwner(), material);
	}
}

void CE::MaterialRootComponent::OnConstruct(World&, entt::entity owner)
{
	mOwner = owner;
}

void CE::MaterialRootComponent::SwitchMaterial()
{
	World* world = World::TryGetWorldAtTopOfStack();
	ASSERT(world != nullptr);

	SwitchMaterialRecursive(world->GetRegistry(), mOwner, mWantedMaterial);
}

void CE::MaterialRootComponent::SwitchMaterial(Registry& reg, const AssetHandle<Material>& material)
{
	if (material == mWantedMaterial)
	{
		return;
	}

	mWantedMaterial = material;

	SwitchMaterialRecursive(reg, mOwner, mWantedMaterial);
}

CE::MetaType CE::MaterialRootComponent::Reflect()
{
	auto type = MetaType{ MetaType::T<MaterialRootComponent>{}, "MaterialRootComponent" };
	MetaProps& props = type.GetProperties();
	props.Add(Props::sIsScriptableTag);

	type.AddField(&MaterialRootComponent::mWantedMaterial, "mWantedMaterial").GetProperties().Add(Props::sIsScriptableTag);

	type.AddFunc(static_cast<void (MaterialRootComponent::*)()>(&MaterialRootComponent::SwitchMaterial), "SwitchMaterialEditor").GetProperties().Add(Props::sCallFromEditorTag);

	type.AddFunc([](MaterialRootComponent& materialRoot, const AssetHandle<Material>& material)
		{
			if (material == nullptr)
			{
				LOG(LogWorld, Warning, "Attempted to set NULL material.");
				return;
			}

			World* world = World::TryGetWorldAtTopOfStack();
			ASSERT(world != nullptr);

			materialRoot.SwitchMaterial(world->GetRegistry(), material);

		}, "SwitchMaterial", MetaFunc::ExplicitParams<MaterialRootComponent&,
		const AssetHandle<Material>&>{}, "MaterialRootComponent", "Material").GetProperties().Add(Props::sIsScriptableTag).Set(Props::sIsScriptPure, false);

		BindEvent(type, sOnConstruct, &MaterialRootComponent::OnConstruct);

		ReflectComponentType<MaterialRootComponent>(type);
		return type;
}
