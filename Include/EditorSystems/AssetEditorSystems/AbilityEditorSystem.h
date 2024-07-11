#ifdef EDITOR
#pragma once
#include "EditorSystems/AssetEditorSystems/AssetEditorSystem.h"

#include "Assets/Ability/Ability.h"

namespace CE
{
	class World;

	class AbilityEditorSystem final :
		public AssetEditorSystem<Ability>
	{
	public:
		AbilityEditorSystem(Ability&& asset);

		void Tick(float deltaTime) override;

	private:
		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(AbilityEditorSystem);
	};
}

#endif // EDITOR
