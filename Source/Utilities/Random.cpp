#include "Precomp.h"
#include "Utilities/Random.h"

#include "Meta/MetaType.h"
#include "Meta/MetaProps.h"

CE::MetaType CE::Random::Reflect()
{
    MetaType type{ MetaType::T<Random>{}, "Random" };
    type.AddFunc(&SetSeed, "SetSeed").GetProperties().Add(Props::sIsScriptableTag);

    type.AddFunc(static_cast<uint32(*)()>(&Random::Uint32), "Uint32").GetProperties().Add(Props::sIsScriptableTag);
    type.AddFunc(static_cast<int32(*)()>(&Random::Int32), "Int").GetProperties().Add(Props::sIsScriptableTag);
    type.AddFunc(static_cast<float32(*)()>(&Random::Float), "Float").GetProperties().Add(Props::sIsScriptableTag);
    
    type.AddFunc(static_cast<uint32(*)(const uint32&, const uint32&)>(&Random::UInt32Range), "Uint32Range", "Min", "Max").GetProperties().Add(Props::sIsScriptableTag);
    type.AddFunc(static_cast<int32(*)(const int32&, const int32&)>(&Random::Int32Range), "IntRange", "Min", "Max").GetProperties().Add(Props::sIsScriptableTag);
    type.AddFunc(static_cast<float32(*)(const float32&, const float32&)>(&Random::FloatRange), "FloatRange", "Min", "Max").GetProperties().Add(Props::sIsScriptableTag);

    return type;
}
