#pragma once
#include "LinearColor.h"
#include "Utilities/Imgui/ImguiInspect.h"

namespace Engine
{
	class ColorGradient
	{
	public:
		LinearColor GetColorAt(float t) const;
		
		bool operator==(const ColorGradient& other) const { return other.mPoints == mPoints; }
		bool operator!=(const ColorGradient& other) const { return other.mPoints != mPoints; }

		using Points = std::vector<std::pair<float, LinearColor>>;
		Points mPoints{ { 0.0f, LinearColor{ 0.0f } }, { 1.0f,  LinearColor{1.0f} } };

	private:
		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(ColorGradient);
	};

template<class Archive>
void save(Archive& ar, const ColorGradient& value)
{
	save(ar, reinterpret_cast<const ColorGradient::Points&>(value));
}

template<class Archive>
void load(Archive& ar, ColorGradient& value)
{
	load(ar, reinterpret_cast<ColorGradient::Points&>(value));
}
}

#ifdef EDITOR
IMGUI_AUTO_DEFINE_INLINE(template<>, Engine::ColorGradient, ImGui::Auto(var.mPoints, name);)
#endif // EDITOR
