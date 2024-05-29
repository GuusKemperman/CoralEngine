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
