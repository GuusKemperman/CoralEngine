#pragma once

namespace Engine
{
	class Prefab;
}

namespace Game
{
	class SpawnerComponent
	{
	public:
		std::shared_ptr<const Engine::Prefab> mPrefab{};

		float mCurrentTimer = 0.0f;
		float mSpawningTimer = 5.0f;

	private:
		friend Engine::ReflectAccess;
		static Engine::MetaType Reflect();
		REFLECT_AT_START_UP(SpawnerComponent);
	};
}
