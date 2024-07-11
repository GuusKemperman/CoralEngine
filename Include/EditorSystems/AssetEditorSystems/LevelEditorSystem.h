#ifdef EDITOR
#pragma once
#include "EditorSystems/AssetEditorSystems/AssetEditorSystem.h"

#include "Assets/Level.h"	

namespace CE
{
	class WorldInspectHelper;

	class LevelEditorSystem final :
		public AssetEditorSystem<Level>
	{
	public:
		LevelEditorSystem(Level&& asset);
		~LevelEditorSystem() override;

		void Tick(float deltaTime) override;

		void SaveState(std::ostream& toStream) const override;

		void LoadState(std::istream& fromStream) override;

	private:
		void ApplyChangesToAsset() override;

		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(LevelEditorSystem);

		std::unique_ptr<WorldInspectHelper> mWorldHelper{};
	};
}
#endif // EDITOR
