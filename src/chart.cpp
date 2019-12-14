#include <string>
#include <iostream>
#include <iomanip>
#include <array>
#include <cassert>
#include <cmath>
#include <sstream>
#include <imgui.h>
#include <intercharts/chart.h>


/* static auto format_float(float val, int prec = 0) {

    std::stringstream sstr;
    sstr.imbue(std::locale());
    sstr << std::setprecision(prec) << val << std::ends;
    return sstr.str();
} */

static auto format_float(float val, int prec = 0) -> std::string {
    char buffer[20];
    snprintf(buffer, sizeof(buffer), "%.*f", prec, val);
    return buffer;
}

Chart::Chart(float x2, float y2, float x1, float y1):
    x_min(x1), x_max(x2), y_min(y1), y_max(y2), dpi_scaling_factor(1)
{
}

void Chart::render(float w, float h, float dpi_scaling)
{
    // TODO: make into properties
    const int spacing = 4;

    updateGeometry(w, h, dpi_scaling);

    ImGui::InvisibleButton("Chart", ImVec2{ w_scaled, h_scaled });

    drawGrid();

    if (value_plotter) value_plotter->render();
}

void Chart::updateGeometry(float w, float h, float dpi_scaling)
{
    static const float spacing = 2;

    assert(x_grid_medium % x_grid_minor == 0);
    assert(x_grid_major % x_grid_medium == 0);

    dpi_scaling_factor = dpi_scaling;

    auto ds = dpi_scaling;

    ImVec2 p_scr = ImGui::GetCursorScreenPos();

    w_scaled = ds * w, h_scaled = ds * h;
    margin = 2 * ds * 0.5f * label_font_size; // estimate to leave enough room for first and last X labels
    w_grid = ds * w - margin - margin;
    h_grid = ds * (h - label_font_size - spacing); // vertical spacing between lower edge and X labels

    pixels_per_unit_x = w_grid / (x_max - x_min);     // meter/pixel ratio
    pixels_per_unit_y = h_grid / 200.0f;            // TODO: set upper limit by parameter

    x_grid = p_scr.x + margin;
    y_grid = p_scr.y;
    y_xaxis_labels = y_grid + h_grid + ds * spacing;
}

void Chart::drawGrid()
{
    auto dl = ImGui::GetWindowDrawList();
    auto ds = dpi_scaling_factor;

    // Vertical grid lines (on X axis)
    for (float x = x_min; x <= x_max; x += x_grid_minor) {

        bool at_medium_gridline = (int)x % x_grid_medium == 0;
        bool at_major_gridline  = (int)x % x_grid_major  == 0;

        float thickness = ds * 1.0f;
        if (at_medium_gridline) thickness = ds * 1.6f;
        if (at_major_gridline ) thickness = ds * 2.5f;

        float x_line = x_grid + pixels_per_unit_x * x;
        dl->AddLine(ImVec2{ x_line, y_grid }, ImVec2{ x_line, y_grid + h_grid }, 0xFF888888, thickness);

        if (at_medium_gridline) drawLabel(x_line, y_xaxis_labels, format_float(x, 0).c_str());
    }

    // TODO: horizontal grid lines (Y axis)

    // TODO: separate method so "box" can be drawn "over" the values?
    dl->AddRect(ImVec2{ x_grid, y_grid }, ImVec2{ x_grid + w_grid, y_grid + h_grid }, 0xFF888888);
}

void Chart::drawLabel(float x, float y, const char* label)
{
    auto ds = dpi_scaling_factor;

    auto dl = ImGui::GetWindowDrawList();
    auto size = ImGui::GetFont()->CalcTextSizeA(ds * label_font_size, FLT_MAX, FLT_MAX, label);
    
    dl->AddText(nullptr, ds * label_font_size, ImVec2{ x - size.x / 2, y }, 0xFFFFFFFF, label);
}

void Bar_plotter::render()
{
    auto wdl = ImGui::GetWindowDrawList();
    auto fdl = ImGui::GetForegroundDrawList();
    auto& io = ImGui::GetIO();

    int i_rect = 0;
    for (const auto& plot_rect : bars) {

        const auto rect = barToScreenRect(plot_rect);
        const auto interaction_rects = getBarInteractionZoneRects(rect);

        if (ImGui::IsItemActive()) {
            const auto click_pos = io.MouseClickedPos[0];
            if (!dragging) {
                for (auto i = 0U; i < interaction_rects.size(); i ++) {
                    // TODO: support disabling each zone
                    const auto& rect = interaction_rects[i];
                    if (screenRectContainsPos(rect, click_pos)) beginDragOperation(i_rect, (Bar_zone)(Bar_zone::Left_edge + i));
                }
            }
            if (dragging) {
                updateDragOperation();
                fdl->AddLine(click_pos, io.MousePos, ImGui::GetColorU32(ImGuiCol_Button), 4.0f);
            }
        }
        else {
            if (dragging) endDragOperation();
            const auto mouse_pos = io.MousePos;
            for (auto i = 0U; i < interaction_rects.size(); i ++) {
                // TODO: support disabling each zone
                const auto& rect = interaction_rects[i];
                if (screenRectContainsPos(rect, mouse_pos)) drawZoneHighlight(rect);
            }
        }

        // TODO: visually support dragging operations
        wdl->AddRectFilled({ rect.x, rect.y }, { rect.z, rect.w }, i_rect == dragged_bar_index ? 0x800000FF : 0xFF0000FF);
        wdl->AddRect({ rect.x, rect.y }, { rect.z, rect.w }, 0xFFFFFFFF);

        ++i_rect;
    }
}

void Bar_plotter::beginDragOperation(int bar_index, Bar_zone zone)
{
    const auto& bar = bars[bar_index];
    switch (zone) {
    case Bar_zone::Left_edge  : x_at_drag_start = bar.x1; break;
    case Bar_zone::Top_edge   : y_at_drag_start = bar.y2; break;
    case Bar_zone::Right_edge : x_at_drag_start = bar.x2; break;
    case Bar_zone::Bottom_edge: y_at_drag_start = bar.y1; break;
    case Bar_zone::Interior   : x_at_drag_start = bar.x1; y_at_drag_start = bar.y1; break;
    default: assert(false);
    }

    dragging = true;
    dragged_bar_index = bar_index;
    dragged_zone = zone;
}

void Bar_plotter::updateDragOperation()
{
    using namespace std::string_literals;

    auto drag_delta = ImGui::GetMouseDragDelta(0);
    if (drag_delta.x != 0 && drag_delta.y != 0) {

        auto& io = ImGui::GetIO();
        auto fdl = ImGui::GetForegroundDrawList();
        auto ds = chart().dpiScalingFactor();

        const auto click_pos = io.MouseClickedPos[0];
        const auto mouse_pos = io.MousePos;

        float delta_x = (mouse_pos.x - click_pos.x) / chart().pixelsPerUnitHorz();
        float delta_y = (mouse_pos.y - click_pos.y) / chart().pixelsPerUnitVert();

        float new_x = roundUnitsX(x_at_drag_start + delta_x);
        float new_y = roundUnitsY(y_at_drag_start - delta_y);

        tryDragZoneTo(dragged_zone, new_x, new_y);

        // TODO: temporary, mostly for debugging -> replace
        fdl->AddText(nullptr, chart().labelFontSize(), { mouse_pos.x + ds * 3, mouse_pos.y - ds * 2 - chart().labelFontSize() }, 0xFFFFFFFF, 
            (format_float(new_x) + "," + format_float(new_y)).c_str());
    }
}

void Bar_plotter::endDragOperation()
{
    // TODO: commit changed value

    dragging = false;
    dragged_bar_index = -1;
    dragged_zone = Bar_zone::None;
}

void Bar_plotter::drawZoneHighlight(const ImVec4& zone)
{
    auto fdl = ImGui::GetForegroundDrawList();

    fdl->AddRectFilled({ zone.x, zone.y }, { zone.z, zone.w }, 0x8080FFFF); // TODO: color constant or setting
}

bool Bar_plotter::tryDragZoneTo(Bar_zone zone, float& new_x, float& new_y)
{
    auto& bar = bars[dragged_bar_index];

    // TODO: callback-based item-to-bar conversion
    if (bar_change_applier(bar, dragged_bar_index, zone, new_x, new_y)) {
        switch (zone) {
        case Left_edge  : bar.x1 = new_x; break;
        case Right_edge : bar.x2 = new_x; break;
        case Top_edge   : bar.y2 = new_y; break;
        case Bottom_edge: bar.y1 = new_y; break;
        case Interior   : 
            bar.x2 += new_x - bar.x1; bar.x1 = new_x; 
            bar.y2 += new_y - bar.y1; bar.y1 = new_y; 
            break;
        default: 
            assert(false);
        }
    }

    return false;
}

auto Bar_plotter::roundUnitsX(float x) -> float
{
    return rounding_unit_x * round(x / rounding_unit_x);
}

auto Bar_plotter::roundUnitsY(float y) -> float
{
    return rounding_unit_y * round(y / rounding_unit_y);
}

auto Bar_plotter::getBarInteractionZoneRects(const ImVec4& rect) -> std::array<ImVec4, 5>
{
    return std::array<ImVec4, 5> {
        leftEdgeRect  (rect),
        topEdgeRect   (rect),
        rightEdgeRect (rect),
        bottomEdgeRect(rect),
        interiorRect (rect),
    };
}

auto Bar_plotter::barToScreenRect(const Bar& rect) const -> ImVec4
{
    const auto& pa = chart().plottingArea();
    auto ppux = chart().pixelsPerUnitHorz();
    auto ppuy = chart().pixelsPerUnitVert();

    return {
        pa.x + rect.x1 * ppux,     // x = left
        pa.w - rect.y2 * ppuy,     // y = top
        pa.x + rect.x2 * ppux,     // z = right
        pa.w - rect.y1 * ppuy,     // w = bottom
    };
}

auto Bar_plotter::leftEdgeRect(const ImVec4& rect) const -> ImVec4
{
    auto ds = chart().dpiScalingFactor();

    return ImVec4{
        rect.x - 3 * ds,    // left edge
        rect.y + 3 * ds,
        rect.x + 3 * ds,
        rect.w - 3 * ds
    };
}

auto Bar_plotter::rightEdgeRect(const ImVec4& rect) const -> ImVec4
{
    auto ds = chart().dpiScalingFactor();

    return ImVec4{
        rect.z - 3 * ds,    // right edge
        rect.y + 3 * ds,
        rect.z + 3 * ds,
        rect.w - 3 * ds
    };
}

auto Bar_plotter::topEdgeRect(const ImVec4& rect) const -> ImVec4
{
    auto ds = chart().dpiScalingFactor();

    return ImVec4{
        rect.x + 3 * ds,
        rect.y - 3 * ds,
        rect.z - 3 * ds,
        rect.y + 3 * ds
    };
}

auto Bar_plotter::bottomEdgeRect(const ImVec4& rect) const -> ImVec4
{
    auto ds = chart().dpiScalingFactor();

    return ImVec4{
        rect.x + 3 * ds,
        rect.w - 3 * ds,
        rect.z - 3 * ds,
        rect.w + 3 * ds
    };
}

auto Bar_plotter::interiorRect(const ImVec4& rect) const -> ImVec4
{
    auto ds = chart().dpiScalingFactor();

    return ImVec4{
        rect.x + 3 * ds,
        rect.y + 3 * ds,
        rect.z - 3 * ds,
        rect.w - 3 * ds
    };
}

bool Bar_plotter::screenRectContainsPos(const ImVec4& rect, const ImVec2& pos) const
{
    return pos.x >= rect.x && pos.x <= rect.z && pos.y >= rect.y && pos.y <= rect.w;
}

auto Value_plotter::plotToScreenUnitsX(float x) const -> float
{
    return chart().plottingArea().x + x * chart().pixelsPerUnitHorz(); // TODO: support origin != (0, 0)
}

auto Value_plotter::plotToScreenUnitsY(float y) const -> float
{
    return chart().plottingArea().w - y * chart().pixelsPerUnitVert(); // TODO: support origin != (0, 0)
}
