#include "equivalence_controls.h"

#include <imgui.h>

#include "theme.h"
#include "../render/shader_runner.h"

void render_equivalence_tolerance_control(EquivalenceSampleConfig& config)
{
    bool tolerant = config.max_delta_tolerance > 0;
    if (themed_checkbox("Allow float-reordering tolerance", &tolerant))
    {
        config.max_delta_tolerance = tolerant ? 1 : 0;
    }

    ImGui::BeginDisabled(!tolerant);
    int tolerance_value = tolerant ? config.max_delta_tolerance : 0;
    if (ImGui::SliderInt("Max delta tolerance", &tolerance_value, 1, 8))
    {
        config.max_delta_tolerance = tolerance_value;
    }
    ImGui::EndDisabled();
}
