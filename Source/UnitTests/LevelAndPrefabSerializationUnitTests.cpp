#include "Precomp.h"

#include "Assets/Level.h"
#include "Core/UnitTests.h"
#include "Core/AssetManager.h"
#include "Assets/Core/AssetLoadInfo.h"
#include "Assets/Core/AssetSaveInfo.h"
#include "Components/CameraComponent.h"
#include "Components/IsDestroyedTag.h"
#include "Containers/view_istream.h"
#include "World/World.h"
#include "World/Registry.h"
#include "Components/NameComponent.h"
#include "Components/PrefabOriginComponent.h"
#include "Components/TopDownCamControllerComponent.h"
#include "Components/TransformComponent.h"
#include "World/Archiver.h"

using namespace Engine;

static constexpr std::string_view sTestLevelName = "__TestLevel__";
static constexpr std::string_view sTestPrefabName = "__TestPrefab__";

namespace
{
	World ReloadUsingLevel(World&& world);

	World ReloadUsingPrefab(World&& world, entt::entity entity);

	struct PrefabChange
	{
		std::function<UnitTest::Result(World&, entt::entity)> mMakeChanges{};
		std::function<UnitTest::Result(const World&, entt::entity)> mCheckChanges{};
	};

	UnitTest::Result TestPrefabChanges(World&& initialWorld,
		entt::entity initialEntity,
		std::vector<PrefabChange> changes);
}

UNIT_TEST(Serialization, NoPrefabsLevelSerialization)
{
	static const std::string parentName = "EntityName\t\n\t!!";
	static constexpr glm::vec3 parentPosition = { 102.0f, 2035.f, -2035.f };

	static const std::string childName = "ChildName\t\n\t!!";
	static constexpr glm::vec3 childPosition = -glm::vec3{ 102.0f, 2035.f, -2035.f };

	World world{ false };
	Registry& reg = world.GetRegistry();

	const entt::entity parent = reg.Create();
	const entt::entity child = reg.Create();

	{
		reg.AddComponent<NameComponent>(parent, parentName);
		reg.AddComponent<TransformComponent>(parent).SetLocalPosition(parentPosition);

		reg.AddComponent<NameComponent>(child, childName);

		TransformComponent& childTransform = reg.AddComponent<TransformComponent>(child);
		childTransform.SetLocalPosition(childPosition);
		childTransform.SetParent(&reg.Get<TransformComponent>(parent));
	}

	World reloadedWorld = ReloadUsingLevel(std::move(world));
	Registry& reloadedReg = reloadedWorld.GetRegistry();

	TEST_ASSERT(reloadedReg.Valid(parent));
	TEST_ASSERT(reloadedReg.Valid(child));

	TEST_ASSERT(reloadedReg.TryGet<NameComponent>(parent) != nullptr);
	TEST_ASSERT(reloadedReg.TryGet<NameComponent>(child) != nullptr);

	TEST_ASSERT(reloadedReg.TryGet<TransformComponent>(parent) != nullptr);
	TEST_ASSERT(reloadedReg.TryGet<TransformComponent>(child) != nullptr);

	TEST_ASSERT(reloadedReg.Get<NameComponent>(parent).mName == parentName);
	TEST_ASSERT(reloadedReg.Get<NameComponent>(child).mName == childName);

	TEST_ASSERT(reloadedReg.Get<TransformComponent>(parent).GetLocalPosition() == parentPosition);
	TEST_ASSERT(reloadedReg.Get<TransformComponent>(child).GetLocalPosition() == childPosition);

	TEST_ASSERT(reloadedReg.Get<TransformComponent>(child).GetParent() == &reloadedReg.Get<TransformComponent>(parent));
	TEST_ASSERT(reloadedReg.Storage<entt::entity>().in_use() == 2);

	return UnitTest::Success;
}

UNIT_TEST(Serialization, PrefabsSerialization)
{
	static const std::string parentName = "EntityName\t\n\t!!";
	static constexpr glm::vec3 parentPosition = { 102.0f, 2035.f, -2035.f };

	static const std::string childName = "ChildName\t\n\t!!";
	static constexpr glm::vec3 childPosition = -glm::vec3{ 102.0f, 2035.f, -2035.f };

	World world{ false };
	Registry& reg = world.GetRegistry();

	const entt::entity parent = reg.Create();
	const entt::entity child = reg.Create();

	{
		reg.AddComponent<NameComponent>(parent, parentName);
		reg.AddComponent<TransformComponent>(parent).SetLocalPosition(parentPosition);

		reg.AddComponent<NameComponent>(child, childName);

		TransformComponent& childTransform = reg.AddComponent<TransformComponent>(child);
		childTransform.SetLocalPosition(childPosition);
		childTransform.SetParent(&reg.Get<TransformComponent>(parent));
	}

	World reloadedWorld = ReloadUsingPrefab(std::move(world), parent);
	Registry& reloadedReg = reloadedWorld.GetRegistry();

	TEST_ASSERT(reloadedReg.Valid(parent));
	TEST_ASSERT(reloadedReg.Valid(child));

	TEST_ASSERT(reloadedReg.TryGet<PrefabOriginComponent>(parent) != nullptr);
	TEST_ASSERT(reloadedReg.TryGet<PrefabOriginComponent>(child) != nullptr);

	TEST_ASSERT(reloadedReg.Get<PrefabOriginComponent>(parent).GetHashedPrefabName() == Name::HashString(sTestPrefabName));
	TEST_ASSERT(reloadedReg.Get<PrefabOriginComponent>(child).GetHashedPrefabName() == Name::HashString(sTestPrefabName));

	TEST_ASSERT(reloadedReg.TryGet<NameComponent>(parent) != nullptr);
	TEST_ASSERT(reloadedReg.TryGet<NameComponent>(child) != nullptr);

	TEST_ASSERT(reloadedReg.TryGet<TransformComponent>(parent) != nullptr);
	TEST_ASSERT(reloadedReg.TryGet<TransformComponent>(child) != nullptr);

	TEST_ASSERT(reloadedReg.Get<NameComponent>(parent).mName == parentName);
	TEST_ASSERT(reloadedReg.Get<NameComponent>(child).mName == childName);

	TEST_ASSERT(reloadedReg.Get<TransformComponent>(parent).GetLocalPosition() == parentPosition);
	TEST_ASSERT(reloadedReg.Get<TransformComponent>(child).GetLocalPosition() == childPosition);

	TEST_ASSERT(reloadedReg.Get<TransformComponent>(child).GetParent() == &reloadedReg.Get<TransformComponent>(parent));
	TEST_ASSERT(reloadedReg.Storage<entt::entity>().in_use() == 2);

	return UnitTest::Success;
}

UNIT_TEST(Serialization, EmptyEntityLevelSerialization)
{
	World world{ false };
	Registry& reg = world.GetRegistry();

	const entt::entity entity = reg.Create();

	World reloadedWorld = ReloadUsingLevel(std::move(world));
	Registry& reloadedReg = reloadedWorld.GetRegistry();

	TEST_ASSERT(reloadedReg.Valid(entity));
	TEST_ASSERT(reloadedReg.Storage<entt::entity>().in_use() == 1);

	return UnitTest::Success;
}

UNIT_TEST(Serialization, EmptyComponentLevelSerialization)
{
	World world{ false };
	Registry& reg = world.GetRegistry();

	const entt::entity entity = reg.Create();
	reg.AddComponent<IsDestroyedTag>(entity);
	TEST_ASSERT(reg.HasComponent<IsDestroyedTag>(entity));

	World reloadedWorld = ReloadUsingLevel(std::move(world));
	Registry& reloadedReg = reloadedWorld.GetRegistry();

	TEST_ASSERT(reloadedReg.Valid(entity));
	TEST_ASSERT(reloadedReg.Storage<entt::entity>().in_use() == 1);
	TEST_ASSERT(reloadedReg.HasComponent<IsDestroyedTag>(entity));

	return UnitTest::Success;
}

UNIT_TEST(Serialization, EmptyEntityPrefabSerialization)
{
	World world{ false };
	Registry& reg = world.GetRegistry();

	const entt::entity entity = reg.Create();

	World reloadedWorld = ReloadUsingPrefab(std::move(world), entity);
	Registry& reloadedReg = reloadedWorld.GetRegistry();

	TEST_ASSERT(reloadedReg.Valid(entity));
	TEST_ASSERT(reloadedReg.Storage<entt::entity>().in_use() == 1);

	return UnitTest::Success;
}

UNIT_TEST(Serialization, PrefabAddComponent)
{
	World world{ false };

	Registry& reg = world.GetRegistry();
	entt::entity entity = reg.Create();

	return TestPrefabChanges(std::move(world), entity,
		{
			{
				[](World& world, entt::entity entity)
				{
					world.GetRegistry().AddComponent<NameComponent>(entity, "Name!");
					return UnitTest::Result::Success;
				},
				[](const World& world, entt::entity entity)
				{
					const Registry& reg = world.GetRegistry();

					TEST_ASSERT(reg.Valid(entity));
					TEST_ASSERT(reg.TryGet<NameComponent>(entity) != nullptr);
					TEST_ASSERT(reg.Get<NameComponent>(entity).mName == "Name!");

					return UnitTest::Result::Success;
				}
			}
		});
}

UNIT_TEST(Serialization, PrefabRemoveComponent)
{
	World world{ false };

	Registry& reg = world.GetRegistry();
	entt::entity entity = reg.Create();
	reg.AddComponent<NameComponent>(entity);

	return TestPrefabChanges(std::move(world), entity,
		{
			{
				[](World& world, entt::entity entity)
				{
					Registry& reg = world.GetRegistry();

					TEST_ASSERT(reg.TryGet<NameComponent>(entity) != nullptr);
					reg.RemoveComponent<NameComponent>(entity);
					return UnitTest::Result::Success;

				},
				[](const World& world, entt::entity entity)
				{
					const Registry& reg = world.GetRegistry();

					TEST_ASSERT(reg.Valid(entity));
					TEST_ASSERT(reg.TryGet<NameComponent>(entity) == nullptr);
					return UnitTest::Result::Success;
				}
			}
		});
}

UNIT_TEST(Serialization, PrefabAddChild)
{
	World world{ false };

	Registry& reg = world.GetRegistry();
	entt::entity parent = reg.Create();
	reg.AddComponent<TransformComponent>(parent);

	const auto addChild = [](World& world, entt::entity parent)
		{
			Registry& reg = world.GetRegistry();
			entt::entity child = reg.Create();
			TransformComponent& childTransform = reg.AddComponent<TransformComponent>(child);

			TEST_ASSERT(reg.TryGet<TransformComponent>(parent) != nullptr);

			TransformComponent& parentTransform = reg.Get<TransformComponent>(parent);
			childTransform.SetParent(&parentTransform);
			return UnitTest::Result::Success;
		};

	return TestPrefabChanges(std::move(world), parent,
		{
			{
				addChild,
				[](const World& world, entt::entity parent)
				{
					const Registry& reg = world.GetRegistry();

					TEST_ASSERT(reg.TryGet<TransformComponent>(parent) != nullptr);

					const TransformComponent& parentTransform = reg.Get<TransformComponent>(parent);
					TEST_ASSERT(parentTransform.GetChildren().size() == 1);
					TEST_ASSERT(reg.Storage<entt::entity>()->in_use() == 2);
					return UnitTest::Result::Success;
				},
			},
			{
				addChild,
				[](const World& world, entt::entity parent)
				{
					const Registry& reg = world.GetRegistry();

					TEST_ASSERT(reg.TryGet<TransformComponent>(parent) != nullptr);

					const TransformComponent& parentTransform = reg.Get<TransformComponent>(parent);
					TEST_ASSERT(parentTransform.GetChildren().size() == 2);
					TEST_ASSERT(reg.Storage<entt::entity>()->in_use() == 3);
					return UnitTest::Result::Success;
				},
			}
		});
}

UNIT_TEST(Serialization, PrefabRemoveChild)
{
	World world{ false };

	Registry& reg = world.GetRegistry();
	entt::entity parent = reg.Create();
	reg.AddComponent<TransformComponent>(parent);

	entt::entity child = reg.Create();
	TransformComponent& childTransform = reg.AddComponent<TransformComponent>(child);

	TEST_ASSERT(reg.TryGet<TransformComponent>(parent) != nullptr);

	TransformComponent& parentTransform = reg.Get<TransformComponent>(parent);
	childTransform.SetParent(&parentTransform);

	return TestPrefabChanges(std::move(world), parent,
		{
			{
				[](World& world, entt::entity parent)
				{
					Registry& reg = world.GetRegistry();

					TEST_ASSERT(reg.TryGet<TransformComponent>(parent) != nullptr);
					TransformComponent& parentTransform = reg.Get<TransformComponent>(parent);

					TEST_ASSERT(parentTransform.GetChildren().size() == 1);
					TransformComponent& childTransform = parentTransform.GetChildren()[0];

					TEST_ASSERT(reg.Valid(childTransform.GetOwner()));
					TEST_ASSERT(reg.Storage<entt::entity>().in_use() == 2);

					reg.Destroy(childTransform.GetOwner());
					reg.RemovedDestroyed();

					TEST_ASSERT(reg.Storage<entt::entity>().in_use() == 1);

					return UnitTest::Result::Success;
				},
				[](const World& world, entt::entity parent)
				{
					const Registry& reg = world.GetRegistry();

					TEST_ASSERT(reg.TryGet<TransformComponent>(parent) != nullptr);

					const TransformComponent& parentTransform = reg.Get<TransformComponent>(parent);
					TEST_ASSERT(parentTransform.GetChildren().empty());
					TEST_ASSERT(reg.Storage<entt::entity>()->in_use() == 1);
					return UnitTest::Result::Success;
				},
			},
		});
}

UNIT_TEST(Serialization, CopyPaste)
{
	static const std::string parentName = "EntityName\t\n\t!!";
	static constexpr glm::vec3 parentPosition = { 102.0f, 2035.f, -2035.f };

	static const std::string childName = "ChildName\t\n\t!!";
	static constexpr glm::vec3 childPosition = -glm::vec3{ 102.0f, 2035.f, -2035.f };

	World world{ false };
	Registry& reg = world.GetRegistry();

	const entt::entity parent = reg.Create();
	const entt::entity child = reg.Create();

	{
		reg.AddComponent<NameComponent>(parent, parentName);
		reg.AddComponent<TransformComponent>(parent).SetLocalPosition(parentPosition);
		reg.AddComponent<TopDownCamControllerComponent>(parent).mTarget = child;

		reg.AddComponent<NameComponent>(child, childName);

		TransformComponent& childTransform = reg.AddComponent<TransformComponent>(child);
		childTransform.SetLocalPosition(childPosition);
		childTransform.SetParent(&reg.Get<TransformComponent>(parent));
	}

	int depthRemaining = 4;
	std::function<UnitTest::Result(std::vector<entt::entity>&)> doesMatch = [&](std::vector<entt::entity>& entitiesToCheck) -> UnitTest::Result
		{
			TEST_ASSERT(reg.Storage<entt::entity>().in_use() == entitiesToCheck.size());

			for (const entt::entity& entity : entitiesToCheck)
			{
				TEST_ASSERT(reg.TryGet<NameComponent>(entity) != nullptr);

				const std::string_view name = reg.Get<NameComponent>(entity).mName;

				TEST_ASSERT(name == parentName || name == childName);

				if (name == parentName)
				{
					const TransformComponent* const parentTransform = reg.TryGet<TransformComponent>(entity);

					TEST_ASSERT(parentTransform != nullptr);
					TEST_ASSERT(parentTransform->GetLocalPosition() == parentPosition);
					TEST_ASSERT(parentTransform->GetParent() == nullptr);

					const TopDownCamControllerComponent* const topDownController = reg.TryGet<TopDownCamControllerComponent>(entity);

					TEST_ASSERT(topDownController != nullptr);

					TEST_ASSERT(parentTransform->GetChildren().size() == 1);
					TEST_ASSERT(parentTransform->GetChildren()[0].get().GetOwner() == topDownController->mTarget);
				}
				else if (name == childName)
				{
					TEST_ASSERT(reg.TryGet<TransformComponent>(entity) != nullptr);
					TEST_ASSERT(reg.Get<TransformComponent>(entity).GetLocalPosition() == childPosition);
					TEST_ASSERT(reg.Get<TransformComponent>(entity).GetParent() != nullptr);
				}
			}

			--depthRemaining;

			if (depthRemaining > 0)
			{
				BinaryGSONObject copy = Archiver::Serialize(world, std::array<entt::entity, 2>{ parent, child }, true);
				const std::vector<entt::entity> copiedEntities = Archiver::Deserialize(world, copy);
				entitiesToCheck.insert(entitiesToCheck.end(), copiedEntities.begin(), copiedEntities.end());
				return doesMatch(entitiesToCheck);
			}
			return UnitTest::Success;
		};

	std::vector<entt::entity> entities{ parent, child };
	return doesMatch(entities);
}

namespace
{
	World ReloadUsingLevel(World&& world)
	{
		Level testLevel{ sTestLevelName };
		testLevel.CreateFromWorld(world);

		AssetLoadInfo savedLevel = testLevel.Save();

		const Level reloadedLevel{ savedLevel };
		return reloadedLevel.CreateWorld(false);
	}

	World ReloadUsingPrefab(World&& world, entt::entity entity)
	{
		Prefab testPrefab{ sTestPrefabName };
		testPrefab.CreateFromEntity(world, entity);

		AssetLoadInfo savedPrefab = testPrefab.Save();

		Prefab reloadedPrefab{ savedPrefab };

		World reloadedWorld{ false };
		Registry& reloadedReg = reloadedWorld.GetRegistry();
		reloadedReg.CreateFromPrefab(reloadedPrefab, entity);
		return reloadedWorld;
	}

	UnitTest::Result TestPrefabChanges(World&& initialWorld,
		entt::entity initialEntity,
		std::vector<PrefabChange> changes)
	{
		// Prevent the prefab from sticking around after the test is done.
		struct PrefabDeleter
		{
			~PrefabDeleter()
			{
				auto prefab = AssetManager::Get().TryGetWeakAsset<Asset>(sTestPrefabName);

				if (prefab.has_value())
				{
					AssetManager::Get().DeleteAsset(std::move(*prefab));
				}
			}
		};
		PrefabDeleter __{};

		// Make a prefab from it
		std::shared_ptr<const Prefab> prefabInAssetManager{};

		auto updatePrefab = [&]
			{
				if (prefabInAssetManager == nullptr)
				{
					Prefab testPrefab{ sTestPrefabName };
					testPrefab.CreateFromEntity(initialWorld, initialEntity);
					prefabInAssetManager = AssetManager::Get().AddAsset(std::move(testPrefab));
				}
				else
				{
					AssetLoadInfo loadInfo = prefabInAssetManager->Save();

					prefabInAssetManager.reset();
					{
						auto prefabToDelete = AssetManager::Get().TryGetWeakAsset<Asset>(sTestPrefabName);

						if (prefabToDelete.has_value())
						{
							AssetManager::Get().DeleteAsset(std::move(*prefabToDelete));
						}
					}

					Prefab newPrefab{ loadInfo };
					newPrefab.CreateFromEntity(initialWorld, initialEntity);
					prefabInAssetManager = AssetManager::Get().AddAsset(std::move(newPrefab));
				}
			};

		updatePrefab();
		TEST_ASSERT(prefabInAssetManager != nullptr);

		AssetSaveInfo serializedLevel = [&]
			{
				// Spawn the prefab in the world
				World levelWorld{ false };
				levelWorld.GetRegistry().CreateFromPrefab(*prefabInAssetManager, initialEntity);

				// Make a level from it
				Level testLevel{ sTestLevelName };
				testLevel.CreateFromWorld(levelWorld);

				// Save the level
				return testLevel.Save();
			}();

			for (const PrefabChange& change : changes)
			{
				UnitTest::Result result = change.mMakeChanges(initialWorld, initialEntity);

				if (result != UnitTest::Result::Success)
				{
					return result;
				}

				updatePrefab();
				AssetLoadInfo loadInfo{ serializedLevel };
				Level level{ loadInfo };
				World worldWithChangesApplied = level.CreateWorld(false);
				result = change.mCheckChanges(worldWithChangesApplied, initialEntity);

				if (result != UnitTest::Result::Success)
				{
					return result;
				}
			}

			return UnitTest::Result::Success;
	}
}