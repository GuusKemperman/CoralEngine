#pragma once
#include "magic_enum/magic_enum.hpp"

#include "Meta/Fwd/MetaTypeFwd.h"
#include "Utilities/Imgui/ImguiInspect.h"

namespace CE
{
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
		Blocking
	};

	enum class CollisionLayer : uint8
	{
		/**
		 * \brief All obstacles that will never move. Used for pathfinding
		 */
		WorldStatic,

		/**
		 * \brief All objects fired by the player or enemies.
		 */
		WorldDynamic,

		/**
		 * \brief The players, enemies and other NPCs
		 */
		Character,

		/**
		 * \brief For, well, projectiles!
		 */
		Projectiles,

		/**
		 * \brief For queries, for operations such as finding all objects within a radius.
		 */
		Query,

		EngineLayer0 = WorldStatic,
		EngineLayer1 = WorldDynamic,
		EngineLayer2 = Character,
		EngineLayer3 = Projectiles,
		EngineLayer4 = Query,
		EngineLayer5, // Future proofing
		EngineLayer6, // Future proofing
		EngineLayer7, // Future proofing
		GameLayer0,
		GameLayer1,
		GameLayer2,
		GameLayer3,
		GameLayer4,
		GameLayer5,
		GameLayer6,
		GameLayer7
	};

	static constexpr bool IsCollisionLayerStatic(CollisionLayer layer)
	{
		return layer == CollisionLayer::WorldStatic;
	}

	struct CollisionRules
	{
#ifdef EDITOR
		void DisplayWidget(const std::string& name);
#endif
		constexpr bool operator!=(const CollisionRules& other) const { return mLayer != other.mLayer || mResponses != other.mResponses; }
		constexpr bool operator==(const CollisionRules& other) const { return mLayer == other.mLayer && mResponses == other.mResponses; }

		constexpr CollisionResponse GetResponse(const CollisionRules& other) const;

		constexpr CollisionResponse GetResponse(CollisionLayer layer) const;

		constexpr void SetResponse(CollisionLayer layer, CollisionResponse response);

		CollisionLayer mLayer{};
		std::array<CollisionResponse, magic_enum::enum_count<CollisionLayer>()> mResponses{};
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
					CollisionResponse::Overlap,		// WorldStatic
					CollisionResponse::Ignore,		// WorldDynamic
					CollisionResponse::Overlap,		// Character
					CollisionResponse::Ignore,		// Terrain
					CollisionResponse::Ignore,		// Query
				}
			}
		};

		static constexpr CollisionPreset sStaticObstacles
		{
			"Static Obstacles",
			{
				CollisionLayer::WorldStatic,
				{
					CollisionResponse::Ignore,		// WorldStatic
					CollisionResponse::Blocking,	// WorldDynamic
					CollisionResponse::Blocking,	// Character
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
					CollisionResponse::Blocking,	// WorldDynamic
					CollisionResponse::Blocking,	// Character
					CollisionResponse::Ignore,		// Terrain
					CollisionResponse::Overlap,		// Query
				}
			}
		};
	}

	static constexpr std::array sCollisionPresets
	{
		CollisionPresets::sWorldDynamic,
		CollisionPresets::sStaticObstacles,
		CollisionPresets::sCharacter,
	};
}

constexpr CE::CollisionResponse CE::CollisionRules::GetResponse(const CollisionRules& other) const
{
	return static_cast<CollisionResponse>(std::min(static_cast<std::underlying_type_t<CollisionResponse>>(GetResponse(other.mLayer)),
		static_cast<std::underlying_type_t<CollisionResponse>>(other.GetResponse(mLayer))));
}

constexpr CE::CollisionResponse CE::CollisionRules::GetResponse(CollisionLayer layer) const
{
	return mResponses[static_cast<std::underlying_type_t<CollisionLayer>>(layer)];
}

constexpr void CE::CollisionRules::SetResponse(CollisionLayer layer, CollisionResponse response)
{
	mResponses[static_cast<std::underlying_type_t<CollisionLayer>>(layer)] = response;
}

namespace cereal
{
	class BinaryOutputArchive;
	class BinaryInputArchive;

	void save(BinaryOutputArchive& ar, const CE::CollisionRules& value, uint32 version);
	void load(BinaryInputArchive& ar, CE::CollisionRules& value, uint32 version);
}

CEREAL_CLASS_VERSION(CE::CollisionRules, 0);

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
}; REFLECT_AT_START_UP(collRules, CE::CollisionRules);
