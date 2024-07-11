#pragma once
#include "Meta/MetaReflect.h"
#include "Assets/Core/AssetHandle.h"

namespace CE
{
	class Material;
	class World;
	class Registry;

	class MaterialRootComponent
	{
	public:
		void SwitchMaterial();
		void SwitchMaterial(Registry& reg, const AssetHandle<Material>& material);

		void OnConstruct(World&, entt::entity owner);

		entt::entity mOwner{};

		AssetHandle<Material> mWantedMaterial{};

	private:
		void SwitchMaterialRecursive(Registry& reg, entt::entity entity, const AssetHandle<Material>& material);

		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(MaterialRootComponent);
	};
}