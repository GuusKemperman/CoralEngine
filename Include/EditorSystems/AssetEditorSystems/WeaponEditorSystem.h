#ifdef EDITOR
#pragma once
#include "Assets/Ability/Weapon.h"
#include "AssetEditorSystem.h"

namespace CE
{
	class WeaponEditorSystem :
		public AssetEditorSystem<Weapon>
	{
	public:
		WeaponEditorSystem(Weapon&& asset);

		void Tick(float deltaTime) override;

	private:
		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(WeaponEditorSystem);
	};
}
#endif // EDITOR
