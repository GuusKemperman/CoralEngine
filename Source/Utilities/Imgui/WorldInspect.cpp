#include "Precomp.h"
#include "Utilities/Imgui/WorldInspect.h"

#include "Assets/Level.h"
#include "imgui/ImGuizmo.h"
#include "imgui/imgui_internal.h"

#include "World/World.h"
#include "World/WorldRenderer.h"
#include "World/Registry.h"
#include "Utilities/FrameBuffer.h"
#include "Core/Input.h"
#include "Components/TransformComponent.h"
#include "Components/CameraComponent.h"
#include "Components/NameComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Assets/Prefabs/Prefab.h"
#include "Assets/StaticMesh.h"
#include "Assets/Material.h"
#include "Utilities/Imgui/ImguiDragDrop.h"
#include "Utilities/Imgui/ImguiInspect.h"
#include "Utilities/Imgui/ImguiHelpers.h"
#include "Utilities/Search.h"
#include "Meta/MetaManager.h"
#include "Meta/MetaProps.h"
#include "Core/AssetManager.h"
#include "Utilities/StringFunctions.h"
#include "Utilities/Reflect/ReflectComponentType.h"
#include "World/Archiver.h"

namespace
{
	ImGuizmo::OPERATION sGuizmoOperation{ ImGuizmo::OPERATION::TRANSLATE };
	ImGuizmo::MODE sGuizmoMode{ ImGuizmo::MODE::WORLD };
	bool sShouldGuizmoSnap{};
	glm::vec3 sSnapTo{ 1.0f };

	void RemoveInvalidEntities(Engine::World& world, std::vector<entt::entity>& selectedEntities);

	void DeleteEntities(Engine::World& world, std::vector<entt::entity>& selectedEntities);
	std::string CopyToClipBoard(const Engine::World& world, const std::vector<entt::entity>& selectedEntities);
	void CutToClipBoard(Engine::World& world, std::vector<entt::entity>& selectedEntities);
	void PasteClipBoard(Engine::World& world, std::vector<entt::entity>& selectedEntities, std::string_view clipboardData);
	void Duplicate(Engine::World& world, std::vector<entt::entity>& selectedEntities);
	std::optional<std::string_view> GetSerializedEntitiesInClipboard();
	bool IsStringFromCopyToClipBoard(std::string_view string);

	void CheckShortcuts(Engine::World& world, std::vector<entt::entity>& selectedEntities);
	void ReceiveDragDrops(Engine::World& world);
}

Engine::WorldInspectHelper::WorldInspectHelper(World&& worldThatHasNotYetBegunPlay) :
	mViewportFrameBuffer(std::make_unique<FrameBuffer>()),
	mWorldBeforeBeginPlay(std::make_unique<World>(std::move(worldThatHasNotYetBegunPlay)))
{
	ASSERT(!mWorldBeforeBeginPlay->HasBegunPlay());
}

Engine::WorldInspectHelper::~WorldInspectHelper() = default;

Engine::World& Engine::WorldInspectHelper::GetWorld()
{
	if (mWorldAfterBeginPlay != nullptr)
	{
		return *mWorldAfterBeginPlay;
	}
	ASSERT(mWorldBeforeBeginPlay != nullptr);
	return *mWorldBeforeBeginPlay;
}

Engine::World& Engine::WorldInspectHelper::BeginPlay()
{
	if (mWorldAfterBeginPlay != nullptr)
	{
		LOG(LogEditor, Error, "Called begin play when the world has already begun play");
		return GetWorld();
	}
	ASSERT(!mWorldBeforeBeginPlay->HasBegunPlay() && "Do not call BeginPlay on the world yourself, use WorldInspectHelper::BeginPlay")

		mWorldAfterBeginPlay = std::make_unique<World>(false);

	// Duplicate our level world
	const BinaryGSONObject serializedWorld = Archiver::Serialize(*mWorldBeforeBeginPlay);
	Archiver::Deserialize(*mWorldAfterBeginPlay, serializedWorld);

	mWorldAfterBeginPlay->BeginPlay();
	return GetWorld();
}

Engine::World& Engine::WorldInspectHelper::EndPlay()
{
	if (mWorldAfterBeginPlay == nullptr)
	{
		LOG(LogEditor, Error, "Called EndPlay, but WorldInspectHelper::BeginPlay was never called");
		return GetWorld();
	}
	ASSERT(mWorldAfterBeginPlay->HasBegunPlay() && "Do not call EndPlay on the world yourself, use WorldInspectHelper::EndPlay");

	mWorldAfterBeginPlay->EndPlay();
	mWorldAfterBeginPlay.reset();

	return GetWorld();
}

void Engine::WorldInspectHelper::DisplayAndTick(const float deltaTime)
{
	mDeltaTimeRunningAverage = mDeltaTimeRunningAverage * sRunningAveragePreservePercentage + deltaTime * (1.0f - sRunningAveragePreservePercentage);

	ImGui::Splitter(true, &mViewportWidth, &mHierarchyAndDetailsWidth);

	if (ImGui::BeginChild("WorldViewport", { mViewportWidth, 0.0f }))
	{
		const ImVec2 beginPlayPos = ImGui::GetWindowContentRegionMin() + ImVec2{ ImGui::GetContentRegionAvail().x / 2.0f, 10.0f };
		const ImVec2 viewportPos = ImGui::GetCursorPos();

		ImDrawList* drawList = ImGui::GetCurrentWindow()->DrawList;

		drawList->ChannelsSplit(2);

		drawList->ChannelsSetCurrent(1);

		if (!mSelectedEntities.empty())
		{

			if (ImGui::RadioButton("Translate", sGuizmoOperation == ImGuizmo::TRANSLATE))
				sGuizmoOperation = ImGuizmo::TRANSLATE;
			ImGui::SameLine();
			if (ImGui::RadioButton("Rotate", sGuizmoOperation == ImGuizmo::ROTATE))
				sGuizmoOperation = ImGuizmo::ROTATE;
			ImGui::SameLine();
			if (ImGui::RadioButton("Scale", sGuizmoOperation == ImGuizmo::SCALE))
				sGuizmoOperation = ImGuizmo::SCALE;


			if (sGuizmoOperation != ImGuizmo::SCALE)
			{
				if (ImGui::RadioButton("Local", sGuizmoMode == ImGuizmo::LOCAL))
					sGuizmoMode = ImGuizmo::LOCAL;
				ImGui::SameLine();
				if (ImGui::RadioButton("World", sGuizmoMode == ImGuizmo::WORLD))
					sGuizmoMode = ImGuizmo::WORLD;
			}

			ImGui::Checkbox("Snap", &sShouldGuizmoSnap);
			ImGui::SameLine();

			if (sShouldGuizmoSnap)
			{
				switch (sGuizmoOperation)
				{
				case ImGuizmo::TRANSLATE:
					ImGui::InputFloat3("Snap", value_ptr(sSnapTo));
					break;
				case ImGuizmo::ROTATE:
					ImGui::InputFloat("Angle Snap", value_ptr(sSnapTo));
					break;
				case ImGuizmo::SCALE:
					ImGui::InputFloat("Scale Snap", value_ptr(sSnapTo));
					break;
				default:
					break;
				}
			}
		}

		ImGui::SetCursorPos(beginPlayPos);

		if (!GetWorld().HasBegunPlay())
		{
			ImGui::SetNextItemAllowOverlap();
			ImGui::SetItemTooltip("Begin play");
			if (ImGui::Button(ICON_FA_PLAY))
			{
				(void)BeginPlay();
			}
		}
		else
		{
			if (GetWorld().IsPaused())
			{
				ImGui::SetNextItemAllowOverlap();
				if (ImGui::Button(ICON_FA_PLAY))
				{
					GetWorld().Unpause();
				}
				ImGui::SetItemTooltip("Resume play");
			}
			else
			{
				ImGui::SetNextItemAllowOverlap();
				if (ImGui::Button(ICON_FA_PAUSE))
				{
					GetWorld().Pause();
				}
				ImGui::SetItemTooltip("Pause");
			}

			ImGui::SameLine();
			ImGui::SetCursorPosY(beginPlayPos.y);

			ImGui::SetNextItemAllowOverlap();
			if (ImGui::Button(ICON_FA_STOP))
			{
				(void)EndPlay();
			}
			ImGui::SetItemTooltip("Stop");
		}

		const glm::vec2 fpsCursorPos = { viewportPos.x + mViewportWidth - 60.0f, 0.0f };
		ImGui::SetCursorPos(fpsCursorPos);

		ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.3f);
		ImGui::TextUnformatted(Format("FPS {:.1f}", 1.0f / mDeltaTimeRunningAverage).data());
		ImGui::PopStyleVar();

		{
			const auto possibleCamerasView = GetWorld().GetRegistry().View<CameraComponent>();

			if (possibleCamerasView.size() > 1)
			{
				const auto cam = GetWorld().GetRenderer().GetMainCamera();

				const entt::entity cameraEntity = cam.has_value() ? cam->first : entt::null;

				ImGui::SetCursorPos({ fpsCursorPos.x - ImGui::CalcTextSize(ICON_FA_CAMERA).x - 10.0f, fpsCursorPos.y });

				if (ImGui::Button(ICON_FA_CAMERA))
				{
					ImGui::OpenPopup("CameraSelectPopUp");
				}

				if (ImGui::BeginPopup("CameraSelectPopUp"))
				{
					for (entt::entity possibleCamera : possibleCamerasView)
					{
						if (ImGui::MenuItem(NameComponent::GetDisplayName(GetWorld().GetRegistry(), possibleCamera).c_str(), nullptr, possibleCamera == cameraEntity))
						{
							GetWorld().GetRenderer().SetMainCamera(possibleCamera);
						}
					}
					ImGui::EndPopup();
				}
			}
		}

		drawList->ChannelsSetCurrent(0);
		ImGui::SetCursorPos(viewportPos);

		GetWorld().Tick(deltaTime);
		WorldViewport::Display(GetWorld(), *mViewportFrameBuffer, &mSelectedEntities);

		if (const std::shared_ptr<const Level> nextLevel = GetWorld().GetNextLevel(); nextLevel != nullptr)
		{
			GetWorld() = nextLevel->CreateWorld(true);
		}

		drawList->ChannelsMerge();
	}
	ImGui::EndChild();

	ImGui::SameLine();

	if (ImGui::BeginChild("HierarchyAndDetailsWindow", { mHierarchyAndDetailsWidth, 0.0f }))
	{
		ImGui::PushID(2); // Second splitter requires new ID
		ImGui::Splitter(false, &mHierarchyHeight, &mDetailsHeight);
		ImGui::PopID();

		ImGui::BeginChild("WorldHierarchy", { 0.0f, mHierarchyHeight });
		WorldHierarchy::Display(GetWorld(), &mSelectedEntities);
		ImGui::EndChild();

		ImGui::BeginChild("WorldDetails", { 0.0f, mDetailsHeight });
		WorldDetails::Display(GetWorld(), mSelectedEntities);
		ImGui::EndChild();
	}

	ImGui::EndChild();

	CheckShortcuts(GetWorld(), mSelectedEntities);
}

void Engine::WorldInspectHelper::SaveState(BinaryGSONObject& state)
{
	state.AddGSONMember("selected") << mSelectedEntities;
	state.AddGSONMember("hierarchyWidth") << mHierarchyHeight;
	state.AddGSONMember("detailsWidth") << mDetailsHeight;
	state.AddGSONMember("viewportWidth") << mViewportWidth;
	state.AddGSONMember("hierarchyAndDetailsWidth") << mHierarchyAndDetailsWidth;

	auto activeCamera = GetWorld().GetRenderer().GetMainCamera();

	if (activeCamera.has_value())
	{
		state.AddGSONMember("activeCamera") << activeCamera->first;
	}
}

void Engine::WorldInspectHelper::LoadState(const BinaryGSONObject& state)
{
	const BinaryGSONMember* const selected = state.TryGetGSONMember("selected");
	const BinaryGSONMember* const hierarchyWidth = state.TryGetGSONMember("hierarchyWidth");
	const BinaryGSONMember* const detailsWidth = state.TryGetGSONMember("detailsWidth");
	const BinaryGSONMember* const viewportWidth = state.TryGetGSONMember("viewportWidth");
	const BinaryGSONMember* const hierarchyAndDetailsWidth = state.TryGetGSONMember("hierarchyAndDetailsWidth");

	if (selected == nullptr
		|| hierarchyWidth == nullptr
		|| detailsWidth == nullptr
		|| viewportWidth == nullptr
		|| hierarchyAndDetailsWidth == nullptr)
	{
		LOG(LogEditor, Verbose, "Could not load state for world inspect helper, missing values");
		return;
	}

	*selected >> mSelectedEntities;
	*hierarchyWidth >> mHierarchyHeight;
	*detailsWidth >> mDetailsHeight;
	*viewportWidth >> mViewportWidth;
	*hierarchyAndDetailsWidth >> mHierarchyAndDetailsWidth;

	const BinaryGSONMember* const activeCamera = state.TryGetGSONMember("activeCamera");

	if (activeCamera != nullptr)
	{
		entt::entity cameraEntity{};
		*activeCamera >> cameraEntity;
		GetWorld().GetRenderer().SetMainCamera(cameraEntity);
	}
}

void Engine::WorldViewport::Display(World& world, FrameBuffer& frameBuffer,
	std::vector<entt::entity>* selectedEntities)
{
	const glm::vec2 windowPos = ImGui::GetWindowPos();
	const glm::vec2 contentMin = ImGui::GetWindowContentRegionMin();
	const glm::vec2 contentSize = ImGui::GetContentRegionAvail();

	if (contentSize.x <= 0.0f
		|| contentSize.y <= 0.0f)
	{
		return;
	}

	std::vector<entt::entity> dummySelectedEntities{};
	if (selectedEntities == nullptr)
	{
		selectedEntities = &dummySelectedEntities;
	}// From here on out, we can assume selectedEntities != nullptr

	RemoveInvalidEntities(world, *selectedEntities);

	const auto cameraPair = world.GetRenderer().GetMainCamera();

	if (!cameraPair.has_value())
	{
		ImGui::TextUnformatted("No camera");
		return;
	}

	ImGuizmo::SetDrawlist(ImGui::GetWindowDrawList());
	SetGizmoRect(windowPos + contentMin, contentSize);

	world.GetRenderer().Render(frameBuffer, contentSize);

	ImGui::SetCursorPos(contentMin);

	ImGui::Image((ImTextureID)frameBuffer.GetColorTextureId(),
		ImVec2(contentSize),
		ImVec2(0, 0),
		ImVec2(1, 1));

	// Since it is our 'image' that receives the drag drop, we call this right after the image call.
	ReceiveDragDrops(world);

	ImGui::SetCursorPos(contentMin);

	// There is no need to try to draw gizmos/manipulate transforms when nothing is selected
	if (!selectedEntities->empty())
	{
		ShowComponentGizmos(world, *selectedEntities);
		GizmoManipulateSelectedTransforms(world, *selectedEntities, cameraPair->second);
	}
}

void Engine::WorldViewport::ShowComponentGizmos(World& world, const std::vector<entt::entity>& selectedEntities)
{
	Registry& reg = world.GetRegistry();

	for (auto&& [typeHash,storage] : reg.Storage())
	{
		const MetaType* const type = MetaManager::Get().TryGetType(typeHash);

		if (type == nullptr)
		{
			continue;
		}

		const MetaFunc* const event = TryGetEvent(*type, sDrawGizmoEvent);

		if (event == nullptr)
		{
			continue;
		}

		const bool isStatic = event->GetProperties().Has(Props::sIsEventStaticTag);

		for (entt::entity entity : selectedEntities)
		{
			if (!storage.contains(entity))
			{
				continue;
			}

			if (isStatic)
			{
				event->InvokeUncheckedUnpacked(world, entity);
			}
			else
			{
				MetaAny component{ *type, storage.value(entity), false };
				event->InvokeUncheckedUnpacked(component, world, entity);
			}
		}
	}
}


void Engine::WorldViewport::SetGizmoRect(const glm::vec2 windowPos, const glm::vec2& windowSize)
{
	ImGuizmo::SetRect(windowPos.x, windowPos.y, windowSize.x, windowSize.y);
}

void Engine::WorldViewport::GizmoManipulateSelectedTransforms(World& world,
	const std::vector<entt::entity>& selectedEntities,
	const CameraComponent& camera)
{
	Registry& reg = world.GetRegistry();

	const glm::mat4& view = camera.GetView();
	const glm::mat4& proj = camera.GetProjection();

	const float* const snap = sShouldGuizmoSnap ? value_ptr(sSnapTo) : nullptr;

	std::vector<TransformComponent*> transformComponents{};
	std::vector<entt::entity> entitiesToTransform{};
	glm::vec3 totalPosition{};
	glm::vec3 totalScale{};

	for (auto entity : selectedEntities)
	{
		auto transformComponent = reg.TryGet<TransformComponent>(entity);

		if (transformComponent != nullptr)
		{
			totalPosition += transformComponent->GetWorldPosition();
			totalScale += transformComponent->GetWorldScale();

			transformComponents.push_back(transformComponent);
			entitiesToTransform.push_back(transformComponent->GetOwner());
		}
	}

	const glm::vec3 avgPosition = totalPosition / static_cast<float>(transformComponents.size());
	const glm::vec3 avgScale = totalScale / static_cast<float>(transformComponents.size());

	glm::mat4 avgMatrix;

	ImGuizmo::RecomposeMatrixFromComponents(value_ptr(avgPosition),
		value_ptr(transformComponents.size() == 1 ? transformComponents[0]->GetWorldOrientationEuler() * (360.0f / TWOPI) : glm::vec3{}),
		value_ptr(avgScale),
		&avgMatrix[0][0]);

	glm::mat4 delta;

	if (Manipulate(value_ptr(view), value_ptr(proj), sGuizmoOperation, sGuizmoMode, value_ptr(avgMatrix), value_ptr(delta), snap))
	{
		// Apply the delta to all transformComponents
		for (const auto transformComponent : transformComponents)
		{
			glm::mat4 transformMatrix;

			ImGuizmo::RecomposeMatrixFromComponents(value_ptr(transformComponent->GetWorldPosition()),
				value_ptr(transformComponent->GetWorldOrientationEuler() * (360.0f / TWOPI)),
				value_ptr(transformComponent->GetWorldScale()),
				&transformMatrix[0][0]);

			// TODO Fix the scaling of multiple items at a time
			if (sGuizmoOperation & ImGuizmo::SCALE)
			{
				transformMatrix = transformMatrix * delta;
			}
			else
			{
				transformMatrix = delta * transformMatrix;
			}

			glm::vec3 translation{};
			glm::vec3 eulerOrientation{};
			glm::vec3 scale{};

			ImGuizmo::DecomposeMatrixToComponents(&transformMatrix[0][0], value_ptr(translation), value_ptr(eulerOrientation),
				value_ptr(scale));

			transformComponent->SetWorldMatrix(transformMatrix);
		}
	}
}

void Engine::WorldDetails::Display(World& world, std::vector<entt::entity>& selectedEntities)
{
	const uint32 numOfSelected = static_cast<uint32>(selectedEntities.size());

	if (numOfSelected == 0)
	{
		ImGui::TextUnformatted("No entities selected");
		return;
	}

	Registry& reg = world.GetRegistry();

	ImGui::TextUnformatted(NameComponent::GetDisplayName(reg, selectedEntities[0]).c_str());

	if (numOfSelected > 1)
	{
		ImGui::SameLine();
		ImGui::Text(" and %u others", numOfSelected - 1);
	}

	const bool addComponentPopUpJustOpened = ImGui::Button(ICON_FA_PLUS);
	ImGui::SetItemTooltip("Add a new component");

	if (addComponentPopUpJustOpened)
	{
		ImGui::OpenPopup("##AddComponentPopUp");
	}

	ImGui::SameLine();

	const std::string searchFor = Search::DisplaySearchBar();

	Search::Choices<MetaType> componentsToDisplay{};
	std::vector<TypeId> classesThatCannotBeAdded{};

	for (auto&& [typeHash, storage] : reg.Storage())
	{
		// Only display components if every selected entity has that component.

		bool allEntitiesHaveOne = true;
		for (uint32 i = 0; i < numOfSelected; i++)
		{
			if (!storage.contains(selectedEntities[i]))
			{
				allEntitiesHaveOne = false;
				break;
			}
		}

		if (!allEntitiesHaveOne)
		{
			continue;
		}

		const MetaType* componentType = MetaManager::Get().TryGetType(typeHash);

		if (componentType == nullptr
			|| componentType->GetProperties().Has(Props::sNoInspectTag))
		{
			continue;
		}

		componentsToDisplay.emplace_back(componentType->GetName(), *componentType);

		// We can't add components more than once.
		classesThatCannotBeAdded.push_back(typeHash);
	}

	Search::EraseChoicesThatDoNotMatch(searchFor, componentsToDisplay);

	for (const Search::Choice<MetaType>& choice : componentsToDisplay)
	{
		const MetaType& componentClass = choice.mValue;
		const TypeId typeHash = componentClass.GetTypeId();

		ImGui::PushID(static_cast<int>(typeHash));

		// Makes sure we popId regardless of where we call continue or break
		struct IdPopper
		{
			~IdPopper()
			{
				ImGui::PopID();
			}
		};
		IdPopper __{};

		const char* className = componentClass.GetName().c_str();
		auto& storage = *reg.Storage(typeHash);

		bool removeButtonPressed{};

		const bool isHeaderOpen = ImGui::CollapsingHeaderWithButton(className, "X", &removeButtonPressed);

		if (removeButtonPressed)
		{
			for (const auto entity : selectedEntities)
			{
				reg.RemoveComponentIfEntityHasIt(componentClass.GetTypeId(), entity);
			}

			continue;
		}

		if (!isHeaderOpen)
		{
			continue;
		}


		for (const MetaFunc& func : componentClass.EachFunc())
		{
			if (!func.GetProperties().Has(Props::sCallFromEditorTag))
			{
				continue;
			}

			const bool isMemberFunc = func.GetParameters().size() == 1 && func.GetParameters()[0].mTypeTraits.mStrippedTypeId == typeHash;

			if (!func.GetParameters().empty()
				&& !isMemberFunc)
			{
				LOG(LogEditor, Warning, "Function {}::{} has {} property, but the function has parameters",
					componentClass.GetName(), func.GetDesignerFriendlyName(), Props::sCallFromEditorTag);
				continue;
			}

			if (ImGui::Button(func.GetDesignerFriendlyName().data()))
			{
				for (const entt::entity entity : selectedEntities)
				{
					FuncResult result{};

					if (isMemberFunc)
					{
						MetaAny component{ componentClass, storage.value(entity), false };
						result = func(component);
					}
					else
					{
						result = func();
					}

					if (result.HasError())
					{
						LOG(LogEditor, Error, "Error invoking {}::{} on entity {} - {}",
							componentClass.GetName(), func.GetDesignerFriendlyName(),
							static_cast<EntityType>(entity),
							result.Error());
					}
				}
			}
		}

		const MetaFunc* const onInspect = TryGetEvent(componentClass, sInspectEvent);

		if (onInspect != nullptr)
		{
			FuncResult result = (*onInspect)(world, selectedEntities);

			if (result.HasError())
			{
				LOG(LogEditor, Error, "An error occured while inspecting component that had a custom OnInspect: {}", result.Error());
			}
		}

		MetaAny firstComponent{ componentClass, storage.value(selectedEntities[0]), false };

		for (const MetaField& field : componentClass.EachField())
		{
			if (field.GetProperties().Has(Props::sNoInspectTag))
			{
				continue;
			}

			const MetaType& memberType = field.GetType();

			const TypeTraits constRefMemberType{ memberType.GetTypeId(), TypeForm::ConstRef };
			const FuncId idOfEqualityFunc = MakeFuncId(MakeTypeTraits<bool>(), { constRefMemberType, constRefMemberType });

			const MetaFunc* const equalityOperator = memberType.TryGetFunc(OperatorType::equal, idOfEqualityFunc);

			MetaAny refToValueInFirstComponent = field.MakeRef(firstComponent);

			bool allValuesTheSame = true;

			if (equalityOperator != nullptr)
			{
				for (uint32 i = 1; i < numOfSelected; i++)
				{
					MetaAny anotherComponent{ componentClass, storage.value(selectedEntities[i]), false };
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
				LOG(LogEditor, Error, "Could not display value for {}::{} as it could not be constructed",
					field.GetOuterType().GetName(),
					field.GetName(),
					newValue.Error());
				continue;
			}

			if (!ShowInspectUI(std::string{ field.GetName() }, newValue.GetReturnValue()))
			{
				continue;
			}

			for (const entt::entity entity : selectedEntities)
			{
				MetaAny component = reg.Get(componentClass.GetTypeId(), entity);
				MetaAny refToValue = field.MakeRef(component);

				const FuncResult result = memberType.CallFunction(OperatorType::assign, refToValue, newValue.GetReturnValue());

				if (result.HasError())
				{
					LOG(LogEditor, Warning, "Could not copy assign value to {}::{} - {}",
						componentClass.GetName(),
						field.GetName(),
						result.Error());
					break;
				}
			}
		}
	}

	if (ImGui::BeginPopup("##AddComponentPopUp"))
	{
		if (addComponentPopUpJustOpened)
		{
			ImGui::SetKeyboardFocusHere();
		}

		Search::Choices<MetaType> choices = Search::CollectChoices<MetaType>([&classesThatCannotBeAdded](const MetaType& type)
			{
				return type.GetProperties().Has(Props::sComponentTag)
					&& !type.GetProperties().Has(Props::sNoInspectTag)
					&& std::find_if(classesThatCannotBeAdded.begin(), classesThatCannotBeAdded.end(),
						[&type](const TypeId other)
						{
							return type.IsExactly(other);
						}) == classesThatCannotBeAdded.end();
			});

		std::optional<std::reference_wrapper<const MetaType>> componentToAdd = Search::DisplaySearchBar<MetaType>(choices);

		if (componentToAdd.has_value())
		{
			for (entt::entity entity : selectedEntities)
			{
				if (reg.TryGet(componentToAdd->get().GetTypeId(), entity) == nullptr)
				{
					reg.AddComponent(*componentToAdd, entity);
				}
			}
			ImGui::CloseCurrentPopup();
		}

		ImGui::EndPopup();
	}
}

void Engine::WorldHierarchy::Display(World& world, std::vector<entt::entity>* selectedEntities)
{
	std::vector<entt::entity> dummySelectedEntities{};
	if (selectedEntities == nullptr)
	{
		selectedEntities = &dummySelectedEntities;
	}// From here on out, we can assume selectedEntities != nullptr

	if (ImGui::IsMouseClicked(1)
		&& ImGui::IsWindowHovered())
	{
		ImGui::OpenPopup("HierarchyPopUp");
	}

	Registry& reg = world.GetRegistry();

	if (ImGui::Button(ICON_FA_PLUS))
	{
		const entt::entity newEntity = reg.Create();

		// Add a transform component, since
		// 99% of entities require it
		reg.AddComponent<TransformComponent>(newEntity);
	}
	ImGui::SetItemTooltip("Create a new entity");

	ImGui::SameLine();

	const std::string searchFor = Search::DisplaySearchBar();

	if (!searchFor.empty())
	{
		Search::Choices<entt::entity> entitiesToDisplay{};
		const auto& allEntities = reg.Storage<entt::entity>();

		for (const auto& [entity] : allEntities.each())
		{
			entitiesToDisplay.emplace_back(NameComponent::GetDisplayName(reg, entity), entity);
		}

		Search::EraseChoicesThatDoNotMatch(searchFor, entitiesToDisplay);

		for (const Search::Choice<entt::entity>& entityToDisplay : entitiesToDisplay)
		{
			DisplaySingle(reg, entityToDisplay.mValue, *selectedEntities, nullptr);
		}
	}
	else
	{
		bool anyDisplayed{};

		// First we display all entities without transforms
		{
			const auto& allEntities = reg.Storage<entt::entity>();

			for (const auto& [entity] : allEntities.each())
			{
				if (reg.TryGet<TransformComponent>(entity) != nullptr)
				{
					continue;
				}

				DisplaySingle(reg, entity, *selectedEntities, nullptr);
				anyDisplayed = true;
			}
		}

		if (anyDisplayed)
		{
			ImGui::SeparatorText("Entities with transforms");
		}

		// Now we display only entities with transforms
		{
			auto withTransforms = reg.View<TransformComponent>();

			for (auto [entity, transform] : withTransforms.each())
			{
				// We recursively display children.
				// So we only call display family if
				// this transform does not have a parent.
				if (transform.IsOrphan())
				{
					DisplayFamily(reg, transform, *selectedEntities);
				}
			}
		}
	}

	DisplayRightClickPopUp(world, *selectedEntities);

	ImGui::InvisibleButton("DragToUnparent", glm::max(static_cast<glm::vec2>(ImGui::GetContentRegionAvail()), glm::vec2{ 1.0f, 1.0f }));
	ReceiveDragDropOntoParent(reg, std::nullopt);
	ReceiveDragDrops(world);
}

void Engine::WorldHierarchy::DisplayFamily(Registry& reg,
	TransformComponent& parentTransform,
	std::vector<entt::entity>& selectedEntities)
{
	const entt::entity entity = parentTransform.GetOwner();
	ImGui::PushID(static_cast<int>(entity));

	const std::vector<std::reference_wrapper<TransformComponent>>& children = parentTransform.GetChildren();

	bool isTreeNodeOpen{};

	if (!children.empty())
	{
		isTreeNodeOpen = ImGui::TreeNode("");
		ImGui::SameLine();
	}

	DisplaySingle(reg, entity, selectedEntities, &parentTransform);

	if (isTreeNodeOpen)
	{
		for (TransformComponent& childTransform : children)
		{
			DisplayFamily(reg, childTransform, selectedEntities);
		}

		ImGui::TreePop();
	}

	ImGui::PopID();
}

void Engine::WorldHierarchy::DisplaySingle(Registry& registry,
	const entt::entity owner,
	std::vector<entt::entity>& selectedEntities,
	TransformComponent* const transformComponent)
{
	const std::string displayName = NameComponent::GetDisplayName(registry, owner).append("##DisplayName");

	bool isSelected = std::find(selectedEntities.begin(), selectedEntities.end(), owner) != selectedEntities.end();
	const ImVec2 selectableAreaSize = ImGui::CalcTextSize(displayName.c_str());

	if (ImGui::Selectable(displayName.c_str(), &isSelected, 0, selectableAreaSize))
	{
		if (!Input::Get().IsKeyboardKeyHeld(Input::KeyboardKey::LeftControl))
		{
			selectedEntities.clear();
		}

		if (isSelected)
		{
			if (std::find(selectedEntities.begin(), selectedEntities.end(), owner) == selectedEntities.end())
			{
				selectedEntities.push_back(owner);
			}
		}
		else
		{
			selectedEntities.erase(std::remove(selectedEntities.begin(), selectedEntities.end(), owner),
				selectedEntities.end());
		}
	}

	ImGui::SetItemTooltip(Format("Entity {}", static_cast<EntityType>(owner)).c_str());

	// Only objects with transforms can accept children
	if (!transformComponent)
	{
		return;
	}

	if (!selectedEntities.empty())
	{
		DragDrop::SendEntities(selectedEntities);
	}

	ReceiveDragDropOntoParent(registry, owner);

	std::optional<WeakAsset<Prefab>> receivedPrefab = DragDrop::PeekAsset<Prefab>();

	if (!receivedPrefab.has_value()
		|| !DragDrop::AcceptAsset())
	{
		return;
	}

	entt::entity prefabEntity = registry.CreateFromPrefab(*receivedPrefab->MakeShared());

	TransformComponent* const prefabTransform = registry.TryGet<TransformComponent>(prefabEntity);
	TransformComponent* const parentTransform = registry.TryGet<TransformComponent>(owner);

	if (prefabTransform != nullptr
		&& parentTransform != nullptr)
	{
		prefabTransform->SetParent(parentTransform);
	}
}

void Engine::WorldHierarchy::ReceiveDragDropOntoParent(Registry& registry,
	std::optional<entt::entity> parentAllToThisEntity)
{
	const std::optional<std::vector<entt::entity>> receivedEntities = DragDrop::PeekEntities();

	if (receivedEntities.has_value())
	{
		bool doAllHaveTransforms = true;
		for (const entt::entity entity : *receivedEntities)
		{
			if (registry.TryGet<TransformComponent>(entity) == nullptr)
			{
				doAllHaveTransforms = false;
				break;
			}
		}

		if (doAllHaveTransforms
			&& DragDrop::AcceptEntities())
		{
			TransformComponent* const newParent = parentAllToThisEntity.has_value() ? registry.TryGet<TransformComponent>(*parentAllToThisEntity) : nullptr;

			for (const entt::entity entityToParent : *receivedEntities)
			{
				TransformComponent* childTransform = registry.TryGet<TransformComponent>(entityToParent);

				if (childTransform != nullptr)
				{
					childTransform->SetParent(newParent, true);
				}
			}
		}
	}
}

void Engine::WorldHierarchy::DisplayRightClickPopUp(World& world, std::vector<entt::entity>& selectedEntities)
{
	if (!ImGui::BeginPopup("HierarchyPopUp"))
	{
		return;
	}

	if (ImGui::MenuItem("Delete", "Del", nullptr, !selectedEntities.empty()))
	{
		DeleteEntities(world, selectedEntities);
	}

	if (ImGui::MenuItem("Duplicate", "Ctrl+D", nullptr, !selectedEntities.empty()))
	{
		Duplicate(world, selectedEntities);
	}

	if (ImGui::MenuItem("Cut", "Ctrl+X", nullptr, !selectedEntities.empty()))
	{
		CutToClipBoard(world, selectedEntities);
	}

	if (ImGui::MenuItem("Copy", "Ctrl+C", nullptr, !selectedEntities.empty()))
	{
		CopyToClipBoard(world, selectedEntities);
	}

	std::optional<std::string_view> clipboardData = GetSerializedEntitiesInClipboard();
	if (ImGui::MenuItem("Paste", "Ctrl+V", nullptr, clipboardData.has_value()))
	{
		PasteClipBoard(world, selectedEntities, *clipboardData);
	}

	ImGui::EndPopup();
}

namespace
{
	void RemoveInvalidEntities(Engine::World& world, std::vector<entt::entity>& selectedEntities)
	{
		selectedEntities.erase(std::remove_if(selectedEntities.begin(), selectedEntities.end(),
			[&world](const entt::entity& entity)
			{
				return !world.GetRegistry().Valid(entity);
			}), selectedEntities.end());
	}

	void DeleteEntities(Engine::World& world, std::vector<entt::entity>& selectedEntities)
	{
		world.GetRegistry().Destroy(selectedEntities.begin(), selectedEntities.end(), true);
		selectedEntities.clear();
	}


	// We prepend some form of known string so we can verify whether
	// the clipboard holds copied entities
	constexpr std::string_view sCopiedEntitiesId = "B1C2FF80";

	struct WasRootCopyTag
	{
		static Engine::MetaType Reflect()
		{
			Engine::MetaType type{ Engine::MetaType::T<WasRootCopyTag>{}, "WasRootCopyTag" };
			Engine::ReflectComponentType<WasRootCopyTag>(type);
			return type;
		}
	};

	std::string CopyToClipBoard(const Engine::World& world, const std::vector<entt::entity>& selectedEntities)
	{
		using namespace Engine;

		if (selectedEntities.empty())
		{
			return {};
		}

		// Const_cast is fine, we remove the changes immediately afterwards.
		// We just want the tags to be serialized so we know what to return in the Paste function.
		Registry& reg = const_cast<Registry&>(world.GetRegistry());

		reg.AddComponents<WasRootCopyTag>(selectedEntities.begin(), selectedEntities.end());
		BinaryGSONObject object = Archiver::Serialize(world, selectedEntities, true);
		reg.RemoveComponents<WasRootCopyTag>(selectedEntities.begin(), selectedEntities.end());

		std::ostringstream strStream{};

		object.SaveToBinary(strStream);

		// Clipboards work with c strings,
		// since our binary data might contain a
		// \0 character, we convert it to HEX first
		const std::string clipBoardData = std::string{ sCopiedEntitiesId } + StringFunctions::BinaryToHex(strStream.str());
		ImGui::SetClipboardText(clipBoardData.c_str());
		return clipBoardData;
	}

	void CutToClipBoard(Engine::World& world, std::vector<entt::entity>& selectedEntities)
	{
		CopyToClipBoard(world, selectedEntities);
		DeleteEntities(world, selectedEntities);
	}

	void PasteClipBoard(Engine::World& world, std::vector<entt::entity>& selectedEntities, std::string_view clipBoardData)
	{
		if (!IsStringFromCopyToClipBoard(clipBoardData))
		{
			return;
		}

		using namespace Engine;

		// Force reflection, if it hasn't been already
		(void)MetaManager::Get().GetType<WasRootCopyTag>();

		BinaryGSONObject object{};

		{
			const std::string binaryCopiedEntities = StringFunctions::HexToBinary(clipBoardData.substr(sCopiedEntitiesId.size()));
			view_istream stream{ binaryCopiedEntities };

			if (!object.LoadFromBinary(stream))
			{
				LOG(LogWorld, Error, "Trying to paste entities, but the provided string was unexpectedly invalid");
				return;
			}
		}

		selectedEntities = Archiver::Deserialize(world, object);

		Registry& reg = world.GetRegistry();

		// We only return the entities that were passed into the Paste function,
		// otherwise we would be returning the children, and thats difficult for
		// the caller to sort out.
		selectedEntities.erase(std::remove_if(selectedEntities.begin(), selectedEntities.end(),
			[&reg](entt::entity entity)
			{
				return !reg.HasComponent<WasRootCopyTag>(entity);
			}), selectedEntities.end());

		// The tag got copied as well, lets remove that as well
		reg.RemoveComponents<WasRootCopyTag>(selectedEntities.begin(), selectedEntities.end());
	}

	void Duplicate(Engine::World& world, std::vector<entt::entity>& selectedEntities)
	{
		std::string clipboardData = CopyToClipBoard(world, selectedEntities);
		PasteClipBoard(world, selectedEntities, clipboardData);
	}

	std::optional<std::string_view> GetSerializedEntitiesInClipboard()
	{
		const char* clipBoardDataCStr = ImGui::GetClipboardText();

		if (clipBoardDataCStr == nullptr)
		{
			return std::nullopt;
		}

		const std::string_view clipBoardData{ clipBoardDataCStr };

		if (IsStringFromCopyToClipBoard(clipBoardData))
		{
			return clipBoardData;
		}

		return std::nullopt;
	}

	bool IsStringFromCopyToClipBoard(std::string_view string)
	{
		return string.substr(0, sCopiedEntitiesId.size()) == sCopiedEntitiesId;
	}

	void CheckShortcuts(Engine::World& world, std::vector<entt::entity>& selectedEntities)
	{
		using namespace Engine;

		RemoveInvalidEntities(world, selectedEntities);

		if (Input::Get().WasKeyboardKeyReleased(Input::KeyboardKey::Delete))
		{
			DeleteEntities(world, selectedEntities);
		}

		if (Input::Get().IsKeyboardKeyHeld(Input::KeyboardKey::LeftControl)
			|| Input::Get().IsKeyboardKeyHeld(Input::KeyboardKey::RightControl))
		{
			if (Input::Get().WasKeyboardKeyPressed(Input::KeyboardKey::C))
			{
				CopyToClipBoard(world, selectedEntities);
			}

			std::optional<std::string_view> copiedEntities = GetSerializedEntitiesInClipboard();
			if (copiedEntities.has_value()
				&& Input::Get().WasKeyboardKeyPressed(Input::KeyboardKey::V))
			{
				PasteClipBoard(world, selectedEntities, *copiedEntities);
			}

			if (Input::Get().WasKeyboardKeyPressed(Input::KeyboardKey::X))
			{
				CutToClipBoard(world, selectedEntities);
			}

			if (Input::Get().WasKeyboardKeyPressed(Input::KeyboardKey::D))
			{
				Duplicate(world, selectedEntities);
			}

			if (Input::Get().WasKeyboardKeyPressed(Input::KeyboardKey::A))
			{
				const auto& entityStorage = world.GetRegistry().Storage<entt::entity>();
				selectedEntities = { entityStorage.begin(), entityStorage.end() };
			}
		}

		if (Input::Get().WasKeyboardKeyPressed(Input::KeyboardKey::Escape))
		{
			selectedEntities.clear();
		}

		if (Input::Get().WasKeyboardKeyPressed(Input::KeyboardKey::R))
		{
			sGuizmoOperation = ImGuizmo::SCALE;
		}

		if (Input::Get().WasKeyboardKeyPressed(Input::KeyboardKey::E))
		{
			sGuizmoOperation = ImGuizmo::ROTATE;
		}

		if (Input::Get().WasKeyboardKeyPressed(Input::KeyboardKey::T))
		{
			sGuizmoOperation = ImGuizmo::TRANSLATE;
		}
	}

	void ReceiveDragDrops(Engine::World& world)
	{
		using namespace Engine;

		std::optional<WeakAsset<Prefab>> receivedPrefab = DragDrop::PeekAsset<Prefab>();

		if (receivedPrefab.has_value()
			&& DragDrop::AcceptAsset())
		{
			world.GetRegistry().CreateFromPrefab(*receivedPrefab->MakeShared());
		}

		std::optional<WeakAsset<StaticMesh>> receivedMesh = DragDrop::PeekAsset<StaticMesh>();

		if (receivedMesh.has_value()
			&& DragDrop::AcceptAsset())
		{
			Registry& reg = world.GetRegistry();
			entt::entity entity = reg.Create();

			reg.AddComponent<TransformComponent>(entity);
			StaticMeshComponent& meshComponent = reg.AddComponent<StaticMeshComponent>(entity);
			meshComponent.mStaticMesh = receivedMesh->MakeShared();
			meshComponent.mMaterial = Material::TryGetDefaultMaterial();
		}
	}
}