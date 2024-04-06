#include "Precomp.h"
#include "Components/TransformComponent.h"

#include "GSON/GSONBinary.h"
#include "World/World.h"
#include "World/Registry.h"
#include "Meta/MetaType.h"
#include "Meta/MetaProps.h"
#include "Utilities/Reflect/ReflectComponentType.h"
#include "Meta/ReflectedTypes/STD/ReflectVector.h"

namespace
{
	void DecomposeMatrixToComponents(const float* matrix, float* translation, float* rotation, float* scale);
}

CE::TransformComponent::~TransformComponent()
{
	SetParent(nullptr);

	// The children deattach themselves from their parent, to prevent modifying the array while iterating over it, make a copy.
	const std::vector<std::reference_wrapper<TransformComponent>> childrenCopy = mChildren;

	for (TransformComponent& child : childrenCopy)
	{
		child.SetParent(nullptr);
	}
}

void CE::TransformComponent::OnConstruct(World&, entt::entity owner)
{
	mOwner = owner;
}

glm::mat4 CE::TransformComponent::ToMatrix(const glm::vec3 position, const glm::vec3 scale, const glm::quat orientation)
{
	// Scales first, rotates second, translates last.
	const glm::mat4 translationMatrix = translate(glm::mat4{ 1.0f }, position);
	const glm::mat4 scaleMatrix = glm::scale(glm::mat4{ 1.0f }, scale);
	const glm::mat4 rotationMatrix = toMat4(orientation);

	return translationMatrix * rotationMatrix * scaleMatrix;
}

glm::mat4 CE::TransformComponent::GetLocalMatrix() const
{
	return ToMatrix(mLocalPosition, mLocalScale, mLocalOrientation);
}

void CE::TransformComponent::SetLocalMatrix(const glm::mat4& matrix)
{
	glm::vec3 eulerOrientationDegrees{};

	DecomposeMatrixToComponents(&matrix[0][0],
	                                      value_ptr(mLocalPosition),
	                                      value_ptr(eulerOrientationDegrees),
	                                      value_ptr(mLocalScale));

	SetLocalOrientation(eulerOrientationDegrees * (TWOPI / 360.0f));
}

glm::mat4 CE::TransformComponent::GetWorldMatrix() const
{
	if (mParent == nullptr)
	{
		return GetLocalMatrix();
	}

	return mParent->GetWorldMatrix() * GetLocalMatrix();
}

void CE::TransformComponent::SetWorldMatrix(const glm::mat4& matrix)
{
	glm::vec3 translation{};
	glm::vec3 eulerOrientation{};
	glm::vec3 scale{};

	DecomposeMatrixToComponents(&matrix[0][0], value_ptr(translation), value_ptr(eulerOrientation), value_ptr(scale));

	SetWorldPosition(translation);
	SetWorldOrientation(eulerOrientation * (TWOPI / 360.0f));
	SetWorldScale(scale);
}

#ifdef _MSC_VER
#pragma warning(push)
// Potentially uninitialized local variable 'worldPositionToRestore' used
// no it's not.
#pragma warning(disable : 4701)
#endif


void CE::TransformComponent::SetParent(TransformComponent* const parent, const bool preserveWorld)
{
	if (mParent == parent
		|| parent == this)
	{
		return;
	}

	// If we are being parented against one of our own children
	if (parent != nullptr
		&& parent->IsAForeFather(*this))
	{
		parent->SetParent(nullptr);
	}

	glm::vec3 worldPositionToRestore;
	glm::quat worldOrientationToRestore;
	glm::vec3 worldScaleToRestore;

	if (preserveWorld)
	{
		worldPositionToRestore = GetWorldPosition();
		worldOrientationToRestore = GetWorldOrientation();
		worldScaleToRestore = GetWorldScale();
	}

	if (mParent != nullptr)
	{
		mParent->DetachChild(*this);
	}

	mParent = parent;

	if (mParent != nullptr)
	{
		mParent->AttachChild(*this);
	}

	if (preserveWorld)
	{
		SetWorldPosition(worldPositionToRestore);
		SetWorldOrientation(worldOrientationToRestore);
		SetWorldScale(worldScaleToRestore);
	}
}

#ifdef _MSC_VER
#pragma warning(pop)
#endif

bool CE::TransformComponent::IsAForeFather(const TransformComponent& potentialForeFather) const
{
	if (mParent == nullptr)
	{
		return false;
	}
	if (mParent == &potentialForeFather)
	{
		return true;
	}
	return mParent->IsAForeFather(potentialForeFather);
}

glm::vec3 CE::TransformComponent::GetWorldPosition() const
{
	return mParent == nullptr ? mLocalPosition : GetWorldMatrix() * glm::vec4{ 0.0f, 0.0f, 0.0f, 1.0f };
}

void CE::TransformComponent::SetWorldPosition(const glm::vec3 position)
{
	if (mParent == nullptr)
	{
		SetLocalPosition(position);
	}
	else
	{
		const glm::mat4 parentInvWorldMatrix = inverse(mParent->GetWorldMatrix());
		SetLocalPosition(glm::vec3{ parentInvWorldMatrix * glm::vec4(position, 1.0f) });
	}
}

glm::quat CE::TransformComponent::GetWorldOrientation() const
{
	if (mParent == nullptr)
	{
		return GetLocalOrientation();
	}

	return mParent->GetWorldOrientation() * GetLocalOrientation();
}

void CE::TransformComponent::SetWorldOrientation(const glm::quat orientation)
{
	if (mParent == nullptr)
	{
		SetLocalOrientation(orientation);
	}
	else
	{
		const glm::quat parentWorldOrienation = mParent->GetWorldOrientation();
		SetLocalOrientation(inverse(parentWorldOrienation) * orientation);
	}
}

glm::vec3 CE::TransformComponent::GetWorldScale() const
{
	if (mParent == nullptr)
	{
		return GetLocalScale();
	}
	return mParent->GetWorldScale() * GetLocalScale();
}

void CE::TransformComponent::SetWorldScale(const glm::vec3 scale)
{
	if (mParent == nullptr)
	{
		SetLocalScale(scale);
	}
	else
	{
		const glm::vec3 parentScale = mParent->GetWorldScale();
		SetLocalScale(scale / parentScale);
	}
}

void CE::TransformComponent::AttachChild(TransformComponent& child)
{
	//ASSERT(!IsAForeFather(child) && "Cannot attach a parent to its child");
	mChildren.push_back(child);
}

void CE::TransformComponent::DetachChild(TransformComponent& child)
{
	const auto it = std::find_if(mChildren.begin(), mChildren.end(), 
		[&child](const TransformComponent& transform)
		{
			return &child == &transform;
		});
	ASSERT(it != mChildren.end() && "Cannot detach child, it was never attached");

	mChildren.erase(it);
}

CE::MetaType CE::TransformComponent::Reflect()
{
	MetaType type = MetaType{ MetaType::T<TransformComponent>{}, "TransformComponent" };
	MetaProps& props = type.GetProperties();
	props.Add(Props::sIsScriptableTag);
	type.AddField(&TransformComponent::mLocalPosition, "mLocalPosition");
	type.AddField(&TransformComponent::mLocalOrientation, "mLocalOrientation");
	type.AddField(&TransformComponent::mLocalScale, "mLocalScale");
	type.AddFunc(&TransformComponent::GetLocalMatrix, "GetLocalMatrix", "").GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc(&TransformComponent::SetLocalMatrix, "SetLocalMatrix", "", "matrix").GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc(&TransformComponent::GetWorldMatrix, "GetWorldMatrix", "").GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc(&TransformComponent::SetWorldMatrix, "SetWorldMatrix", "", "matrix").GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc(&TransformComponent::SetParent, "SetParent", "", "parent", "keepWorld").GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc(&TransformComponent::GetParent, "GetParent", "").GetProperties().Add(Props::sIsScriptableTag);

	type.AddFunc([](TransformComponent& transform)
		{
			auto references = transform.GetChildren();

			std::vector<entt::entity> owners(references.size());

			for (size_t i = 0; i < references.size(); i++)
			{
				owners[i] = references[i].get().GetOwner();
			}

			return owners;
		}, "GetChildren", MetaFunc::ExplicitParams<TransformComponent&>{}, "").GetProperties().Add(Props::sIsScriptableTag);


	type.AddFunc(&TransformComponent::IsOrphan, "IsOrphan", "").GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc(&TransformComponent::IsAForeFather, "IsAForeFather", "", "potentialForeFather").GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc(&TransformComponent::GetOwner, "GetOwner", "").GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc(&TransformComponent::GetLocalPosition, "GetLocalPosition", "").GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc(&TransformComponent::GetLocalPosition2D, "GetLocalPosition2D", "").GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc(&TransformComponent::GetWorldPosition, "GetWorldPosition", "").GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc(&TransformComponent::GetWorldPosition2D, "GetWorldPosition2D", "").GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc(static_cast<void (TransformComponent::*)(glm::vec3)>(&TransformComponent::SetLocalPosition), "SetLocalPosition", "", "position").GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc(static_cast<void (TransformComponent::*)(glm::vec3)>(&TransformComponent::TranslateLocalPosition), "TranslateLocalPosition", "", "translation").GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc(static_cast<void (TransformComponent::*)(glm::vec3)>(&TransformComponent::SetWorldPosition), "SetWorldPosition", "", "position").GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc(static_cast<void (TransformComponent::*)(glm::vec3)>(&TransformComponent::TranslateWorldPosition), "TranslateWorldPosition", "", "translation").GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc(&TransformComponent::GetLocalOrientation, "GetLocalOrientation", "").GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc(&TransformComponent::GetLocalOrientationEuler, "GetLocalOrientationEuler", "").GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc(&TransformComponent::GetWorldOrientation, "GetWorldOrientation", "").GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc(&TransformComponent::GetWorldOrientationEuler, "GetWorldOrientationEuler", "").GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc(static_cast<void (TransformComponent::*)(glm::quat)>(&TransformComponent::SetLocalOrientation), "SetLocalOrientation", "", "rotation").GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc(static_cast<void (TransformComponent::*)(glm::quat)>(&TransformComponent::SetWorldOrientation), "SetWorldOrientation", "", "orientation").GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc(&TransformComponent::GetLocalForward, "GetLocalForward", "").GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc(&TransformComponent::GetLocalUp, "GetLocalUp", "").GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc(&TransformComponent::GetLocalRight, "GetLocalRight", "").GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc(&TransformComponent::GetWorldForward, "GetWorldForward", "").GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc(&TransformComponent::GetWorldUp, "GetWorldUp", "").GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc(&TransformComponent::GetWorldRight, "GetWorldRight", "").GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc(&TransformComponent::SetLocalForward, "SetLocalForward", "", "forward").GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc(&TransformComponent::SetLocalUp, "SetLocalUp", "", "up").GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc(&TransformComponent::SetLocalRight, "SetLocalRight", "", "right").GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc(&TransformComponent::SetWorldForward, "SetWorldForward", "", "forward").GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc(&TransformComponent::SetWorldUp, "SetWorldUp", "", "up").GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc(&TransformComponent::SetWorldRight, "SetWorldRight", "", "right").GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc(&TransformComponent::GetLocalScale, "GetLocalScale", "").GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc(&TransformComponent::GetLocalScale2D, "GetLocalScale2D", "").GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc(&TransformComponent::GetLocalScaleUniform, "GetLocalScaleUniform", "").GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc(&TransformComponent::GetWorldScale, "GetWorldScale", "").GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc(&TransformComponent::GetWorldScale2D, "GetWorldScale2D", "").GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc(&TransformComponent::GetWorldScaleUniform, "GetWorldScaleUniform", "").GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc(static_cast<void (TransformComponent::*)(glm::vec3)>(&TransformComponent::SetLocalScale), "SetLocalScale", "", "scale").GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc(static_cast<void (TransformComponent::*)(glm::vec3)>(&TransformComponent::SetWorldScale), "SetWorldScale", "", "scale").GetProperties().Add(Props::sIsScriptableTag);

	BindEvent(type, sConstructEvent, &TransformComponent::OnConstruct);

	ReflectComponentType<TransformComponent>(type);
	return type;
}

namespace
{
	// Stolen from imguizmo
	void DecomposeMatrixToComponents(const float* matrix, float* translation, float* rotation, float* scale)
	{
		struct matrix_t
			{
			public:

				union
				{
					float m[4][4];
					float m16[16];
					struct
					{
						glm::vec4 right, up, dir, position;
					} v;
					glm::vec4 component[4];
				};
			};

			matrix_t mat = *(matrix_t*)matrix;

			scale[0] = glm::length(mat.v.right);
			scale[1] = glm::length(mat.v.up);
			scale[2] = glm::length(mat.v.dir);

			mat.v.right = glm::normalize(mat.v.right);
			mat.v.up = glm::normalize(mat.v.up);
			mat.v.dir = glm::normalize(mat.v.dir);

			static constexpr float ZPI = 3.14159265358979323846f;
			static constexpr float rad2deg = (180.f / ZPI);

			rotation[0] = rad2deg * atan2f(mat.m[1][2], mat.m[2][2]);
			rotation[1] = rad2deg * atan2f(-mat.m[0][2], sqrtf(mat.m[1][2] * mat.m[1][2] + mat.m[2][2] * mat.m[2][2]));
			rotation[2] = rad2deg * atan2f(mat.m[0][1], mat.m[0][0]);

			translation[0] = mat.v.position.x;
			translation[1] = mat.v.position.y;
			translation[2] = mat.v.position.z;
	}
}