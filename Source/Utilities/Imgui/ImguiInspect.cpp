#include "Precomp.h"
#include "Utilities/Imgui/ImguiInspect.h"
#ifdef EDITOR
#include "Meta/MetaAny.h"
#include "Meta/MetaType.h"

bool CE::ShowInspectUI(const std::string& label, MetaAny& value)
{
    if (value == nullptr)
    {
        LOG(Editor, Warning, "Cannot inspect {}: value was nullpr", label);
        return false;
    }

    const MetaType* const valueType = value.TryGetType();

    if (valueType == nullptr)
    {
        LOG(Editor, Warning, "Cannot inspect {}: valueType was nullpr", label);
        return false;
    }

    FuncResult result = valueType->CallFunction(sShowInspectUIFuncName, label, value);

    if (result.HasError())
    {
        LOG(Editor, Error, "Inspecting failed for {} - {}", label, result.Error());
        return false;
    }

    ASSERT(result.HasReturnValue());

    return *result.GetReturnValue().As<bool>();
}

bool CE::CanBeInspected(const MetaType& type)
{
    return type.TryGetFunc(sShowInspectUIFuncName) != nullptr;
}
#endif // EDITOR