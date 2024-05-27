#include "Precomp.h"
#include "Utilities/Imgui/WorldDetailsPanel.h"

#include "Components/ComponentFilter.h"
#include "Components/NameComponent.h"
#include "Utilities/Imgui/ImguiHelpers.h"
#include "World/Registry.h"

void CE::WorldDetails::Display(World& world, std::vector<entt::entity>& selectedEntities)
{
	if (selectedEntities.empty())
	{
		ImGui::TextUnformatted("No entities selected");
		return;
	}

	Registry& reg = world.GetRegistry();

	std::vector<std::reference_wrapper<const MetaType>> componentsThatAllSelectedHave{};

	for (auto&& [typeHash, storage] : reg.Storage())
	{
		const MetaType* componentType = MetaManager::Get().TryGetType(typeHash);

		if (componentType == nullptr
			|| componentType->GetProperties().Has(Props::sNoInspectTag))
		{
			continue;
		}

		// Only display components if every selected entity has that component.
		bool allEntitiesHaveOne = true;
		for (entt::entity entity : selectedEntities)
		{
			if (!storage.contains(entity))
			{
				allEntitiesHaveOne = false;
				break;
			}
		}

		if (!allEntitiesHaveOne)
		{
			continue;
		}


		componentsThatAllSelectedHave.emplace_back(*componentType);
	}

	// Most commonly used components are placed at the top
	std::sort(componentsThatAllSelectedHave.begin(), componentsThatAllSelectedHave.end(),
		[&reg](const MetaType& lhs, const MetaType& rhs)
		{
			const size_t lhsCount = reg.Storage(lhs.GetTypeId())->size();
			const size_t rhsCount = reg.Storage(rhs.GetTypeId())->size();

			if (lhsCount != rhsCount)
			{
				return lhsCount > rhsCount;
			}

			return lhs.GetName() > rhs.GetName();
		});

	ImGui::TextUnformatted(NameComponent::GetDisplayName(reg, selectedEntities[0]).data());

	if (selectedEntities.size() > 1)
	{
		ImGui::SameLine();
		ImGui::Text(" and %u others", static_cast<uint32>(selectedEntities.size()) - 1);
	}

	const bool addComponentPopUpJustOpened = ImGui::Button(ICON_FA_PLUS);
	ImGui::SetItemTooltip("Add a new component");

	if (addComponentPopUpJustOpened)
	{
		ImGui::OpenPopup("##AddComponentPopUp");
	}

	ImGui::SameLine();

	Search::Begin();

	for (const MetaType& componentClass : componentsThatAllSelectedHave)
	{
		Search::BeginCategory(componentClass.GetName(),
			[&world, &reg, &selectedEntities, &componentClass](std::string_view name)
			{
				bool removeButtonPressed{};
				const bool isHeaderOpen = ImGui::CollapsingHeaderWithButton(name.data(), "X", &removeButtonPressed);

				if (removeButtonPressed)
				{
					for (const auto entity : selectedEntities)
					{
						reg.RemoveComponentIfEntityHasIt(componentClass.GetTypeId(), entity);
					}

					return false;
				}

				if (isHeaderOpen)
				{
					ImGui::PushID(name.data(), name.data() + name.size());
					const MetaFunc* const onInspect = TryGetEvent(componentClass, sInspectEvent);

					if (onInspect != nullptr)
					{
						// We run the custom OnInspect here, directly after opening the collapsing header.
						// We unfortunately cannot search through it's contents, so we always show it.
						onInspect->InvokeCheckedUnpacked(world, selectedEntities);
					}
				}

				return isHeaderOpen;
			});

		for (const MetaFunc& func : componentClass.EachFunc())
		{
			if (!func.GetProperties().Has(Props::sCallFromEditorTag))
			{
				continue;
			}

			const bool isMemberFunc = func.GetParameters().size() == 1 && func.GetParameters()[0].mTypeTraits.mStrippedTypeId == componentClass.GetTypeId();

			if (!func.GetParameters().empty()
				&& !isMemberFunc)
			{
				LOG(LogEditor, Warning, "Function {}::{} has {} property, but the function has parameters",
					componentClass.GetName(), func.GetDesignerFriendlyName(), Props::sCallFromEditorTag);
				continue;
			}

			if (Search::AddItem(func.GetDesignerFriendlyName(),
				[&reg, &selectedEntities, &componentClass, &func](std::string_view name)
				{
					// We only do this additional PushId for functions,
					// prevents some weird behaviour
					// occuring if for some ungodly reason
					// a user decided to have a field and function
					// with the same name
					ImGui::PushID(123456789);

					ImGui::PushID(static_cast<int>(componentClass.GetTypeId()));

					const bool wasPressed = ImGui::Button(name.data());

					ImGui::PopID();
					ImGui::PopID();

					return wasPressed;
				}))
			{
				entt::sparse_set* storage = reg.Storage(componentClass.GetTypeId());

				if (storage != nullptr)
				{
					for (const entt::entity entity : selectedEntities)
					{
						if (isMemberFunc)
						{
							MetaAny component{ componentClass, storage->value(entity), false };

							if (component == nullptr)
							{
								LOG(LogEditor, Error, "Error invoking {}::{}: Component was unexpectedly nullptr",
									componentClass.GetName(), func.GetDesignerFriendlyName());
								continue;
							}

							func.InvokeUncheckedUnpacked(component);
						}
						else
						{
							func.InvokeUncheckedUnpacked();
						}
					}
				}
				else
				{
					LOG(LogEditor, Error, "Error invoking {}::{}: Storage was unexpectedly nullptr",
						componentClass.GetName(), func.GetDesignerFriendlyName());
				}
			}
		}


		for (const MetaField& field : componentClass.EachField())
		{
			if (field.GetProperties().Has(Props::sNoInspectTag))
			{
				continue;
			}

			Search::AddItem(field.GetName(),
				[&componentClass, &field, &reg, &selectedEntities](std::string_view fieldName) -> bool
				{
					entt::sparse_set* storage = reg.Storage(componentClass.GetTypeId());

					if (storage == nullptr)
					{
						LOG(LogEditor, Error, "Error inspecting field {}::{}: Storage was unexpectedly nullptr",
							componentClass.GetName(), fieldName);
						return false;
					}

					MetaAny firstComponent{ componentClass, storage->value(selectedEntities[0]), false };

					if (firstComponent == nullptr)
					{
						LOG(LogEditor, Error, "Error inspecting field {}::{}: Component on first entity was unexpectedly nullptr",
							componentClass.GetName(), fieldName);
						return false;
					}

					const MetaType& memberType = field.GetType();

					const TypeTraits constRefMemberType{ memberType.GetTypeId(), TypeForm::ConstRef };
					const FuncId idOfEqualityFunc = MakeFuncId(MakeTypeTraits<bool>(), { constRefMemberType, constRefMemberType });

					const MetaFunc* const equalityOperator = memberType.TryGetFunc(OperatorType::equal, idOfEqualityFunc);

					MetaAny refToValueInFirstComponent = field.MakeRef(firstComponent);

					bool allValuesTheSame = true;

					if (equalityOperator != nullptr)
					{
						for (uint32 i = 1; i < static_cast<uint32>(selectedEntities.size()); i++)
						{
							MetaAny anotherComponent{ componentClass, storage->value(selectedEntities[i]), false };

							if (anotherComponent == nullptr)
							{
								LOG(LogEditor, Error, "Error inspecting field {}::{}: Component was unexpectedly nullptr",
									componentClass.GetName(), fieldName);
								return false;
							}

							MetaAny refToValueInAnotherComponent = field.MakeRef(anotherComponent);

							FuncResult areEqualResult = (*equalityOperator)(refToValueInFirstComponent, refToValueInAnotherComponent);
							ASSERT(!areEqualResult.HasError());
							ASSERT(areEqualResult.HasReturnValue());

							if (!*areEqualResult.GetReturnValue().As<bool>())
							{
								allValuesTheSame = false;
								break;
							}
						}
					}
					else
					{
						LOG(LogEditor, Error, "Missing equality operator for {}::{}. Will assume all the values are the same.",
							field.GetOuterType().GetName(),
							field.GetName());
					}

					if (!allValuesTheSame)
					{
						ImGui::Text("*");
						ImGui::SetItemTooltip("Not all selected entities have the same value.");
						ImGui::SameLine();
					}

					// If values are not the same, just display a zero initialized value.
					FuncResult newValue = allValuesTheSame ? memberType.Construct(refToValueInFirstComponent) : memberType.Construct();

					if (newValue.HasError())
					{
						LOG(LogEditor, Error, "Could not display value for field {}::{} as it could not be default constructed",
							field.GetOuterType().GetName(),
							field.GetName(),
							newValue.Error());
						return false;
					}

					/*
					Makes the variable read-only, it can not be modified through the editor.

					This is implemented by disabling all interaction with the widget. This
					means this may not work for more complex widgets, such as vectors, as
					the user also won't be able to open the collapsing header to view the
					vector.
					*/
					ImGui::BeginDisabled(field.GetProperties().Has(Props::sIsEditorReadOnlyTag));

					const bool wasChanged = ShowInspectUI(std::string{ field.GetName() }, newValue.GetReturnValue());

					ImGui::EndDisabled();

					if (!wasChanged)
					{
						return false;
					}

					for (const entt::entity entity : selectedEntities)
					{
						MetaAny component = reg.Get(componentClass.GetTypeId(), entity);
						MetaAny refToValue = field.MakeRef(component);

						const FuncResult result = memberType.CallFunction(OperatorType::assign, refToValue, newValue.GetReturnValue());

						if (result.HasError())
						{
							LOG(LogEditor, Error, "Updating field value failed, could not copy assign value to {}::{} - {}",
								componentClass.GetName(),
								field.GetName(),
								result.Error());
							return false;
						}
					}

					return true;
				});
		}

		Search::EndCategory([]{ ImGui::PopID(); });
	}

	Search::End();

	if (Search::BeginPopup("##AddComponentPopUp"))
	{
		for (const MetaType& type : MetaManager::Get().EachType())
		{
			if (ComponentFilter::IsTypeValid(type)
				&& !type.GetProperties().Has(Props::sNoInspectTag)
				&& std::find_if(componentsThatAllSelectedHave.begin(), componentsThatAllSelectedHave.end(),
					[&type](const MetaType& other)
					{
						return type == other;
					}) == componentsThatAllSelectedHave.end()
						&& Search::Button(type.GetName()))
			{
				for (const entt::entity entity : selectedEntities)
				{
					if (!reg.HasComponent(type.GetTypeId(), entity))
					{
						reg.AddComponent(type, entity);
					}
				}
			}
		}

		Search::EndPopup();
	}
}