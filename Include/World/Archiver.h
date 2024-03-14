#pragma once

namespace Engine
{
	class MetaType;
	class Registry;
	class BinaryGSONObject;
	class World;

	// Can be used to serialize/deserialize worlds
	class Archiver
	{
	public:
		/*
		 * Deserialize the serialized entities and place them in
		 * the world.
		 *
		 * Prefabs are assumed to be in the same state as they were
		 * in during the time of serialization. If the prefabs may
		 * change in the time between serializing and deserializing,
		 * use Engine::Level instead.
		 *
		 * Returns the created entities.
		*/
		static std::vector<entt::entity> Deserialize(World& world, const BinaryGSONObject& serializedWorld);

		// Will serialize the entire world.
		static BinaryGSONObject Serialize(const World& world);

		// Will serialize the provided entities, optionally their children as well.
		static BinaryGSONObject Serialize(const World& world, Span<const entt::entity> entities, bool serializeChildren);

	private:
		static BinaryGSONObject SerializeInternal(const World& world, std::vector<entt::entity>&& entitiesToSerialize, bool allEntitiesInWorldAreBeingSerialized);
	};
}
