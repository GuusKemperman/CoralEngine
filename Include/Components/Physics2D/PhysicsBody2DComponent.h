#pragma once
#include "Meta/MetaReflect.h"
#include "Utilities/Imgui/ImguiInspect.h"

namespace CE
{
	class World;
	class PhysicsSystem;

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
		 * \brief All obstacles that will never move. Used for pathfinding
		 */
		StaticObstacles,

		/**
		 * \brief All objects fired by the player or enemies.
		 */
		Projectiles,

		/**
		 * \brief The players and enemies
		 */
		Character,

		/**
		 * \brief The terrain, used for pathfinding. The AI can walk on this.
		 */
		Terrain,

		/**
		 * \brief For queries, for operations such as finding all objects within a radius.
		 */
		Query,

		NUM_OF_LAYERS
	};

	static constexpr bool IsCollisionLayerStatic(CollisionLayer layer)
	{
		return layer == CollisionLayer::StaticObstacles || layer == CollisionLayer::Terrain;
	}

	struct CollisionRules
	{
#ifdef EDITOR
		void DisplayWidget(const std::string& name);
#endif
		constexpr bool operator!=(const CollisionRules& other) const { return mLayer != other.mLayer || mResponses != other.mResponses; }
		constexpr bool operator==(const CollisionRules& other) const { return mLayer == other.mLayer && mResponses == other.mResponses; }

		constexpr CollisionResponse GetResponse(const CollisionRules& other) const;

		// Since there is always terrain beneath us (hopefully),
		// it would always be registered as a collision, since
		// from a 2D top-down perspective, the terrain and the object
		// occupy the same space. But we are kinda 'hacking' 3D,
		// where the object is placed above the terrain. Collision with
		// the terrain is therefore handled more primitively, we simply modify
		// the height in the same step in which we apply the velocity.
		// Because of this, for most purposes, we ignore the terrain.
		constexpr CollisionResponse GetResponseIncludingTerrain(const CollisionRules& other) const;

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
		static constexpr CollisionPreset sProjectiles
		{
			"Projectiles",
			{
				CollisionLayer::Projectiles,
				{
					CollisionResponse::Blocking,	// WorldStatic
					CollisionResponse::Ignore,		// Projectiles
					CollisionResponse::Blocking,	// Character
					CollisionResponse::Ignore,		// Terrain
					CollisionResponse::Overlap,		// Query
				}
			}
		};

		static constexpr CollisionPreset sStaticObstacles
		{
			"Static Obstacles",
			{
				CollisionLayer::StaticObstacles,
				{
					CollisionResponse::Ignore,		// WorldStatic
					CollisionResponse::Blocking,	// Projectiles
					CollisionResponse::Blocking,	// Character
					CollisionResponse::Ignore,		// Terrain
					CollisionResponse::Ignore,		// Query
				}
			}
		};

		static constexpr CollisionPreset sTerrain
		{
			"Terrain",
			{
				CollisionLayer::Terrain,
				{
					CollisionResponse::Ignore,		// WorldStatic
					CollisionResponse::Ignore,		// Projectiles
					CollisionResponse::Ignore,		// Character
					CollisionResponse::Ignore,		// Terrain
					CollisionResponse::Ignore,		// Query
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
					CollisionResponse::Blocking,	// Projectiles
					CollisionResponse::Blocking,	// Character
					CollisionResponse::Ignore,		// Terrain
					CollisionResponse::Overlap,		// Query
				}
			}
		};
	}

	static constexpr std::array<CollisionPreset, 4> sCollisionPresets
	{
		CollisionPresets::sTerrain,
		CollisionPresets::sProjectiles,
		CollisionPresets::sStaticObstacles,
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

		CollisionRules mRules = CollisionPresets::sProjectiles.mRules;

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

		friend PhysicsSystem;

		void ClearForces() { mForce = {}; }
	};

	constexpr CollisionResponse CollisionRules::GetResponse(const CollisionRules& other) const
	{
		if (mLayer == CollisionLayer::Terrain
			|| other.mLayer == CollisionLayer::Terrain)
		{
			return CollisionResponse::Ignore;
		}
		return GetResponseIncludingTerrain(other);
	}

	constexpr CollisionResponse CollisionRules::GetResponseIncludingTerrain(const CollisionRules& other) const
	{
		return static_cast<CollisionResponse>(std::min(static_cast<std::underlying_type_t<CollisionResponse>>(mResponses[static_cast<std::underlying_type_t<CollisionLayer>>(other.mLayer)]),
			static_cast<std::underlying_type_t<CollisionResponse>>(other.mResponses[static_cast<std::underlying_type_t<CollisionLayer>>(mLayer)])));
	}
}

namespace cereal
{
	class BinaryOutputArchive;
	class BinaryInputArchive;

	void save(BinaryOutputArchive& ar, const CE::CollisionRules& value);
	void load(BinaryInputArchive& ar, CE::CollisionRules& value);
}

#ifdef EDITOR
IMGUI_AUTO_DEFINE_INLINE(template<>, CE::CollisionRules, var.DisplayWidget(name);)
#endif // EDITOR

template<>
struct Reflector<CE::CollisionLayer>
{
	static CE::MetaType Reflect();
	static constexpr bool sIsSpecialized = true;
}; REFLECT_AT_START_UP(collLayer, CE::CollisionLayer);

template<>
struct Reflector<CE::CollisionRules>
{
	static CE::MetaType Reflect();
	static constexpr bool sIsSpecialized = true;
}; REFLECT_AT_START_UP(collRules, CE::CollisionRules)

template<>
struct CE::EnumStringPairsImpl<CE::CollisionLayer>
{
	static constexpr EnumStringPairs<CollisionLayer, static_cast<size_t>(CollisionLayer::NUM_OF_LAYERS)> value = 
	{
		EnumStringPair<CollisionLayer>{ CollisionLayer::Terrain, "Terrain" },
		{ CollisionLayer::StaticObstacles, "StaticObstacles" },
		{ CollisionLayer::Projectiles, "Projectiles" },
		{ CollisionLayer::Character, "Character" },
		{ CollisionLayer::Query, "Query" },
	};
};

template<>
struct CE::EnumStringPairsImpl<CE::CollisionResponse>
{
	static constexpr EnumStringPairs<CollisionResponse, static_cast<size_t>(CollisionResponse::NUM_OF_RESPONSES)> value =
	{
		EnumStringPair<CollisionResponse>{ CollisionResponse::Ignore, "Ignore" },
		{ CollisionResponse::Overlap, "Overlap" },
		{ CollisionResponse::Blocking, "Blocking" },
	};
};