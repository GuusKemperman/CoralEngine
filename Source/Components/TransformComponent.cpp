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

CE::TransformComponent::TransformComponent(TransformComponent&& other) noexcept :
	mOwner(other.mOwner)
{
	SetLocalPosition(other.mLocalPosition);
	SetLocalOrientation(other.mLocalOrientation);
	SetLocalScale(other.mLocalScale);
	SetParent(other.mParent);
	other.SetParent(nullptr);
}

CE::TransformComponent::TransformComponent(const TransformComponent& other) noexcept :
	mLocalPosition(other.mLocalPosition),
	mOwner(other.mOwner),
	mLocalOrientation(other.mLocalOrientation),
	mLocalScale(other.mLocalScale)
{
	SetParent(other.mParent);
}

CE::TransformComponent::~TransformComponent()
{
	// The children detach themselves from their parent, to prevent modifying the array while iterating over it, make a copy.
	const std::vector<std::reference_wrapper<TransformComponent>> childrenCopy = mChildren;

	for (TransformComponent& child : childrenCopy)
	{
		child.SetParent(nullptr, true);
	}
	SetParent(nullptr);
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

std::tuple<glm::vec3, glm::vec3, glm::quat> CE::TransformComponent::FromMatrix(const glm::mat4& matrix)
{
	glm::vec3 eulerOrientationDegrees{};
	glm::vec3 position{};
	glm::vec3 scale{};

	DecomposeMatrixToComponents(&matrix[0][0],
		value_ptr(position),
		value_ptr(eulerOrientationDegrees),
		value_ptr(scale));

	return { position, scale, glm::radians(eulerOrientationDegrees) };
}

glm::mat4 CE::TransformComponent::GetLocalMatrix() const
{
	return ToMatrix(mLocalPosition, mLocalScale, mLocalOrientation);
}

void CE::TransformComponent::SetLocalMatrix(const glm::mat4& matrix)
{
	const auto [pos, scale, orientation] = FromMatrix(matrix);

	// TODO can be optimized to only call world matrix once
	SetLocalPosition(pos);
	SetLocalOrientation(orientation);
	SetLocalScale(scale);
}

const glm::mat4& CE::TransformComponent::GetWorldMatrix() const
{
	return mCachedWorldMatrix;
}

void CE::TransformComponent::SetWorldMatrix(const glm::mat4& matrix)
{
	const auto [pos, scale, orientation] = FromMatrix(matrix);

	// TODO can be optimized to only call world matrix once
	SetWorldPosition(pos);
	SetWorldOrientation(orientation);
	SetWorldScale(scale);
}

std::tuple<glm::vec3, glm::vec3, glm::quat> CE::TransformComponent::GetLocalPositionScaleOrientation() const
{
	return { GetLocalPosition(), GetLocalScale(), GetLocalOrientation() };
}

std::tuple<glm::vec3, glm::vec3, glm::quat> CE::TransformComponent::GetWorldPositionScaleOrientation() const
{
	return mParent == nullptr ? GetLocalPositionScaleOrientation() : FromMatrix(mCachedWorldMatrix);
}

void CE::TransformComponent::SetLocalPositionScaleOrientation(glm::vec3 position, glm::vec3 scale, glm::quat orientation)
{
	SetLocalMatrix(ToMatrix(position, scale, orientation));
}

void CE::TransformComponent::SetWorldPositionScaleOrientation(glm::vec3 position, glm::vec3 scale, glm::quat orientation)
{
	SetWorldMatrix(ToMatrix(position, scale, orientation));
}

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

	glm::vec3 worldPositionToRestore{};
	glm::quat worldOrientationToRestore{};
	glm::vec3 worldScaleToRestore{};

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
		// TODO only calculate world matrix once
		SetWorldPosition(worldPositionToRestore);
		SetWorldOrientation(worldOrientationToRestore);
		SetWorldScale(worldScaleToRestore);
	}
	else
	{
		UpdateCachedWorldMatrix();
	}
}

const CE::TransformComponent* CE::TransformComponent::GetParent() const
{ 
	return mParent;
}

const std::vector<std::reference_wrapper<CE::TransformComponent>>& CE::TransformComponent::GetChildren() const
{
	return mChildren;
}

bool CE::TransformComponent::IsOrphan() const
{
	return mParent == nullptr;
}

entt::entity CE::TransformComponent::GetOwner() const
{
	return mOwner;
}

glm::vec3 CE::TransformComponent::GetLocalPosition() const
{
	return mLocalPosition;
}

glm::vec2 CE::TransformComponent::GetLocalPosition2D() const
{
	return To2DRightForward(mLocalPosition);
}

glm::vec2 CE::TransformComponent::GetWorldPosition2D() const
{
	return To2DRightForward(GetWorldPosition());
}

void CE::TransformComponent::SetLocalPosition(const glm::vec3 position)
{
	if (mLocalPosition == position)
	{
		return;
	}

	mLocalPosition = position;
	UpdateCachedWorldMatrix();
}

void CE::TransformComponent::SetLocalPosition(const glm::vec2 position)
{
	SetLocalPosition(To3DRightForward(position, GetLocalPosition()[Axis::Up]));
}

void CE::TransformComponent::TranslateLocalPosition(const glm::vec3 translation)
{
	SetLocalPosition(GetLocalPosition() + translation);
}

void CE::TransformComponent::TranslateLocalPosition(const glm::vec2 translation)
{
	TranslateLocalPosition(To3DRightForward(translation));
}

void CE::TransformComponent::SetWorldPosition(const glm::vec2 position)
{
	SetWorldPosition(To3DRightForward(position, GetWorldPosition()[Axis::Up]));
}

void CE::TransformComponent::TranslateWorldPosition(const glm::vec3 translation)
{
	SetWorldPosition(GetWorldPosition() + translation);
}

void CE::TransformComponent::TranslateWorldPosition(const glm::vec2 translation)
{
	TranslateWorldPosition(To3DRightForward(translation));
}

glm::quat CE::TransformComponent::GetLocalOrientation() const
{
	return mLocalOrientation;
}

glm::vec3 CE::TransformComponent::GetLocalOrientationEuler() const
{
	return eulerAngles(GetLocalOrientation());
}

glm::vec3 CE::TransformComponent::GetWorldOrientationEuler() const
{
	return eulerAngles(GetWorldOrientation());
}

void CE::TransformComponent::SetLocalOrientation(const glm::vec3 rotationEuler)
{
	SetLocalOrientation(glm::quat{ rotationEuler });
}

void CE::TransformComponent::SetLocalOrientation(const glm::quat rotation)
{
	if (rotation == mLocalOrientation)
	{
		return;
	}

	mLocalOrientation = rotation;
	UpdateCachedWorldMatrix();
}

void CE::TransformComponent::SetWorldOrientation(const glm::vec3 rotationEuler)
{
	SetWorldOrientation(glm::quat{ rotationEuler });
}

glm::vec3 CE::TransformComponent::GetLocalForward() const
{
	return GetLocalAxis(Axis::Forward);
}

glm::vec3 CE::TransformComponent::GetLocalUp() const
{
	return GetLocalAxis(Axis::Up);
}

glm::vec3 CE::TransformComponent::GetLocalRight() const
{
	return GetLocalAxis(Axis::Right);
}

glm::vec3 CE::TransformComponent::GetLocalAxis(const Axis::Values axis) const
{
	return Math::RotateVector(ToVector3(axis), GetLocalOrientation());
}

glm::vec3 CE::TransformComponent::GetWorldForward() const
{
	return GetWorldAxis(Axis::Forward);
}

glm::vec3 CE::TransformComponent::GetWorldUp() const
{
	return GetWorldAxis(Axis::Up);
}

glm::vec3 CE::TransformComponent::GetWorldRight() const
{
	return GetWorldAxis(Axis::Right);
}

glm::vec3 CE::TransformComponent::GetWorldAxis(const Axis::Values axis) const
{
	return Math::RotateVector(ToVector3(axis), GetWorldOrientation());
}

void CE::TransformComponent::SetLocalForward(const glm::vec3& forward)
{
	SetLocalOrientation(Math::CalculateRotationBetweenOrientations(sForward, forward));
}

void CE::TransformComponent::SetLocalUp(const glm::vec3& up)
{
	SetLocalOrientation(Math::CalculateRotationBetweenOrientations(sUp, up));
}

void CE::TransformComponent::SetLocalRight(const glm::vec3& right)
{
	SetLocalOrientation(Math::CalculateRotationBetweenOrientations(sRight, right));
}

void CE::TransformComponent::SetWorldForward(const glm::vec3& forward)
{
	SetWorldOrientation(Math::CalculateRotationBetweenOrientations(sForward, forward));
}

void CE::TransformComponent::SetWorldUp(const glm::vec3& up)
{
	SetWorldOrientation(Math::CalculateRotationBetweenOrientations(sUp, up));
}

void CE::TransformComponent::SetWorldRight(const glm::vec3& right)
{
	SetWorldOrientation(Math::CalculateRotationBetweenOrientations(sRight, right));
}

glm::vec3 CE::TransformComponent::GetLocalScale() const
{
	return mLocalScale;
}

glm::vec2 CE::TransformComponent::GetLocalScale2D() const
{
	return To2DRightForward(GetLocalScale());
}

float CE::TransformComponent::GetLocalScaleUniform() const
{
	const glm::vec3 scale = GetLocalScale();
	return (scale.x + scale.y + scale.z) * (1.0f / 3.0f);
}

glm::vec2 CE::TransformComponent::GetWorldScale2D() const
{
	return To2DRightForward(GetWorldScale());
}

float CE::TransformComponent::GetWorldScaleUniform() const
{
	const glm::vec3 scale = GetWorldScale();
	return (scale.x + scale.y + scale.z) * (1.0f / 3.0f);
}

float CE::TransformComponent::GetWorldScaleUniform2D() const
{
	const glm::vec2 scale = GetWorldScale2D();
	return (scale.x + scale.y) * (1.0f / 2.0f);
}

void CE::TransformComponent::SetLocalScale(const float xyz)
{
	SetLocalScale(glm::vec3{ xyz });
}

void CE::TransformComponent::SetLocalScaleRightForward(const float scale)
{
	SetLocalScale(glm::vec3{ scale, GetLocalScale().y, scale });
}

void CE::TransformComponent::SetLocalScale(const glm::vec3 scale)
{
	if (mLocalScale == scale)
	{
		return;
	}

	mLocalScale = scale;
	UpdateCachedWorldMatrix();
}

void CE::TransformComponent::SetLocalScale(const glm::vec2 scale)
{
	SetLocalScale(To3DRightForward(scale, GetLocalScale()[Axis::Up]));
}

void CE::TransformComponent::SetWorldScale(const float xyz)
{
	SetWorldScale(glm::vec3{ xyz });
}

void CE::TransformComponent::SetWorldScale(const glm::vec2 scale)
{
	SetWorldScale(To3DRightForward(scale, GetWorldScale()[Axis::Up]));
}

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
	return mParent == nullptr ? GetLocalPosition() : GetWorldMatrix() * glm::vec4{ 0.0f, 0.0f, 0.0f, 1.0f };
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
	return std::get<2>(GetWorldPositionScaleOrientation());
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
	return std::get<1>(GetWorldPositionScaleOrientation());
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

void CE::TransformComponent::UpdateCachedWorldMatrix()
{
	if (mParent == nullptr)
	{
		mCachedWorldMatrix = GetLocalMatrix();
	}
	else
	{
		mCachedWorldMatrix = mParent->GetWorldMatrix() * GetLocalMatrix();
	}

	for (TransformComponent& child : mChildren)
	{
		child.UpdateCachedWorldMatrix();
	}
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

	type.AddFunc([](const TransformComponent& transform)
		{
			auto references = transform.GetChildren();

			std::vector<entt::entity> owners(references.size());

			for (size_t i = 0; i < references.size(); i++)
			{
				owners[i] = references[i].get().GetOwner();
			}

			return owners;
		}, "GetChildren", MetaFunc::ExplicitParams<const TransformComponent&>{}, "").GetProperties().Add(Props::sIsScriptableTag);


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