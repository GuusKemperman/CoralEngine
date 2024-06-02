#pragma once
#include "Assets/Core/AssetHandle.h"
#include "Meta/Fwd/MetaTypeFwd.h"

namespace CE
{
	struct ReflectAccess;
}

namespace Game
{
	class Upgrade;

	// Stores a reference to an upgrade asset.
	// Currently being used for the UI visualization and selection of an upgrade,
	// but it can be used for anything needing to store a reference to an upgrade.
	class UpgradeStoringComponent
	{
	public:
		CE::AssetHandle<Upgrade> mUpgrade{};

	private:
		friend CE::ReflectAccess;
		static CE::MetaType Reflect();
		REFLECT_AT_START_UP(UpgradeStoringComponent);
	};
}
