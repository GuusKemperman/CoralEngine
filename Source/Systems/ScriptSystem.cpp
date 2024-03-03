#include "Precomp.h"
#include "Systems/ScriptSystem.h"

#include "World/World.h"
#include "World/Registry.h"
#include "Scripting/ScriptTools.h"
#include "Core/VirtualMachine.h"
#include "Meta/MetaManager.h"

void Engine::ScriptSystem::Update(World& world, float dt)
{
	Registry& reg = world.GetRegistry();

	MetaManager& metaMngr = MetaManager::Get();

	// We can't directly iterate over the storages,
	// a script may spawn a component and construct a
	// new storage, which could invalidate the iterators
	// to the storage itself.

	std::vector<std::reference_wrapper<const MetaType>> typesToIterateOver{};

	for (auto&& [typeHash, storage] : reg.Storage())
	{
		const MetaType* const type = metaMngr.TryGetType(typeHash);

		if (type != nullptr
			&& WasTypeCreatedByScript(*type))
		{
			typesToIterateOver.emplace_back(*type);
		}
	}

	// It's veryy slightly faster to convert the arguments to a meta any once.
	MetaAny dtArg{ dt };

	for (const MetaType& type : typesToIterateOver)
	{
		const MetaFunc* const func = type.TryGetFunc("OnTick"_Name);

		if (func == nullptr)
		{
			continue;
		}

		static constexpr TypeTraits floatTypeTraits = MakeTypeTraits<float>();
		FuncId funcId = MakeFuncId(MakeTypeTraits<void>(), { TypeTraits{ type.GetTypeId(), TypeForm::Ref }, floatTypeTraits });
		if (func->GetFuncId() != funcId)
		{
			LOG(LogScripting, Warning, "{} has a tick function, but it is either static, or does not take a float as the first parameter", type.GetName());
			continue;
		}

		entt::sparse_set* storage = reg.Storage(type.GetTypeId());

		if (storage == nullptr)
		{
			continue;
		}

		for (const entt::entity entity : *storage)
		{
			// I *think* this test is needed, in case of tombstones?
			if (!storage->contains(entity))
			{
				continue;
			}

			void* ptr = storage->value(entity);
			MetaAny refToComponent{ type, ptr, false };
		
			FuncResult result = (*func)(refToComponent, dtArg);

			if (result.HasError())
			{
				LOG(LogScripting, Error, "An error occured while executing component {} of entity {} - {}",
					type.GetName(),
					static_cast<EntityType>(entity),
					result.Error())
			}
		}
	}
}

Engine::MetaType Engine::ScriptSystem::Reflect()
{
	return MetaType{ MetaType::T<ScriptSystem>{}, "ScriptSystem", MetaType::Base<System>{} };
}
