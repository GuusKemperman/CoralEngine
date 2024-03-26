#pragma once
#include "Meta/MetaReflect.h"
#include "Utilities/Math.h"

namespace Engine
{	
	class World;
	class BinaryGSONObject;

	class Axis
	{
	public:
		enum Values
		{
			X,
			Y,
			Z,
			Right = X,
			Up = Y,
			Forward = Z,
		};
	};

	constexpr glm::vec3 ToVector3(Axis::Values axis) { glm::vec3 v{}; v[static_cast<int>(axis)] = 1.0f; return v; }

	constexpr glm::vec2 To2DRightForward(glm::vec3 v3)
	{
		return { v3[Axis::Right], v3[Axis::Forward] };
	}

	constexpr glm::vec3 To3DRightForward(glm::vec2 v2, float up = 0.0f)
	{
		glm::vec3 v3{};
		v3[Axis::Right] = v2.x;
		v3[Axis::Forward] = v2.y;
		v3[Axis::Up] = up;
		return v3;
	}

	constexpr glm::vec3 sForward = ToVector3(Axis::Forward);
	constexpr glm::vec3 sRight = ToVector3(Axis::Right);
	constexpr glm::vec3 sUp = ToVector3(Axis::Up);



	/**
	 * \brief A component that manages the position, scale and orientation of an entity.
	 */
	class TransformComponent
	{
	public:
		~TransformComponent();

		void OnConstruct(World& world, entt::entity owner);

		static glm::mat4 ToMatrix(glm::vec3 position, glm::vec3 scale, glm::quat orientation);

		glm::mat4 GetLocalMatrix() const;		
		void SetLocalMatrix(const glm::mat4& matrix);

		glm::mat4 GetWorldMatrix() const;		
		void SetWorldMatrix(const glm::mat4& matrix);

		// -----------------------------------------------------------------------------------------------------------------//
		// Parental relation ships																							//
		// -----------------------------------------------------------------------------------------------------------------//
		
		void SetParent(TransformComponent* parent, bool keepWorld = false);
		
		const TransformComponent* GetParent() const { return mParent; }
		
		const std::vector<std::reference_wrapper<TransformComponent>>& GetChildren() const { return mChildren; }

		bool IsOrphan() const { return mParent == nullptr; }

		/**
		 * \brief Recursively checks if this transformcomponent is a child of the provided transform.
		 * \return True if the provided transformcomponent is this transform is the child of the forefather,
		 * or if this transform's parent is a child of that forefather, or if the parent of this transform's parent
		 * is a forefather, etc.
		 */
		bool IsAForeFather(const TransformComponent& potentialForeFather) const;

		entt::entity GetOwner() const { return mOwner; }

		// -----------------------------------------------------------------------------------------------------------------//
		// Getting/setting the position																						//
		// -----------------------------------------------------------------------------------------------------------------//
		
		glm::vec3 GetLocalPosition() const { return mLocalPosition; }
		glm::vec2 GetLocalPosition2D() const { return To2DRightForward(mLocalPosition); }

		glm::vec3 GetWorldPosition() const;
		glm::vec2 GetWorldPosition2D() const { return To2DRightForward(GetWorldPosition()); }

		void SetLocalPosition(const glm::vec3 position) { mLocalPosition = position; }
		void SetLocalPosition(const glm::vec2 position) { mLocalPosition = To3DRightForward(position, mLocalPosition[Axis::Up]); }

		void TranslateLocalPosition(const glm::vec3 translation) { mLocalPosition += translation; }
		void TranslateLocalPosition(const glm::vec2 translation) { mLocalPosition += To3DRightForward(translation); }

		void SetWorldPosition(glm::vec3 position);
		void SetWorldPosition(const glm::vec2 position) { SetWorldPosition(To3DRightForward(position, GetWorldPosition()[Axis::Up])); }

		void TranslateWorldPosition(const glm::vec3 translation) { SetWorldPosition(GetWorldPosition() + translation); }
		void TranslateWorldPosition(const glm::vec2 translation) { TranslateWorldPosition(To3DRightForward(translation)); }

		// -----------------------------------------------------------------------------------------------------------------//
		// Getting/setting the orientation																					//
		// -----------------------------------------------------------------------------------------------------------------//
		
		glm::quat GetLocalOrientation() const { return mLocalOrientation; }
		glm::vec3 GetLocalOrientationEuler() const { return eulerAngles(mLocalOrientation); }

		glm::quat GetWorldOrientation() const;
		glm::vec3 GetWorldOrientationEuler() const { return eulerAngles(GetWorldOrientation()); }

		// In radians
		void SetLocalOrientation(const glm::vec3 rotationEuler) { mLocalOrientation = glm::quat{ rotationEuler }; }		
		void SetLocalOrientation(const glm::quat rotation) { mLocalOrientation = rotation; }

		// In radians
		void SetWorldOrientation(const glm::vec3 rotationEuler) { SetWorldOrientation(glm::quat{ rotationEuler }); }
		void SetWorldOrientation(glm::quat orientation);

		glm::vec3 GetLocalForward() const { return GetLocalAxis(Axis::Forward); }
		glm::vec3 GetLocalUp() const { return GetLocalAxis(Axis::Up);}
		glm::vec3 GetLocalRight() const { return GetLocalAxis(Axis::Right); }
		
		glm::vec3 GetLocalAxis(const Axis::Values axis) const { return Math::RotateVector(ToVector3(axis), mLocalOrientation); }

		glm::vec3 GetWorldForward() const { return GetWorldAxis(Axis::Forward); }
		glm::vec3 GetWorldUp() const { return GetWorldAxis(Axis::Up); }
		glm::vec3 GetWorldRight() const { return GetWorldAxis(Axis::Right); }
		glm::vec3 GetWorldAxis(const Axis::Values axis) const { return Math::RotateVector(ToVector3(axis), GetWorldOrientation()); }

		void SetLocalForward(const glm::vec3& forward) { SetLocalOrientation(Math::CalculateRotationBetweenOrientations(sForward, forward)); }
		void SetLocalUp(const glm::vec3& up) { SetLocalOrientation(Math::CalculateRotationBetweenOrientations(sUp, up)); }
		void SetLocalRight(const glm::vec3& right) { SetLocalOrientation(Math::CalculateRotationBetweenOrientations(sRight, right)); }

		void SetWorldForward(const glm::vec3& forward) { SetWorldOrientation(Math::CalculateRotationBetweenOrientations(sForward, forward)); }		
		void SetWorldUp(const glm::vec3& up) { SetWorldOrientation(Math::CalculateRotationBetweenOrientations(sUp, up)); }
		void SetWorldRight(const glm::vec3& right) { SetWorldOrientation(Math::CalculateRotationBetweenOrientations(sRight, right)); }

		// -----------------------------------------------------------------------------------------------------------------//
		// Getting/setting the Scale																						//
		// -----------------------------------------------------------------------------------------------------------------//

		glm::vec3 GetLocalScale() const { return mLocalScale; }
		glm::vec2 GetLocalScale2D() const { return To2DRightForward(mLocalScale); }
		float GetLocalScaleUniform() const { const glm::vec3 scale = GetLocalScale(); return (scale.x + scale.y + scale.z) * (1.0f / 3.0f); }

		glm::vec3 GetWorldScale() const;
		glm::vec2 GetWorldScale2D() const { return To2DRightForward(GetWorldScale()); }
		float GetWorldScaleUniform() const { const glm::vec3 scale = GetWorldScale(); return (scale.x + scale.y + scale.z) * (1.0f / 3.0f); }
		float GetWorldScaleUniform2D() const { const glm::vec2 scale = GetWorldScale2D(); return (scale.x + scale.y) * (1.0f / 2.0f); }
	
		void SetLocalScale(const float xyz) { mLocalScale = glm::vec3{ xyz }; }
		void SetLocalScaleRightForward(const float scale) { mLocalScale = glm::vec3{ scale, mLocalScale.y, scale }; }
		void SetLocalScale(const glm::vec3 scale) { mLocalScale = scale; }
		void SetLocalScale(const glm::vec2 scale) { mLocalScale = To3DRightForward(scale, mLocalScale[Axis::Up]); }
		
		void SetWorldScale(const float xyz) { SetWorldScale(glm::vec3{ xyz }); }
		void SetWorldScale(glm::vec3 scale);
		void SetWorldScale(const glm::vec2 scale) { SetWorldScale(To3DRightForward(scale, GetWorldScale()[Axis::Up])); }
		
	private:
		void AttachChild(TransformComponent& child);
		void DetachChild(TransformComponent& child);

		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(TransformComponent);

		glm::vec3 mLocalPosition{};

		entt::entity mOwner{};		
		glm::quat mLocalOrientation = { 1.0f, 0.0f, 0.0f, 0.0f };
		glm::vec3 mLocalScale = { 1.0f, 1.0f, 1.0f };

		// Storing a pointer is safe, as pointer stability has been enabled.
		TransformComponent* mParent{};

		// Storing pointers is safe, as pointer stability has been enabled for this component.
		std::vector<std::reference_wrapper<TransformComponent>> mChildren{}; 
	};
}

template<>
struct entt::component_traits<Engine::TransformComponent, void>
{
	using type = Engine::TransformComponent;

	// TransformComponent is often accessed in random order so we get little 
	// benefit from compact arrangements. But we do VERY often access our parent,
	// so not having to look up the adress each time is very useful.
	static constexpr bool in_place_delete = true;

	static constexpr std::size_t page_size = internal::page_size<Engine::TransformComponent>::value;
};
