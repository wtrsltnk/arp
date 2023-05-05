#ifndef IMGUI_KNOB_H
#define IMGUI_KNOB_H

#include <functional>
#include <imgui.h>
#include <string>
#include <vector>

namespace ImGui
{

    IMGUI_API bool Knob(
        char const *label,
        float *p_value,
        float v_min,
        float v_max,
        ImVec2 const &size,
        char const *tooltip = nullptr);

    IMGUI_API bool KnobUchar(
        char const *label,
        unsigned char *p_value,
        unsigned char v_min,
        unsigned char v_max,
        ImVec2 const &size,
        char const *tooltip = nullptr);

    IMGUI_API bool KnobInt(
        char const *label,
        int *p_value,
        int v_min,
        int v_max,
        ImVec2 const &size,
        char const *tooltip = nullptr);

    IMGUI_API void ShowTooltipOnHover(
        char const *tooltip);

} // namespace ImGui

#endif // IMGUI_KNOB_H
