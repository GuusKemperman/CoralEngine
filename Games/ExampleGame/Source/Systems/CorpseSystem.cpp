#include "Precomp.h"
#include "Systems/CorpseSystem.h"

#include "Components/TransformComponent.h"
#include "Components/CorpseComponent.h"
#include "Components/CorpseManagerComponent.h"
#include "Components/PlayerComponent.h"
#include "World/Registry.h"

void Game::CorpseSystem::Update(CE::World& world, float dt)
{
	CE::Registry& reg = world.GetRegistry();

	auto& corpseStorage = reg.Storage<CorpseComponent>();

	const entt::entity managerEntity = reg.View<CorpseManagerComponent>().front();

	if (managerEntity == entt::null)
	{
		if (!corpseStorage.empty())
		{
			LOG(LogGame, Error, "Corpses requires an entity with the CorpseManagerComponent to be somewhere in the world");
		}
		return;
	}
	const CorpseManagerComponent& manager = reg.Get<CorpseManagerComponent>(managerEntity);

	corpseStorage.sort([&corpseStorage](const entt::entity lhs, const entt::entity rhs)
		{
			return corpseStorage.get(lhs).mTimeOfDeath < corpseStorage.get(rhs).mTimeOfDeath;
		});

	const float currentTime = world.GetCurrentTimeScaled();

	const auto corpseView = reg.View<CorpseComponent, CE::TransformComponent>();
	const float sinkAmount = manager.mSinkSpeed * dt;

	auto current = corpseView.begin();
	uint32 amountSinking{};

	std::optional<glm::vec2> playerPos{};

	const entt::entity playerEntity = reg.View<CE::TransformComponent, CE::PlayerComponent>().front();

	if (playerEntity != entt::null)
	{
		playerPos = reg.Get<CE::TransformComponent>(playerEntity).GetWorldPosition2D();
	}

	const float destroyAtDist2 = CE::Math::sqr(manager.mDestroyAllInstantlyIfOutsideOfRange);

	for (; current != corpseView.end(); ++current, ++amountSinking)
	{
		entt::entity entity = *current;
		CorpseComponent& corpse = corpseView.get<CorpseComponent>(entity);
		
		const float timeSinceDeath = currentTime - corpse.mTimeOfDeath;

		// Theyre sorted from oldest to newest
		if (timeSinceDeath < manager.mMaxTimeAlive)
		{
			// We have found one that is not sinking
			break;
		}

		CE::TransformComponent& transform = corpseView.get<CE::TransformComponent>(entity);
		glm::vec3 worldPos = transform.GetWorldPosition();

		if (playerPos.has_value()
			&& glm::distance2(CE::To2DRightForward(worldPos), *playerPos) >= destroyAtDist2)
		{
			reg.Destroy(entity, true);
			continue;
		}

		worldPos[CE::Axis::Up] -= sinkAmount;

		if (worldPos[CE::Axis::Up] < manager.mDestroyAtHeight)
		{
			reg.Destroy(entity, true);
			continue;
		}

		transform.SetWorldPosition(worldPos);
	}

	const uint32 maxSize = manager.mMaxAlive;
	const uint32 amountNotSinking = static_cast<uint32>(corpseStorage.size()) - amountSinking;

	if (amountNotSinking <= maxSize)
	{
		return;
	}

	const uint32 amountToErase = amountNotSinking - maxSize;
	for (uint32 i = 0; i < amountToErase && current != corpseView.end(); ++current, i++)
	{
		const entt::entity entity = *current;
		CorpseComponent& corpse = corpseView.get<CorpseComponent>(entity);
		corpse.mTimeOfDeath = currentTime - manager.mMaxTimeAlive;
	}
}

CE::MetaType Game::CorpseSystem::Reflect()
{
	return CE::MetaType{ CE::MetaType::T<CorpseSystem>{}, "CorpseSystem", CE::MetaType::Base<System>{} };
}