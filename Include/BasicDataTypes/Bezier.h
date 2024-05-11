#pragma once
#include "Utilities/Imgui/ImguiInspect.h"
#include "cereal/types/array.hpp"
#include "Meta/MetaReflect.h"

namespace CE
{
	class Bezier
	{
	public:
		using ValueStorage = std::array<glm::vec2, 8>;

		bool operator==(const Bezier& other) const { return other.mControlPoints == mControlPoints; }
		bool operator!=(const Bezier& other) const { return other.mControlPoints != mControlPoints; }

		float GetSurfaceAreaBetweenFast(float t1, float t2) const;

		float GetSurfaceAreaBetween(float t1, float t2, float stepSize) const;

		FORCE_INLINE float GetValueAt(float time) const;

#ifdef EDITOR
		void DisplayWidget(const char* label);
#endif // EDITOR

		ValueStorage mControlPoints{ glm::vec2{0.0f}, glm::vec2{1.0f}, glm::vec2{-1.0f} };

	private:
		static FORCE_INLINE float spline(const float* key, float t);

		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(Bezier);
	};

	// Much faster than ImGui::CurveValue, because we can take advantage of having a fixed size array
	float Bezier::GetValueAt(const float time) const
	{
		if (time <= mControlPoints[0].x) return mControlPoints[0].y;

		const float* input = reinterpret_cast<const float*>(mControlPoints.data());
		const float myValue = spline(input, time);

		// ASSERT(myValue == ImGui::CurveValueSmooth(time, static_cast<int>(mControlPoints.size()), reinterpret_cast<const ImVec2*>(mControlPoints.data())));

		return myValue;
	}

	inline float Bezier::spline(const float* key, float t)
	{
		constexpr int num = std::tuple_size_v<ValueStorage>;

		static constexpr signed char coefs[16] = {
			-1, 2,-1, 0,
			3,-5, 0, 2,
			-3, 4, 1, 0,
			1,-1, 0, 0 };

		const int size = 1 + 1;

		// find key
		int k = 0;

		while (key[k * size] < t) k++;

		// interpolant
		const float h = (t - key[(k - 1) * size]) / (key[k * size] - key[(k - 1) * size]);

		float v0{};

		// add basis functions
		for (int i = 0; i < 4; i++)
		{
			int kn = k + i - 2; if (kn < 0) kn = 0; else if (kn > (num - 1)) kn = num - 1;

			const signed char* co = coefs + 4 * i;

			const float b = 0.5f * (((co[0] * h + co[1]) * h + co[2]) * h + co[3]);

			v0 += b * key[kn * size + 1];
		}

		return v0;
	}

	template<class Archive>
	void save(Archive& ar, const Bezier& value)
	{
		save(ar, reinterpret_cast<const Bezier::ValueStorage&>(value));
	}

	template<class Archive>
	void load(Archive& ar, Bezier& value)
	{
		load(ar, reinterpret_cast<Bezier::ValueStorage&>(value));
	}
}

#ifdef EDITOR
IMGUI_AUTO_DEFINE_INLINE(template<>, CE::Bezier, var.DisplayWidget(name.c_str());)
#endif // EDITOR

