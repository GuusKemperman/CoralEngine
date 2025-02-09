#pragma once
#include "Meta/Fwd/MetaTypeIdFwd.h"

namespace CE
{
	class EngineConfig
	{
	public:
		EngineConfig() = default;
		EngineConfig(int argc, char** argv);

		// Prevents a type from entering the runtime
		// reflection system. Can be used to remove
		// core engine types, e.g., assets, systems,
		// or components.
		//
		// While this is the intended method of opting
		// out of engine features, it is no guarantee.
		// For essential types, it is possible that
		// the type is constructed through raw C++,
		// it then doesn't matter if it's in the runtime
		// reflection system.
		//
		// In order to provide a strong guarantee that
		// MetaManager::GetType always returns a type,
		// the ban request will be ignored. A warning
		// will be emitted in non-shipping builds.
		template<typename T>
		void BanType();

		std::vector<std::string> mProgramArguments{};

		std::unordered_set<TypeId> mBannedTypes{};

		std::string mGameDir{};

		bool mShouldRunUnitTests{};

		glm::ivec2 mWindowSizeHint = { 1920, 1080 };

		bool mIsFullScreen{};

		std::string mWindowTitle{};
	};

	class Engine
	{
	public:
		Engine(const EngineConfig& config);
		~Engine();

		void Run(Name starterLevel);
	};

	template <typename T>
	void EngineConfig::BanType()
	{
		mBannedTypes.emplace(MakeTypeId<T>());
	}
}
