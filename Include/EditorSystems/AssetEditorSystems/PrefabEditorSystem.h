#ifdef EDITOR
#pragma once
#include "EditorSystems/AssetEditorSystems/AssetEditorSystem.h"

#include "Assets/Prefabs/Prefab.h"

namespace Engine
{
	class WorldInspectHelper;
	
	class PrefabEditorSystem final :
		public AssetEditorSystem<Prefab>
	{
	public:
		PrefabEditorSystem(Prefab&& asset);
		~PrefabEditorSystem() override;

		void Tick(float deltaTime) override;

	private:
		void ApplyChangesToAsset() override;

		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(PrefabEditorSystem);

		std::unique_ptr<WorldInspectHelper> mWorldHelper{};

		entt::entity mPrefabInstance{};
	};
}
#endif // EDITOR
