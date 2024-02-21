#ifdef EDITOR
#pragma once
#include "EditorSystems/AssetEditorSystems/AssetEditorSystem.h"

#include "Assets/Material.h"

namespace Engine
{
	class World;

	class MaterialEditorSystem final :
		public AssetEditorSystem<Material>
	{
	public:
		MaterialEditorSystem(Material&& asset);
		~MaterialEditorSystem() override;

		void Tick(float deltaTime) override;

	private:
		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(MaterialEditorSystem);
	};
}

#endif // EDITOR