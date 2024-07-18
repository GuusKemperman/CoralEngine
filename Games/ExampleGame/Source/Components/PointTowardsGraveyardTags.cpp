#include "Precomp.h"
#include "Components/PointTowardsGraveyardTags.h"

#include "Components/TransformComponent.h"
#include "Meta/MetaType.h"
#include "Utilities/Reflect/ReflectComponentType.h"

void Game::IsSignPostTag::OnBeginPlay(CE::World& world, entt::entity signPost)
{
	CE::Registry& reg = world.GetRegistry();

	glm::vec2 nearestGraveyardPos{};
	float nearestDist = std::numeric_limits<float>::infinity();

	CE::TransformComponent* myTransform = reg.TryGet<CE::TransformComponent>(signPost);

	if (myTransform == nullptr)
	{
		return;
	}

	const glm::vec2 myPos = myTransform->GetWorldPosition2D();

	for (const auto& [_, graveyardTransform] : reg.View<IsGraveyardTag, CE::TransformComponent>().each())
	{
		const glm::vec2 pos = graveyardTransform.GetWorldPosition2D();
		const float dist = glm::distance(pos, myPos);
		if (dist < nearestDist)
		{
			nearestDist = dist;
			nearestGraveyardPos = pos;
		}
	};

	if (nearestDist == std::numeric_limits<float>::infinity())
	{
		// Make sure we are not pointing in the wrong direction
		for (const CE::TransformComponent& child : myTransform->GetChildren())
		{
			reg.Destroy(child.GetOwner(), true);
		}
		return;
	}

	const glm::vec2 dir = (nearestGraveyardPos - myPos) / nearestDist;
	myTransform->SetWorldOrientation(CE::Math::Direction2DToXZQuatOrientation(dir));
}

CE::MetaType Game::IsSignPostTag::Reflect()
{
	CE::MetaType metaType = CE::MetaType{ CE::MetaType::T<IsSignPostTag>{}, "IsSignPostTag" };

	CE::BindEvent(metaType, CE::sOnBeginPlay, &IsSignPostTag::OnBeginPlay);
	CE::ReflectComponentType<IsSignPostTag>(metaType);

	return metaType;
}

CE::MetaType Game::IsGraveyardTag::Reflect()
{
	CE::MetaType metaType = CE::MetaType{ CE::MetaType::T<IsGraveyardTag>{}, "IsGraveyardTag" };
	CE::ReflectComponentType<IsGraveyardTag>(metaType);

	return metaType;
}
