#ifdef EDITOR
#pragma once
#include "EditorSystems/AssetEditorSystems/AssetEditorSystem.h"

#include "Assets/Upgrade.h"

namespace CE
{
	class World;

	class UpgradeEditorSystem final :
		public AssetEditorSystem<Upgrade>
	{
	public:
		UpgradeEditorSystem(Upgrade&& asset);

		void Tick(float deltaTime) override;

	private:
		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(UpgradeEditorSystem);
	};
}

#endif // EDITOR