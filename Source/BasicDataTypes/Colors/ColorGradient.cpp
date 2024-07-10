#include "Precomp.h"
#include "BasicDataTypes/Colors/ColorGradient.h"

#include "Meta/MetaType.h"
#include "Utilities/Math.h"
#include "Utilities/Reflect/ReflectFieldType.h"

CE::LinearColor CE::ColorGradient::GetColorAt(float t) const
{
    if (mPoints.empty())
    {
        return {};
    }

    if (mPoints.size() == 1)
    {
        return mPoints.front().second;
    }

    const size_t numOfIterations = mPoints.size() - 1;
    for (size_t i = 0; i < numOfIterations; i++)
    {
        if (mPoints[i + 1].first < t)
        {
            continue;
        }

        const float timeInbetween = Math::lerpInv(mPoints[i].first, mPoints[i + 1].first, t);
        return Math::lerp(mPoints[i].second, mPoints[i + 1].second, timeInbetween);
    }

    return mPoints.back().second;
}

CE::MetaType CE::ColorGradient::Reflect()
{
    MetaType type = MetaType{ MetaType::T<ColorGradient>{}, "ColorGradient" };
    ReflectFieldType<ColorGradient>(type);
    return type;
}
