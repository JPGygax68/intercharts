#include <iostream>
#include <imgui.h>
#include <intercharts/bar_chart.h>

void Bar_chart::renderAndInteractWithValues()
{
    using MI = Mouse_interaction;

    auto wdl = ImGui::GetWindowDrawList();
    auto fdl = ImGui::GetForegroundDrawList();
    auto &io = ImGui::GetIO();

    int i_bar = 0;
    for (const auto &bar : bars)
    {
        const auto rect = barToScreenRect(bar);
        const auto interaction_rects = getBarInteractionZoneRects(rect);

        // Was there a click? (the "item" is the InvisibleButton representing the Chart)
        if (ImGui::IsItemActive())
        {
            const auto click_pos = io.MouseClickedPos[0];
            if (mouse_interaction == MI::Dragging_bar_zone) {
                updateDragOperation();
                fdl->AddLine(click_pos, io.MousePos, ImGui::GetColorU32(ImGuiCol_Button), 4.0f);
            }
            else if (mouse_interaction == MI::None) {
                if (tryStartDragOperationOnExistingBar(i_bar, interaction_rects)) ;
                // else if (...);
            }
        }
        else
        {
            if      (mouse_interaction == MI::Dragging_bar_zone) endDragOperation();
            else if (mouse_interaction == MI::Adding_new_bar   ) endAddNewBarOperation();
            highlightHoveredInteractionZone(interaction_rects);
        }

        drawBar(rect, i_bar == dragged_bar_index);

        ++i_bar;
    }
}

void Bar_chart::afterRenderingValues()
{
    using MI = Mouse_interaction;

    if (mouse_interaction == MI::None) {
        if (ImGui::IsItemActive()) {
            beginAddNewBarOperation();
        }
    }
    else if (mouse_interaction == MI::Adding_new_bar) {
        updateAddNewBarOperation();
    }
}

void Bar_chart::drawBar(const ImVec4& rect, bool dragging)
{
    auto wdl = ImGui::GetWindowDrawList();

    wdl->AddRectFilled({rect.x, rect.y}, {rect.z, rect.w}, dragging ? 0x800000FF : 0xFF0000FF);
    wdl->AddRect({rect.x, rect.y}, {rect.z, rect.w}, 0xFFFFFFFF);
}

bool Bar_chart::tryStartDragOperationOnExistingBar(int i_rect, const Interaction_zone_list& zones)
{
    for (auto i = 0U; i < zones.size(); i++)
    {
        // TODO: support disabling each zone
        const auto &rect = zones[i];
        if (screenRectContainsPos(rect, ImGui::GetIO().MouseClickedPos[0])) {
            beginDragOperation(i_rect, (Bar_zone)(Bar_zone::Left_edge + i));
            return true;
        }
    }

    return false;
}

void Bar_chart::highlightHoveredInteractionZone(const Interaction_zone_list& zones)
{
    for (auto i = 0U; i < zones.size(); i++)
    {
        // TODO: support disabling each zone
        const auto &rect = zones[i];
        if (screenRectContainsPos(rect, ImGui::GetIO().MousePos)) drawZoneHighlight(rect);
    }
}

void Bar_chart::beginDragOperation(int bar_index, Bar_zone zone)
{
    const auto &bar = bars[bar_index];
    switch (zone)
    {
    case Bar_zone::Left_edge:
        x_at_drag_start = bar.x1;
        break;
    case Bar_zone::Top_edge:
        y_at_drag_start = bar.y2;
        break;
    case Bar_zone::Right_edge:
        x_at_drag_start = bar.x2;
        break;
    case Bar_zone::Bottom_edge:
        y_at_drag_start = bar.y1;
        break;
    case Bar_zone::Interior:
        x_at_drag_start = bar.x1;
        y_at_drag_start = bar.y1;
        break;
    default:
        assert(false);
    }

    mouse_interaction = Mouse_interaction::Dragging_bar_zone;
    dragged_bar_index = bar_index;
    dragged_zone = zone;
}

void Bar_chart::updateDragOperation()
{
    using namespace std::string_literals;

    auto drag_delta = ImGui::GetMouseDragDelta(0);
    if (drag_delta.x != 0 && drag_delta.y != 0)
    {

        auto &io = ImGui::GetIO();
        auto fdl = ImGui::GetForegroundDrawList();
        auto ds = dpiScalingFactor();

        const auto click_pos = io.MouseClickedPos[0];
        const auto mouse_pos = io.MousePos;

        float delta_x = (mouse_pos.x - click_pos.x) / pixelsPerUnitHorz();
        float delta_y = (mouse_pos.y - click_pos.y) / pixelsPerUnitVert();

        float new_x = roundXValue(x_at_drag_start + delta_x);
        float new_y = roundYValue(y_at_drag_start - delta_y);

        tryDragZoneTo(dragged_zone, new_x, new_y);

        // TODO: temporary, mostly for debugging -> replace
        fdl->AddText(nullptr, labelFontSize(), {mouse_pos.x + ds * 3, mouse_pos.y - ds * 2 - labelFontSize()}, 0xFFFFFFFF,
                     (formatFloat(new_x) + "," + formatFloat(new_y)).c_str());
    }
}

void Bar_chart::endDragOperation()
{
    // TODO: commit changed value

    mouse_interaction = Mouse_interaction::None;
    dragged_bar_index = -1;
    dragged_zone = Bar_zone::None;
}

void Bar_chart::beginAddNewBarOperation()
{
    mouse_interaction = Mouse_interaction::Adding_new_bar;
}

void Bar_chart::updateAddNewBarOperation()
{
    auto& io = ImGui::GetIO();

    const auto click_pos = io.MouseClickedPos[0];
    const auto mouse_pos = io.MousePos;

    new_bar.x1 = roundXValue(screenToPlotUnitsX(mouse_pos.x < click_pos.x ? mouse_pos.x : click_pos.x));
    new_bar.x2 = roundXValue(screenToPlotUnitsX(mouse_pos.x < click_pos.x ? click_pos.x : mouse_pos.x));
    new_bar.y1 = roundYValue(screenToPlotUnitsY(mouse_pos.y > click_pos.y ? mouse_pos.y : click_pos.y));
    new_bar.y2 = roundYValue(screenToPlotUnitsY(mouse_pos.y > click_pos.y ? click_pos.y : mouse_pos.y));
    assert(new_bar.x1 <= new_bar.x2 && new_bar.y1 <= new_bar.y2);
    std::cout << new_bar.x1 << "," << new_bar.x2 << "; " << new_bar.y1 << "," << new_bar.y2 << std::endl;

    if (new_bar.x2 > new_bar.x1 && new_bar.y2 > new_bar.y1) {
        float dummy_x = 0, dummy_y = 0;
        /* if (bar_change_applier(new_bar, bars.size(), Bar_zone::None, dummy_x, dummy_y)) */ {
            const auto rect = barToScreenRect(new_bar);
            drawBar(rect, false);
        }
    }
}

void Bar_chart::endAddNewBarOperation()
{
    // TODO... ?
    mouse_interaction = Mouse_interaction::None;
}

void Bar_chart::drawZoneHighlight(const ImVec4 &zone)
{
    auto fdl = ImGui::GetForegroundDrawList();

    fdl->AddRectFilled({zone.x, zone.y}, {zone.z, zone.w}, 0x8080FFFF); // TODO: color constant or setting
}

bool Bar_chart::tryDragZoneTo(Bar_zone zone, float &new_x, float &new_y)
{
    auto &bar = bars[dragged_bar_index];

    // TODO: callback-based item-to-bar conversion
    if (bar_change_applier(bar, dragged_bar_index, zone, new_x, new_y))
    {
        switch (zone)
        {
        case Left_edge:
            bar.x1 = new_x;
            break;
        case Right_edge:
            bar.x2 = new_x;
            break;
        case Top_edge:
            bar.y2 = new_y;
            break;
        case Bottom_edge:
            bar.y1 = new_y;
            break;
        case Interior:
            bar.x2 += new_x - bar.x1;
            bar.x1 = new_x;
            bar.y2 += new_y - bar.y1;
            bar.y1 = new_y;
            break;
        default:
            assert(false);
        }
    }

    return false;
}

auto Bar_chart::roundXValue(float x) -> float
{
    return rounding_unit_x * round(x / rounding_unit_x);
}

auto Bar_chart::roundYValue(float y) -> float
{
    return rounding_unit_y * round(y / rounding_unit_y);
}

auto Bar_chart::getBarInteractionZoneRects(const ImVec4 &rect) -> std::array<ImVec4, 5>
{
    return std::array<ImVec4, 5>{
        leftEdgeRect(rect),
        topEdgeRect(rect),
        rightEdgeRect(rect),
        bottomEdgeRect(rect),
        interiorRect(rect),
    };
}

auto Bar_chart::barToScreenRect(const Bar &bar) const -> ImVec4
{
    const auto &pa = plottingArea();
    auto ppux = pixelsPerUnitHorz();
    auto ppuy = pixelsPerUnitVert();

    return {
        pa.x + bar.x1 * ppux, // x = left
        pa.w - bar.y2 * ppuy, // y = top
        pa.x + bar.x2 * ppux, // z = right
        pa.w - bar.y1 * ppuy, // w = bottom
    };
}

auto Bar_chart::leftEdgeRect(const ImVec4 &rect) const -> ImVec4
{
    auto ds = dpiScalingFactor();

    return ImVec4{
        rect.x - 3 * ds, // left edge
        rect.y + 3 * ds,
        rect.x + 3 * ds,
        rect.w - 3 * ds};
}

auto Bar_chart::rightEdgeRect(const ImVec4 &rect) const -> ImVec4
{
    auto ds = dpiScalingFactor();

    return ImVec4{
        rect.z - 3 * ds, // right edge
        rect.y + 3 * ds,
        rect.z + 3 * ds,
        rect.w - 3 * ds};
}

auto Bar_chart::topEdgeRect(const ImVec4 &rect) const -> ImVec4
{
    auto ds = dpiScalingFactor();

    return ImVec4{
        rect.x + 3 * ds,
        rect.y - 3 * ds,
        rect.z - 3 * ds,
        rect.y + 3 * ds};
}

auto Bar_chart::bottomEdgeRect(const ImVec4 &rect) const -> ImVec4
{
    auto ds = dpiScalingFactor();

    return ImVec4{
        rect.x + 3 * ds,
        rect.w - 3 * ds,
        rect.z - 3 * ds,
        rect.w + 3 * ds};
}

auto Bar_chart::interiorRect(const ImVec4 &rect) const -> ImVec4
{
    auto ds = dpiScalingFactor();

    return ImVec4{
        rect.x + 3 * ds,
        rect.y + 3 * ds,
        rect.z - 3 * ds,
        rect.w - 3 * ds};
}

bool Bar_chart::screenRectContainsPos(const ImVec4 &rect, const ImVec2 &pos) const
{
    return pos.x >= rect.x && pos.x <= rect.z && pos.y >= rect.y && pos.y <= rect.w;
}
