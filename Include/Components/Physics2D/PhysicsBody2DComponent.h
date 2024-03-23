#pragma once
#include "Meta/MetaReflect.h"
#include "Utilities/Imgui/ImguiInspect.h"

namespace Engine
{
	class World;
	class PhysicsSystem2D;

	enum class CollisionResponse : uint8
	{
		Ignore,

		/**
		 * \brief If both objects have overlap enabled, the OnCollision events are invoked
		 */
		Overlap,

		/**
		 * \brief If both objects are blocking, the OnCollision events are invoked, and collision resolution will start.
		 */
		Blocking,

		NUM_OF_RESPONSES
	};

	enum class CollisionLayer : uint8
	{
		/**
		 * \brief The terrain, all objects that will never move. Used for pathfinding
		 */
		WorldStatic,

		/**
		 * \brief All obstacles that can be moved, whether that be through physics or by setting the position.
		 */
		WorldDynamic,

		/**
		 * \brief The players and enemies
		 */
		Character,

		NUM_OF_LAYERS
	};

	struct CollisionRules
	{
#ifdef EDITOR
		void DisplayWidget(const std::string& name);
#endif
		constexpr bool operator!=(const CollisionRules& other) const { return mLayer != other.mLayer || mResponses != other.mResponses; }
		constexpr bool operator==(const CollisionRules& other) const { return mLayer == other.mLayer && mResponses == other.mResponses; }

		constexpr CollisionResponse GetResponse(const CollisionRules& other) const;

		CollisionLayer mLayer{};
		std::array<CollisionResponse, static_cast<size_t>(CollisionLayer::NUM_OF_LAYERS)> mResponses{};
	};

	struct CollisionPreset
	{
		std::string_view mName{};
		CollisionRules mRules{};
	};

	namespace CollisionPresets
	{
		static constexpr CollisionPreset sWorldDynamic
		{
			"WorldDynamic",
			{
				CollisionLayer::WorldDynamic,
				{
					CollisionResponse::Blocking,	// WorldStatic
					CollisionResponse::Blocking,	// WorldDynamic
					CollisionResponse::Blocking,	// Character
				}
			}
		};

		static constexpr CollisionPreset sWorldStatic
		{
			"WorldStatic",
			{
				CollisionLayer::WorldStatic,
				{
					CollisionResponse::Ignore,		// WorldStatic
					CollisionResponse::Blocking,	// WorldDynamic
					CollisionResponse::Blocking,	// Character
				}
			}
		};

		static constexpr CollisionPreset sCharacter
		{
			"Character",
			{
				CollisionLayer::Character,
				{
					CollisionResponse::Blocking,	// WorldStatic
					CollisionResponse::Blocking,	// WorldDynamic
					CollisionResponse::Blocking,	// Character
				}
			}
		};
	}

	static constexpr std::array<CollisionPreset, 3> sCollisionPresets
	{
		CollisionPresets::sWorldDynamic,
		CollisionPresets::sWorldStatic,
		CollisionPresets::sCharacter,

	};

	class PhysicsBody2DComponent
	{
	public:
		PhysicsBody2DComponent() = default;
		explicit PhysicsBody2DComponent(float mass, float restitution)
			: mRestitution(restitution)
		{
			mInvMass = mass == 0.f ? 0.f : (1.f / mass);
		}

		CollisionRules mRules = CollisionPresets::sWorldDynamic.mRules;

		float mInvMass = 1.f;
		float mRestitution = 1.f;
		glm::vec2 mLinearVelocity{};
		glm::vec2 mForce{};

		/**
		 * \brief If true, this object can be moved by gravity, forces and impulses. Otherwise, it moves purely according to its velocity.
		 */
		bool mIsAffectedByForces = true;

		void AddForce(glm::vec2 force)
		{
			if (mIsAffectedByForces) mForce += force;
		}

		void ApplyImpulse(glm::vec2 imp)
		{
			if (mIsAffectedByForces) mLinearVelocity += imp * mInvMass;
		}

	private:
		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(PhysicsBody2DComponent);

		friend PhysicsSystem2D;

		void ClearForces() { mForce = {}; }
	};

	constexpr CollisionResponse CollisionRules::GetResponse(const CollisionRules& other) const
	{
		return static_cast<CollisionResponse>(std::min(static_cast<std::underlying_type_t<CollisionResponse>>(mResponses[static_cast<std::underlying_type_t<CollisionLayer>>(other.mLayer)]),
			static_cast<std::underlying_type_t<CollisionResponse>>(other.mResponses[static_cast<std::underlying_type_t<CollisionLayer>>(mLayer)])));
	}
}

namespace cereal
{
	class BinaryOutputArchive;
	class BinaryInputArchive;

	void save(BinaryOutputArchive& ar, const Engine::CollisionRules& value);
	void load(BinaryInputArchive& ar, Engine::CollisionRules& value);
}

#ifdef EDITOR
IMGUI_AUTO_DEFINE_INLINE(template<>, Engine::CollisionRules, var.DisplayWidget(name);)
#endif // EDITOR

template<>
struct Reflector<Engine::CollisionLayer>
{
	static Engine::MetaType Reflect();
	static constexpr bool sIsSpecialized = true;
}; REFLECT_AT_START_UP(collLayer, Engine::CollisionLayer);

template<>
struct Reflector<Engine::CollisionRules>
{
	static Engine::MetaType Reflect();
	static constexpr bool sIsSpecialized = true;
}; REFLECT_AT_START_UP(collRules, Engine::CollisionRules)

template<>
struct Engine::EnumStringPairsImpl<Engine::CollisionLayer>
{
	static constexpr EnumStringPairs<CollisionLayer, static_cast<size_t>(CollisionLayer::NUM_OF_LAYERS)> value = 
	{
		EnumStringPair<CollisionLayer>{ CollisionLayer::WorldStatic, "WorldStatic" },
		{ CollisionLayer::WorldDynamic, "WorldDynamic" },
		{ CollisionLayer::Character, "Character" },
	};
};

template<>
struct Engine::EnumStringPairsImpl<Engine::CollisionResponse>
{
	static constexpr EnumStringPairs<CollisionResponse, static_cast<size_t>(CollisionResponse::NUM_OF_RESPONSES)> value =
	{
		EnumStringPair<CollisionResponse>{ CollisionResponse::Ignore, "Ignore" },
		{ CollisionResponse::Overlap, "Overlap" },
		{ CollisionResponse::Blocking, "Blocking" },
	};
};