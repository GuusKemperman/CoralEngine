#pragma once
#ifdef EDITOR
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4201)
#pragma warning(disable : 4201)
#endif
#include "imgui/auto.h"
#ifdef _MSC_VER
#pragma warning(pop)
#endif

#include "Utilities/Math.h"
#include "Utilities/Search.h"

namespace Engine
{
	/*
	Used ImGui::Auto to inspect the element, possibly changing its value.
		
	This function is meant for each individual data-member; not the entire component itself:

	For example:
		ShowInspectUI("mPosition", transform.mPosition);
	
		static std::vector<glm::vec3> positions{};
		if (ShowInspectUI("Positions", positions))
		{
			std::cout << "Value of positions changed! << std::endl;
		}
	*/
	template<typename T>
	bool ShowInspectUI(const std::string& label, T& value)
	{
		T valueBefore = value;
		ImGui::Auto(value, label);
		return valueBefore != value;
	}

	// Same as ShowInspectUI() but does not allow the user to change the value
	template<typename T>
	void ShowInspectUIReadOnly(const std::string& label, T& value)
	{
		ImGui::BeginDisabled();
		ImGui::Auto(value, label);
		ImGui::EndDisabled();
	}

	static inline constexpr Name sShowInspectUIFuncName = "Inspect"_Name;

	class MetaAny;

	/*
	Used ImGui::Auto to inspect the element, possibly changing it's value. 
	Will only work if the type of the passed in Any is reflected and has an Inspect function.
	
	This function is meant for each individual data-member; not the entire component itself:

	For example:
		ShowInspectUI("mPosition", transform.mPosition);

		static std::vector<glm::vec3> positions{};
		if (ShowInspectUI("Positions", positions))
		{
			std::cout << "Value of positions changed! << std::endl;
		}
	*/
	bool ShowInspectUI(const std::string& label, MetaAny& value);

	bool CanBeInspected(const MetaType& type);
}

IMGUI_AUTO_DEFINE_INLINE(template<>, glm::quat,
	glm::vec3 asEuler = glm::eulerAngles(var) * glm::vec3{ 360.0f / TWOPI };
ImGui::Auto(asEuler, name);
var = glm::quat{ asEuler * glm::vec3{ TWOPI / 360.0f } };)

IMGUI_AUTO_DEFINE_INLINE(template<>, entt::entity, ImGui::Auto(reinterpret_cast<entt::id_type&>(var), name);)

namespace ImGui
{
	template<typename EnumType>
	struct Auto_t<EnumType, std::enable_if_t<Engine::sIsEnumReflected<EnumType>>>
	{
		static void Auto(EnumType& var, const std::string& name)
		{
			var = Engine::Search::DisplayDropDownWithSearchBar<EnumType>(name.c_str(), Engine::EnumToString(var).data()).value_or(var);
		}
		static constexpr bool sIsSpecialized = true;
	};

	template<typename T>
	struct Auto_t<std::optional<T>>
	{
		static void Auto(std::optional<T>& var, const std::string& name)
		{
			bool hasValue = var.has_value();

			if (ImGui::TreeNode(name.c_str()))
			{
				if (ImGui::Checkbox("HasValue", &hasValue))
				{
					if (var.has_value())
					{
						var.reset();
					}
					else
					{
						var.emplace();
					}
				}

				if (var.has_value())
				{
					ImGui::Auto(*var, name);
				}

				ImGui::TreePop();
			}
		}
		static constexpr bool sIsSpecialized = true;
	};
	
}

#endif // EDITOR