#pragma once
#include "Meta/MetaReflect.h"
#include "Utilities/Math.h"

namespace CE
{
	class WorldDetails;
	class Archiver;
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
		TransformComponent() = default;

		TransformComponent(TransformComponent&& other) noexcept;
		TransformComponent(const TransformComponent& other) noexcept;

		TransformComponent& operator=(const TransformComponent&) = delete;
		TransformComponent& operator=(TransformComponent&&) = delete;

		~TransformComponent();

		void OnConstruct(World& world, entt::entity owner);

		static glm::mat4 ToMatrix(glm::vec3 position, glm::vec3 scale, glm::quat orientation);
		static std::tuple<glm::vec3, glm::vec3, glm::quat> FromMatrix(const glm::mat4& matrix);

		glm::mat4 GetLocalMatrix() const;		
		void SetLocalMatrix(const glm::mat4& matrix);

		const glm::mat4& GetWorldMatrix() const;		
		void SetWorldMatrix(const glm::mat4& matrix);

		std::tuple<glm::vec3, glm::vec3, glm::quat> GetLocalPositionScaleOrientation() const;
		std::tuple<glm::vec3, glm::vec3, glm::quat> GetWorldPositionScaleOrientation() const;

		void SetLocalPositionScaleOrientation(glm::vec3 position, glm::vec3 scale, glm::quat orientation);
		void SetWorldPositionScaleOrientation(glm::vec3 position, glm::vec3 scale, glm::quat orientation);

		// -----------------------------------------------------------------------------------------------------------------//
		// Parental relation ships																							//
		// -----------------------------------------------------------------------------------------------------------------//
		
		void SetParent(TransformComponent* parent, bool keepWorld = false);
		
		const TransformComponent* GetParent() const;

		const std::vector<std::reference_wrapper<TransformComponent>>& GetChildren() const;

		bool IsOrphan() const;

		/**
		 * \brief Recursively checks if this transformcomponent is a child of the provided transform.
		 * \return True if the provided transformcomponent is this transform is the child of the forefather,
		 * or if this transform's parent is a child of that forefather, or if the parent of this transform's parent
		 * is a forefather, etc.
		 */
		bool IsAForeFather(const TransformComponent& potentialForeFather) const;

		entt::entity GetOwner() const;

		// -----------------------------------------------------------------------------------------------------------------//
		// Getting/setting the position																						//
		// -----------------------------------------------------------------------------------------------------------------//
		
		glm::vec3 GetLocalPosition() const;
		glm::vec2 GetLocalPosition2D() const;

		glm::vec3 GetWorldPosition() const;
		glm::vec2 GetWorldPosition2D() const;

		void SetLocalPosition(const glm::vec3 position);
		void SetLocalPosition(const glm::vec2 position);

		void TranslateLocalPosition(const glm::vec3 translation);
		void TranslateLocalPosition(const glm::vec2 translation);

		void SetWorldPosition(glm::vec3 position);
		void SetWorldPosition(const glm::vec2 position);

		void TranslateWorldPosition(const glm::vec3 translation);
		void TranslateWorldPosition(const glm::vec2 translation);

		// -----------------------------------------------------------------------------------------------------------------//
		// Getting/setting the orientation																					//
		// -----------------------------------------------------------------------------------------------------------------//
		
		glm::quat GetLocalOrientation() const;
		glm::vec3 GetLocalOrientationEuler() const;

		glm::quat GetWorldOrientation() const;
		glm::vec3 GetWorldOrientationEuler() const;

		// In radians
		void SetLocalOrientation(const glm::vec3 rotationEuler);
		void SetLocalOrientation(const glm::quat rotation);

		// In radians
		void SetWorldOrientation(const glm::vec3 rotationEuler);
		void SetWorldOrientation(glm::quat orientation);

		glm::vec3 GetLocalForward() const;
		glm::vec3 GetLocalUp() const;
		glm::vec3 GetLocalRight() const;

		glm::vec3 GetLocalAxis(const Axis::Values axis) const;

		glm::vec3 GetWorldForward() const;
		glm::vec3 GetWorldUp() const;
		glm::vec3 GetWorldRight() const;
		glm::vec3 GetWorldAxis(const Axis::Values axis) const;

		void SetLocalForward(const glm::vec3& forward);
		void SetLocalUp(const glm::vec3& up);
		void SetLocalRight(const glm::vec3& right);

		void SetWorldForward(const glm::vec3& forward);
		void SetWorldUp(const glm::vec3& up);
		void SetWorldRight(const glm::vec3& right);

		// -----------------------------------------------------------------------------------------------------------------//
		// Getting/setting the Scale																						//
		// -----------------------------------------------------------------------------------------------------------------//

		glm::vec3 GetLocalScale() const;
		glm::vec2 GetLocalScale2D() const;
		float GetLocalScaleUniform() const;

		glm::vec3 GetWorldScale() const;
		glm::vec2 GetWorldScale2D() const;
		float GetWorldScaleUniform() const;
		float GetWorldScaleUniform2D() const;

		void SetLocalScale(const float xyz);
		void SetLocalScaleRightForward(const float scale);
		void SetLocalScale(const glm::vec3 scale);
		void SetLocalScale(const glm::vec2 scale);

		void SetWorldScale(const float xyz);
		void SetWorldScale(glm::vec3 scale);
		void SetWorldScale(const glm::vec2 scale);

	private:
		void AttachChild(TransformComponent& child);
		void DetachChild(TransformComponent& child);

		void UpdateCachedWorldMatrix();

		// We do not serialize the world
		// matrix, so we require
		// the archiver to call UpdateCachedWorldMatrix..
		friend Archiver;


		// We don't have custom setter/getter support yet.
		// WorldDetails may inspect mLocalPosition and
		// adjust its value without ever updating the world
		// matrix. 
		friend WorldDetails;

		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(TransformComponent);

		glm::vec3 mLocalPosition{};

		entt::entity mOwner = entt::null;		
		glm::quat mLocalOrientation = { 1.0f, 0.0f, 0.0f, 0.0f };
		glm::vec3 mLocalScale = { 1.0f, 1.0f, 1.0f };

		glm::mat4 mCachedWorldMatrix{ 1.0f };

		// Storing a pointer is safe, as pointer stability has been enabled.
		TransformComponent* mParent{};

		// Storing pointers is safe, as pointer stability has been enabled for this component.
		std::vector<std::reference_wrapper<TransformComponent>> mChildren{}; 
	};
}

template<>
struct entt::component_traits<CE::TransformComponent, void>
{
	using type = CE::TransformComponent;

	// TransformComponent is often accessed in random order so we get little 
	// benefit from compact arrangements. But we do VERY often access our parent,
	// so not having to look up the adress each time is very useful.
	static constexpr bool in_place_delete = true;

	static constexpr std::size_t page_size = internal::page_size<CE::TransformComponent>::value;
};
