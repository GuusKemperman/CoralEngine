
struct ImVec2;

namespace ImGui
{
    /*
    Example of use:

    ImVec2 foo[10];
    ...
    foo[0].x = -1; // init data so editor knows to take it from here
    ...
    if (ImGui::Curve("Das editor", ImVec2(600, 200), 10, foo))
    {
        // curve changed
    }
    ...
    float value_you_care_about = ImGui::CurveValue(0.7f, 10, foo); // calculate value at position 0.7
*/
    int Curve(const char* label, const ImVec2& size, int maxpoints, ImVec2* points);
    float CurveValue(float p, int maxpoints, const ImVec2* points);
    float CurveValueSmooth(float p, int maxpoints, const ImVec2* points);
};