#pragma once
#include "Utilities/Imgui/ImguiInspect.h"
#include "Meta/MetaReflect.h"

namespace CE
{
	struct LinearColor : public glm::vec<4, float>
	{
		using DataType = vec<4, float>;
		using DataType::DataType;

		constexpr LinearColor(glm::vec4 v) : DataType(v) {};

		constexpr vec<4, uint8> ToPacked() const { return static_cast<vec<4, uint8>>(*this * 255.0f); }

	private:
		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(LinearColor);
	};
}

#ifdef EDITOR
IMGUI_AUTO_DEFINE_INLINE(template<>, CE::LinearColor, ImGui::ColorEdit4(name.c_str(), static_cast<float*>(&var.r));)
#endif // EDITOR
