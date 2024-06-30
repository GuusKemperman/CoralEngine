#include "Precomp.h"
#include "Systems/XPOrbSystem.h"

#include <entt/entity/runtime_view.hpp>

#include "Components/TransformComponent.h"
#include "Components/XPOrbComponent.h"
#include "Components/XPOrbManagerComponent.h"
#include "World/Registry.h"

void Game::XPOrbSystem::Update(CE::World& world, float dt)
{
	CE::Registry& reg = world.GetRegistry();

	auto& xpOrbStorage = reg.Storage<XPOrbComponent>();

	const entt::entity managerEntity = reg.View<XPOrbManagerComponent>().front();

	if (managerEntity == entt::null)
	{
		if (!xpOrbStorage.empty())
		{
			LOG(LogGame, Error, "XPOrbs requires an entity with the XPOrbManagerComponent to be somewhere in the world");
		}
		return;
	}
	const XPOrbManagerComponent& manager = reg.Get<XPOrbManagerComponent>(managerEntity);

	const size_t maxSize = manager.mMaxAlive;
	if (xpOrbStorage.size() > maxSize)
	{
		const size_t amountToErase = xpOrbStorage.size() - maxSize;
		xpOrbStorage.sort_n(amountToErase + 1, [&xpOrbStorage](const entt::entity lhs, const entt::entity rhs)
			{
				return xpOrbStorage.get(lhs).mTimeAlive > xpOrbStorage.get(rhs).mTimeAlive;
			});
		const auto xpOrbView = reg.View<XPOrbComponent>();
		const auto begin = xpOrbView.begin();
		reg.Destroy(begin, begin + amountToErase, true);
		reg.RemovedDestroyed();
	}

	const CE::MetaType* levellingScript = CE::MetaManager::Get().TryGetType("S_LevellingScript");

	if (levellingScript == nullptr)
	{
		LOG(LogGame, Error, "Could not find S_LevellingScript");
		return;
	}

	entt::sparse_set* levellingStorage = reg.Storage(levellingScript->GetTypeId());

	if (levellingStorage == nullptr)
	{
		return;
	}

	entt::runtime_view levellingView{};
	levellingView.iterate(*levellingStorage);
	levellingView.iterate(reg.Storage<CE::TransformComponent>());

	if (levellingView.begin() == levellingView.end())
	{
		return;
	}

	const CE::TransformComponent& levellingTransform = reg.Get<CE::TransformComponent>(*levellingView.begin());
	CE::MetaAny levellingComponent = reg.Get(levellingScript->GetTypeId(), *levellingView.begin());

	const CE::MetaField* isLevelingField = levellingScript->TryGetField("Is Leveling");
	const CE::MetaField* pickUpRangeField = levellingScript->TryGetField("PickUpRange");
	const CE::MetaFunc* addXPFunc = levellingScript->TryGetFunc("AddXP");

	if (isLevelingField == nullptr
		|| pickUpRangeField == nullptr
		|| addXPFunc == nullptr
		|| isLevelingField->GetType().GetTypeId() != CE::MakeTypeId<bool>()
		|| pickUpRangeField->GetType().GetTypeId() != CE::MakeTypeId<float>()
		|| addXPFunc->GetParameters().size() != 2
		|| addXPFunc->GetParameters()[0].mTypeTraits.mStrippedTypeId != levellingScript->GetTypeId()
		|| addXPFunc->GetParameters()[1].mTypeTraits.mStrippedTypeId != CE::MakeTypeId<float>())
	{
		LOG(LogGame, Error, "Hardcoded fields/functions of S_LevellingScript are now missing or are different types");
		return;
	}

	const bool* isLevelling = isLevelingField->MakeRef(levellingComponent).As<bool>();
	const float* pickUpRange = pickUpRangeField->MakeRef(levellingComponent).As<float>();

	if (isLevelling == nullptr
		|| pickUpRange == nullptr)
	{
		LOG(LogGame, Error, "Failed to retrieve values from S_LevellingScript");
		return;
	}

	const float pickUpRange2 = CE::Math::sqr(*pickUpRange);

	const glm::vec2 levellingPos2D = levellingTransform.GetWorldPosition2D();

	const float speedThisFrame = manager.mChaseSpeed * dt;
	const float chaseRangeSqrd = manager.mChaseRange * manager.mChaseRange;

	for (auto [entity, transform, orb] : reg.View<CE::TransformComponent, XPOrbComponent>().each())
	{
		orb.mTimeAlive += dt;
		if (orb.mTimeAlive > manager.mMaxTimeAlive)
		{
			reg.Destroy(entity, true);
			continue;
		}

		glm::vec3 worldPos = transform.GetWorldPosition();

		{
			const glm::vec2 worldPos2D = CE::To2DRightForward(worldPos);

			const glm::vec2 toLevelling = levellingPos2D - worldPos2D;
			const float distance2 = glm::length2(toLevelling);

			if (!*isLevelling
				&& distance2 <= pickUpRange2)
			{
				addXPFunc->InvokeUncheckedUnpacked(levellingComponent, manager.mXPValue);
				reg.Destroy(entity, true);
				continue;
			}

			if (distance2 < chaseRangeSqrd
				&& distance2 > 0.0f)
			{
				const float distance = glm::sqrt(distance2);
				const float speed = glm::min(speedThisFrame, distance);

				worldPos = CE::To3DRightForward(worldPos2D + (toLevelling / distance) * speed, worldPos[CE::Axis::Up]);
			}
		}

		worldPos[CE::Axis::Up] = glm::sin(orb.mTimeAlive * manager.mHoverSpeed) * manager.mHoverHeight + 1.0f;
		transform.SetWorldPosition(worldPos);
	}
}

CE::MetaType Game::XPOrbSystem::Reflect()
{
	return CE::MetaType{ CE::MetaType::T<XPOrbSystem>{}, "XPOrbSystem", CE::MetaType::Base<System>{} };
}