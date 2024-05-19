#include "Precomp.h"
#include "Components/Pathfinding/NavMeshAgentComponent.h"

#include <cereal/types/variant.hpp>

#include "Components/TransformComponent.h"
#include "Components/Physics2D/PhysicsBody2DComponent.h"
#include "Meta/MetaType.h"
#include "Meta/MetaProps.h"
#include "Utilities/Reflect/ReflectComponentType.h"
#include "Utilities/Reflect/ReflectFieldType.h"

void CE::NavMeshAgentComponent::OnConstruct(World&, entt::entity owner)
{
	mOwner = owner;
}

std::optional<glm::vec2> CE::NavMeshAgentComponent::GetTargetPosition(const World& world) const
{
	if (std::holds_alternative<glm::vec2>(mTarget))
	{
		return std::get<glm::vec2>(mTarget);
	}

	const entt::entity target = GetTargetEntity();

	if (target == entt::null)
	{
		return std::nullopt;
	}

	const TransformComponent* transform = world.GetRegistry().TryGet<TransformComponent>(target);

	if (transform != nullptr)
	{
		return transform->GetWorldPosition2D();
	}
	return std::nullopt;
}

entt::entity CE::NavMeshAgentComponent::GetTargetEntity() const
{
	if (std::holds_alternative<entt::entity>(mTarget))
	{
		return std::get<entt::entity>(mTarget);
	}
	return entt::null;
}

void CE::NavMeshAgentComponent::SetTargetPosition(glm::vec2 targetPosition)
{
	mTarget = targetPosition;
}

void CE::NavMeshAgentComponent::SetTargetEntity(entt::entity entity)
{
	mTarget = entity;
}

void CE::NavMeshAgentComponent::ClearTarget(World& world)
{
	mTarget = std::monostate{};
	mPath.clear();

	PhysicsBody2DComponent* const body = world.GetRegistry().TryGet<PhysicsBody2DComponent>(mOwner);

	if (body != nullptr)
	{
		body->mLinearVelocity = {};
	}
}

bool CE::NavMeshAgentComponent::IsChasing() const
{
	return !std::holds_alternative<std::monostate>(mTarget);
}

void load(cereal::BinaryInputArchive& ar, CE::NavMeshAgentComponent::TargetT& target)
{
	ar(target);
}

void save(cereal::BinaryOutputArchive& ar, const CE::NavMeshAgentComponent::TargetT& target)
{
	ar(target);
}

namespace ImGui
{
	template<>
	struct Auto_t<CE::NavMeshAgentComponent::TargetT>
	{
		static void Auto(CE::NavMeshAgentComponent::TargetT& var, const std::string& name)
		{
			if (!ImGui::TreeNode(name.c_str()))
			{
				return;
			}

			const auto typeName = [](size_t index)
				{
					switch (index)
					{
					default:
					case 0: return "None";
					case 1: return "Position";
					case 2: return "Entity";
					}
				};

			if (ImGui::BeginCombo("Target type", typeName(var.index())))
			{
				if (ImGui::MenuItem(typeName(0)))
				{
					var = std::monostate{};
				}

				if (ImGui::MenuItem(typeName(1)))
				{
					var = glm::vec2{};
				}

				if (ImGui::MenuItem(typeName(2)))
				{
					var = entt::entity{ entt::null };
				}

				ImGui::EndCombo();
			}

			switch (var.index())
			{
			default:
			case 0:
				break;
			case 1:
			{
				ImGui::Auto(std::get<glm::vec2>(var), "Position");
				break;
			}
			case 2:
			{
				ImGui::Auto(std::get<entt::entity>(var), "Entity");
				break;
			}
			}

			ImGui::TreePop();
		}
		static constexpr bool sIsSpecialized = true;
	};
}

template<>
struct Reflector<CE::NavMeshAgentComponent::TargetT>
{
	static CE::MetaType Reflect()
	{
		CE::MetaType type{ CE::MetaType::T<CE::NavMeshAgentComponent::TargetT>{}, "NavMeshTarget"};
		CE::ReflectFieldType<CE::NavMeshAgentComponent::TargetT>(type);
		return type;
	}
	static constexpr bool sIsSpecialized = true;
};

CE::MetaType CE::NavMeshAgentComponent::Reflect()
{
	auto metaType = MetaType{MetaType::T<NavMeshAgentComponent>{}, "NavMeshAgentComponent"};
	metaType.GetProperties().Add(Props::sIsScriptableTag);

	metaType.AddFunc(&NavMeshAgentComponent::IsChasing, "IsChasing").GetProperties().Add(Props::sIsScriptableTag);
	metaType.AddFunc(&NavMeshAgentComponent::SetTargetPosition, "SetTargetPosition").GetProperties().Add(Props::sIsScriptableTag);
	metaType.AddFunc(&NavMeshAgentComponent::SetTargetEntity, "SetTargetPosition", "", "Target to chase").GetProperties().Add(Props::sIsScriptableTag);

	metaType.AddFunc([](NavMeshAgentComponent& agent)
		{
			agent.ClearTarget(*World::TryGetWorldAtTopOfStack());
		}, "Clear Target", MetaFunc::ExplicitParams<NavMeshAgentComponent&>{}).GetProperties().Add(Props::sIsScriptableTag);

	metaType.AddFunc(&NavMeshAgentComponent::GetTargetEntity, "GetTargetEntity").GetProperties().Add(Props::sIsScriptableTag);
	metaType.AddFunc([](const NavMeshAgentComponent& agent)
		{
			return agent.GetTargetPosition(*World::TryGetWorldAtTopOfStack()).value_or(glm::vec2{});
		}, "GetTargetPosition", MetaFunc::ExplicitParams<NavMeshAgentComponent&>{}).GetProperties().Add(Props::sIsScriptableTag);

	metaType.AddField(&NavMeshAgentComponent::mTarget, "mTarget");

	BindEvent(metaType, sConstructEvent, &NavMeshAgentComponent::OnConstruct);
	ReflectComponentType<NavMeshAgentComponent>(metaType);

	return metaType;
}
