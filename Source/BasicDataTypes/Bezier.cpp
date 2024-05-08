#include "Precomp.h"
#include "BasicDataTypes/Bezier.h"

#include "imgui/imgui_curves.h"
#include "Meta/MetaType.h"
#include "Utilities/Reflect/ReflectFieldType.h"

float CE::Bezier::GetSurfaceAreaBetweenFast(const float t1, const float t2) const
{
	const float v1 = GetValueAt(t1);
	const float v2 = GetValueAt(t2);

	const float avg = (v1 + v2) * .5f;
	const float dt = t2 - t1;

	return avg * dt;
}

float CE::Bezier::GetSurfaceAreaBetween(float t1, const float t2, const float stepSize) const
{
	float total{};

	while (t1 + stepSize < t2)
	{
		total += GetSurfaceAreaBetweenFast(t1, t1 + stepSize);
		t1 += stepSize;
	}

	total += GetSurfaceAreaBetweenFast(t1, t2);
	return total;
}


#ifdef EDITOR


void CE::Bezier::DisplayWidget(const char* label)
{
	if (!ImGui::TreeNode(label))
	{
		return;
	}

	const float width = ImGui::GetContentRegionAvail().x;
	constexpr float height = 300.0f;

	ImGui::Curve(label, { width, height }, static_cast<int>(mControlPoints.size()), reinterpret_cast<ImVec2*>(mControlPoints.data()));

	const size_t sizeBefore = mControlPoints.size();
	if (ShowInspectUI("ControlPoints", mControlPoints))
	{
		if (mControlPoints.size() != sizeBefore
			&& !mControlPoints.empty())
		{
			mControlPoints.back() = glm::vec2{ -1.0f };
		}

		std::sort(mControlPoints.begin(), mControlPoints.end(),
			[](glm::vec2 l, glm::vec2 r)
			{
				if (l.x >= 0.0f
					&& r.x >= 0.0f)
				{
					return l.x < r.x;
				}
				else
				{
					return l.x > r.x;
				}
			});
	}

	ImGui::TreePop();
}
#endif // EDITOR

CE::MetaType CE::Bezier::Reflect()
{
	MetaType type = MetaType{ MetaType::T<Bezier>{}, "Bezier" };
	type.GetProperties().Add(Props::sIsScriptableTag).Add(Props::sIsScriptOwnableTag);

	type.AddFunc([](const Bezier& bezier, float t1, float t2)
		{
			return bezier.GetSurfaceAreaBetween(t1, t2, 0.05f);
		}, "GetSurfaceAreaBetween", MetaFunc::ExplicitParams<const Bezier&, float, float>{}, "", "T1", "T2").GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc(&Bezier::GetValueAt, "GetValueAt", "", "Time").GetProperties().Add(Props::sIsScriptableTag);

	ReflectFieldType<Bezier>(type);
	return type;
}