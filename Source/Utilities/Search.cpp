#include "Precomp.h"
#include "Utilities/Search.h"

#include "imgui/imgui_internal.h"

std::string Engine::Search::DisplaySearchBar(std::string_view label, std::string_view hint)
{
    std::string str{};
    DisplaySearchBar(str, label, hint);
    return str;
}

void Engine::Search::DisplaySearchBar(std::string& searchTerm, std::string_view label, std::string_view hint)
{
    ImGui::InputTextWithHint(label.data(), hint.data(), &searchTerm);
}

bool Engine::Search::Internal::IsComboAlreadyOpen(std::string_view comboLabel)
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    const ImGuiID id = window->GetID(comboLabel.data());
    const ImGuiID popup_id = ImHashStr("##ComboPopup", 0, id);
    return ImGui::IsPopupOpen(popup_id, ImGuiPopupFlags_None);
}
