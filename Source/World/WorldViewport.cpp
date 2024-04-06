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
	const glm::mat4& invMat = camera.mInvViewProjection;
	const glm::vec4 nearVec = glm::vec4((screenPosition.x - (mLastRenderedAtSize.x * .5f)) / (mLastRenderedAtSize.x * .5f), -1 * (screenPosition.y - (mLastRenderedAtSize.y * .5f)) / (mLastRenderedAtSize.y * .5f), -1, 1.0);
	const glm::vec4 farVec = glm::vec4((screenPosition.x - (mLastRenderedAtSize.x * .5f)) / (mLastRenderedAtSize.x * .5f), -1 * (screenPosition.y - (mLastRenderedAtSize.y * .5f)) / (mLastRenderedAtSize.y * .5f), 1, 1.0);
	glm::vec4 nearResult = invMat * nearVec;
	glm::vec4 farResult = invMat * farVec;
	nearResult /= nearResult.w;
	farResult /= farResult.w;
	const glm::vec3 dir = glm::normalize(glm::vec3(farResult) - glm::vec3(nearResult));

	return normalize(dir);
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

void CE::WorldViewport::UpdateSize(glm::vec2 size)
{
	mLastRenderedAtSize = size;

	// If we are not using the editor, we always
	// render starting from the topleft corner.
#ifdef EDITOR
	mLastRenderedAtPos = ImGui::GetCursorScreenPos();
#endif // EDITOR
}
