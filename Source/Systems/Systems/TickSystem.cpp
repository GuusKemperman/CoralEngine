#include "Precomp.h"
#include "Systems/Events/TickSystem.h"

#include "Utilities/Events.h"
#include "Meta/MetaType.h"
#include "World/Registry.h"
#include "World/World.h"

void Engine::TickSystem::Update(World& world, float dt)
{
	Registry& reg = world.GetRegistry();

	// We can't directly iterate over the storages,
	// a script may spawn a component and construct a
	// new storage, which could invalidate the iterators
	// to the storage itself.
	struct TypeToCallEventFor
	{
		std::reference_wrapper<const MetaType> mType;
		std::reference_wrapper<const MetaFunc> mEvent;
		std::reference_wrapper<entt::sparse_set> mStorage;
	};

	std::vector<TypeToCallEventFor> typesWithEvent{};

	for (auto&& [typeHash, storage] : reg.Storage())
	{
		const MetaType* const type = MetaManager::Get().TryGetType(typeHash);

		if (type == nullptr)
		{
			continue;
		}

		const MetaFunc* const event = TryGetEvent(*type, sTickEvent);

		if (event != nullptr)
		{
			typesWithEvent.emplace_back(TypeToCallEventFor{ *type, *event, storage });
		}
	}

	// It's slightly faster to convert the arguments to a MetaAny once instead of for every component.
	MetaAny worldArg{ world };
	MetaAny dtArg{ dt };

	for (const auto& [type, event, storage] : typesWithEvent)
	{
		for (const entt::entity entity : storage.get())
		{
			// Tombstone check
			if (!storage.get().contains(entity))
			{
				continue;
			}

			void* ptr = storage.get().value(entity);
			ASSERT(ptr != nullptr);

			MetaAny refToComponent{ type.get(), ptr, false};
			FuncResult result = event.get().InvokeUncheckedUnpacked(refToComponent, worldArg, entity, dtArg);

			if (result.HasError())
			{
				LOG(LogWorld, Error, "An error occured while executing component {} of entity {} - {}",
					type.get().GetName(),
					static_cast<EntityType>(entity),
					result.Error())
			}
		}
	}

}

Engine::MetaType Engine::TickSystem::Reflect()
{
	return MetaType{ MetaType::T<TickSystem>{}, "TickSystem", MetaType::Base<System>{} };
}