#include "Precomp.h"
#include "BasicDataTypes/CollisionRules.h"

#include "Meta/MetaType.h"
#include "Meta/MetaProps.h"
#include "Utilities/Reflect/ReflectComponentType.h"
#include "Utilities/Reflect/ReflectFieldType.h"

#ifdef EDITOR
void CE::CollisionRules::DisplayWidget(const std::string& name)
{
	ImGui::SetNextItemOpen(true, ImGuiCond_Once);
	if (ImGui::TreeNode(name.c_str()))
	{
		const auto currentPreset = std::find_if(sCollisionPresets.begin(), sCollisionPresets.end(),
			[this](const CollisionPreset& preset)
			{
				return preset.mRules == *this;
			});

		if (ImGui::BeginCombo("Collision presets", currentPreset == sCollisionPresets.end() ? "Custom" : currentPreset->mName.data()))
		{
			for (const CollisionPreset& preset : sCollisionPresets)
			{
				if (ImGui::MenuItem(preset.mName.data(), nullptr, currentPreset != sCollisionPresets.end() && &preset == &*currentPreset))
				{
					*this = preset.mRules;
				}
			}

			ImGui::EndCombo();
		}

		ShowInspectUI("Object type", mLayer);

		static constexpr int numOfResponses = static_cast<int>(CollisionResponse::NUM_OF_RESPONSES);

		if (ImGui::BeginTable("##ResponseTable", numOfResponses + 1))
		{
			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);

			for (const auto& [value, str] : sEnumStringPairs<CollisionResponse>)
			{
				ImGui::TableNextColumn();
				ImGui::TextUnformatted(str.data());
			}

			for (int layer = 0, id = 213456; layer < static_cast<int>(CollisionLayer::NUM_OF_LAYERS); layer++)
			{
				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(0);
				ImGui::TextUnformatted(EnumToString(static_cast<CollisionLayer>(layer)).data());

				for (int response = 0; response < numOfResponses; response++, id *= 2)
				{
					bool isTicked = mResponses[layer] == static_cast<CollisionResponse>(response);

					ImGui::TableNextColumn();
					ImGui::PushID(id);

					if (ImGui::Checkbox("", &isTicked))
					{
						mResponses[layer] = static_cast<CollisionResponse>(response);
					}

					ImGui::PopID();
				}
			}
			ImGui::EndTable();
		}

		ImGui::TreePop();
	}
}
#endif // EDITOR

CE::MetaType Reflector<CE::CollisionLayer>::Reflect()
{
	using namespace CE;
	using T = CollisionLayer;
	MetaType type{ MetaType::T<T>{}, "CollisionLayer" };

	type.GetProperties().Add(Props::sIsScriptableTag).Add(Props::sIsScriptOwnableTag);
	type.AddFunc(std::equal_to<T>(), OperatorType::equal).GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc(std::not_equal_to<T>(), OperatorType::inequal).GetProperties().Add(Props::sIsScriptableTag);
	ReflectFieldType<T>(type);

	return type;
}

CE::MetaType Reflector<CE::CollisionRules>::Reflect()
{
	using namespace CE;
	MetaType type{ MetaType::T<CollisionRules>{}, "CollisionRules" };
	type.GetProperties().Add(Props::sIsScriptableTag).Add(Props::sIsScriptOwnableTag);

	ReflectFieldType<CollisionRules>(type);
	return type;
}

void cereal::save(BinaryOutputArchive& ar, const CE::CollisionRules& value, uint32 )
{
	ar(value.mLayer, value.mResponses);
}

void cereal::load(BinaryInputArchive& ar, CE::CollisionRules& value, uint32 )
{
	ar(value.mLayer, value.mResponses);
}
