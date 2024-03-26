#ifdef EDITOR
#pragma once
#include "Assets/Core/ImportedAsset.h"

namespace Engine
{
	class Animation;

	class AnimationImporter
	{
	public:
		[[nodiscard]] static ImportedAsset Import(const std::filesystem::path& importedFromFile,
			const uint32 importerVersion,
			const Animation& animation);
	};
}
#endif // EDITOR