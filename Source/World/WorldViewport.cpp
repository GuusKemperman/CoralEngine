#include "Precomp.h"
#include "World/WorldViewport.h"

#include "World/World.h"
#include "World/Registry.h"
#include "Components/CameraComponent.h"
#include "Components/TransformComponent.h"
#include "Core/Input.h"
#include "Core/Device.h"
#include "Systems/System.h"

CE::WorldViewport::WorldViewport(const World& world) :
	mWorld(world),
	mLastRenderedAtSize(Device::IsHeadless() ? glm::vec2{} : Device::Get().GetDisplaySize())
{
}

CE::WorldViewport::~WorldViewport() = default;

glm::vec3 CE::WorldViewport::GetScreenToWorldDirection(glm::vec2 screenPosition) const
{
	const entt::entity cameraOwner = CameraComponent::GetSelected(mWorld);

    if (cameraOwner == entt::null)
    {
		return sForward;
    }

	const auto camera = mWorld.get().GetRegistry().Get<CameraComponent>(cameraOwner);

	screenPosition -= mLastRenderedAtPos;
	const glm::mat4& invMat = glm::inverse(camera.mViewProjection);
	const glm::vec4 nearVec = glm::vec4((screenPosition.x - (mLastRenderedAtSize.x * .5f)) / (mLastRenderedAtSize.x * .5f), -1 * (screenPosition.y - (mLastRenderedAtSize.y * .5f)) / (mLastRenderedAtSize.y * .5f), -1, 1.0);
	const glm::vec4 farVec = glm::vec4((screenPosition.x - (mLastRenderedAtSize.x * .5f)) / (mLastRenderedAtSize.x * .5f), -1 * (screenPosition.y - (mLastRenderedAtSize.y * .5f)) / (mLastRenderedAtSize.y * .5f), 1, 1.0);
	glm::vec4 nearResult = invMat * nearVec;
	glm::vec4 farResult = invMat * farVec;
	nearResult /= nearResult.w;
	farResult /= farResult.w;
	const glm::vec3 dir = glm::normalize(glm::vec3(farResult) - glm::vec3(nearResult));

	return dir;
}

glm::vec3 CE::WorldViewport::ScreenToWorld(glm::vec2 screenPosition, float distanceFromCamera) const
{
	const entt::entity cameraOwner = CameraComponent::GetSelected(mWorld);

	if (cameraOwner == entt::null)
	{
		return {};
	}

	glm::vec3 dir = GetScreenToWorldDirection(screenPosition);

	if (glm::isnan(dir.x)
		|| glm::isnan(dir.y)
		|| glm::isnan(dir.z))
	{
		return {};
	}

	const TransformComponent* transform = mWorld.get().GetRegistry().TryGet<TransformComponent>(cameraOwner);
	const glm::vec3 camPosition = transform == nullptr ? glm::vec3{} : transform->GetWorldPosition();

	return camPosition + dir * distanceFromCamera;
}

glm::vec3 CE::WorldViewport::ScreenToWorldPlane(glm::vec2 screenPosition, float planeHeight) const
{
	const entt::entity cameraOwner = CameraComponent::GetSelected(mWorld);

	if (cameraOwner == entt::null)
	{
		return {};
	}

	const glm::vec3 dir = GetScreenToWorldDirection(screenPosition);

	if (glm::isnan(dir.x)
		|| glm::isnan(dir.y)
		|| glm::isnan(dir.z))
	{
		return {};
	}

	const TransformComponent* cameraTransform = mWorld.get().GetRegistry().TryGet<TransformComponent>(cameraOwner);

	if (cameraTransform == nullptr)
	{
		return {};
	}

	const glm::vec3 rayStart = cameraTransform->GetWorldPosition();
	const glm::vec3 planePoint{ 0.0f, planeHeight, 0.0f };

	static constexpr glm::vec3 planeNormal{ 0.0f, 1.0f, 0.0f };

	const glm::vec3 difference = planePoint - rayStart;
	const float product_1 = glm::dot(difference, planeNormal);
	const float product_2 = glm::dot(dir, planeNormal);
	const float distance_from_origin_to_plane = product_1 / product_2;
	const glm::vec3 intersection = rayStart + dir * distance_from_origin_to_plane;

	return intersection;
}

void CE::WorldViewport::UpdateSize(glm::vec2 size, glm::vec2 pos)
{
	mLastRenderedAtSize = size;
	mLastRenderedAtPos = pos;
}
