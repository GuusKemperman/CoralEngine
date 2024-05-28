#ifdef EDITOR
#pragma once
#include "EditorSystems/AssetEditorSystems/AssetEditorSystem.h"

#include "Assets/Upgrade.h"

namespace Game
{
	class UpgradeEditorSystem final :
		public CE::AssetEditorSystem<Upgrade>
	{
	public:
		UpgradeEditorSystem(Upgrade&& asset);

		void Tick(float deltaTime) override;

	private:
		friend CE::ReflectAccess;
		static CE::MetaType Reflect();
		REFLECT_AT_START_UP(UpgradeEditorSystem);
	};
}

#endif // EDITOR