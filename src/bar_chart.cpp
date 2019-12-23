#include <iostream>
#include <imgui.h>
#include <intercharts/bar_chart.h>


void Bar_chart::renderAndInteractWithValues()
{
    auto wdl = ImGui::GetWindowDrawList();
    auto fdl = ImGui::GetForegroundDrawList();
    auto &io = ImGui::GetIO();

    int bar_index = 0;
    for (const auto &bar : bars)
    {
        const auto rect = barToScreenRect(bar);
        const auto interaction_rects = getBarInteractionZoneRects(rect); // TODO: get rid of this (see below)

        // Was there a click? (the "item" is the InvisibleButton representing the Chart)
        if (ImGui::IsItemActive())
        {
            const auto click_pos = io.MouseClickedPos[0];
            if (mouse_interaction == Mouse_interaction::Dragging_bar_edge) {
                dragBarEdge();
                fdl->AddLine(click_pos, io.MousePos, ImGui::GetColorU32(ImGuiCol_Button), 4.0f);
            }
            else if (mouse_interaction == Mouse_interaction::Dragging_bar_corner) {
                dragBarCorner();
                fdl->AddLine(click_pos, io.MousePos, ImGui::GetColorU32(ImGuiCol_Button), 4.0f);
            }
            else if (mouse_interaction == Mouse_interaction::Dragging_bar) {
                dragBar();
                fdl->AddLine(click_pos, io.MousePos, ImGui::GetColorU32(ImGuiCol_Button), 4.0f);
            }
            else if (mouse_interaction == Mouse_interaction::None) {
                if (tryStartDragOperationOnExistingBar(bar_index)) ;
                // else if (...);
            }
        }
        else
        {
            if      (mouse_interaction == Mouse_interaction::Dragging_bar     ) endDraggingBar();
            else if (mouse_interaction == Mouse_interaction::Dragging_bar_edge) endDraggingBarEdge();
            else if (mouse_interaction == Mouse_interaction::Defining_new_bar ) endAddNewBarOperation(); // TODO: support cancelling with right mouse button?
            highlightHoveredHotspot(interaction_rects); // TODO: replace with code that does not rely on hotspot rects
        }

        drawBar(rect, bar_index == dragged_bar_index);

        ++bar_index;
    }
}

void Bar_chart::afterRenderingValues()
{
    if (mouse_interaction == Mouse_interaction::None) {
        if (ImGui::IsItemActive()) {
            beginAddNewBarOperation();
        }
    }
    else if (mouse_interaction == Mouse_interaction::Defining_new_bar) {
        updateAddNewBarOperation();
    }
}

void Bar_chart::drawBar(const ImVec4& rect, bool dragging)
{
    auto wdl = ImGui::GetWindowDrawList();

    wdl->AddRectFilled({rect.x, rect.y}, {rect.z, rect.w}, dragging ? 0x800000FF : 0xFF0000FF);
    wdl->AddRect({rect.x, rect.y}, {rect.z, rect.w}, 0xFFFFFFFF);
}

bool Bar_chart::tryStartDragOperationOnExistingBar(int bar_index)
{
    auto ds = dpiScalingFactor();
    auto clicked_pos = ImGui::GetIO().MouseClickedPos[0];
    auto rect = barToScreenRect(bars[bar_index]);

    bool on_left_edge   = abs(rect.x - clicked_pos.x) < 3 *ds;
    bool on_right_edge  = abs(rect.z - clicked_pos.x) < 3 *ds;
    bool on_bottom_edge = abs(rect.w - clicked_pos.y) < 3 *ds;
    bool on_top_edge    = abs(rect.y - clicked_pos.y) < 3 *ds;

    if      (on_left_edge  && on_top_edge   ) beginDraggingBarCorner(bar_index, Corner::Top_left    );
    else if (on_right_edge && on_top_edge   ) beginDraggingBarCorner(bar_index, Corner::Top_right   );
    else if (on_right_edge && on_bottom_edge) beginDraggingBarCorner(bar_index, Corner::Bottom_right);
    else if (on_left_edge  && on_bottom_edge) beginDraggingBarCorner(bar_index, Corner::Bottom_left );

    else if (on_left_edge  ) beginDraggingBarEdge(bar_index, Edge::Left  );
    else if (on_right_edge ) beginDraggingBarEdge(bar_index, Edge::Right );
    else if (on_top_edge   ) beginDraggingBarEdge(bar_index, Edge::Top   );
    else if (on_bottom_edge) beginDraggingBarEdge(bar_index, Edge::Bottom);
    
    else if (screenRectContainsPos(rect, clicked_pos)) beginDraggingBar(bar_index);
    else return false;

    return true;
}

void Bar_chart::highlightHoveredHotspot(const Hotspot_zone_list& zones)
{
    for (auto i = 0U; i < zones.size(); i++)
    {
        // TODO: support disabling each zone
        const auto &rect = zones[i];
        if (screenRectContainsPos(rect, ImGui::GetIO().MousePos)) drawZoneHighlight(rect);
    }
}

void Bar_chart::beginDraggingBarEdge(int bar_index, Edge edge)
{
    auto &bar = bars[bar_index];

    if      (edge == Edge::Top   ) y_at_drag_start = bar.y2;
    else if (edge == Edge::Bottom) y_at_drag_start = bar.y1;
    if      (edge == Edge::Left  ) x_at_drag_start = bar.x1;
    else if (edge == Edge::Right ) x_at_drag_start = bar.x2;

    dragged_bar_index = bar_index;
    dragged_edge = edge;

    mouse_interaction = Mouse_interaction::Dragging_bar_edge;
}

void Bar_chart::dragBarEdge()
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

        float delta_x =   (mouse_pos.x - click_pos.x) / pixelsPerUnitHorz();
        float delta_y = - (mouse_pos.y - click_pos.y) / pixelsPerUnitVert();

        float new_x = roundXValue(x_at_drag_start + delta_x);
        float new_y = roundYValue(y_at_drag_start + delta_y);

        tryDragEdgeTo(dragged_edge, new_x, new_y);

        // TODO: temporary, mostly for debugging -> replace
        fdl->AddText(nullptr, labelFontSize(), {mouse_pos.x + ds * 3, mouse_pos.y - ds * 2 - labelFontSize()}, 0xFFFFFFFF,
                     (formatFloat(new_x) + "," + formatFloat(new_y)).c_str());
    }
}

bool Bar_chart::tryDragEdgeTo(Edge edge, float new_x, float new_y)
{
    assert(dragged_bar_index >= 0 && dragged_bar_index < (signed)bars.size());

    Event event;
    event.type = Event::Reshaping_bar;
    event.edges.set(edge);
    event.bar_index = dragged_bar_index;
    event.bar = bars[dragged_bar_index];

    if      (edge == Edge::Left  ) event.bar.x1 = new_x;
    else if (edge == Edge::Right ) event.bar.x2 = new_x;
    if      (edge == Edge::Bottom) event.bar.y1 = new_y;
    else if (edge == Edge::Top   ) event.bar.y2 = new_y;

    if (event_handler(event)) {
        bars[dragged_bar_index] = event.bar;
    }

    return false;
}

void Bar_chart::endDraggingBarEdge()
{
    // TODO: commit changed value

    mouse_interaction = Mouse_interaction::None;
    dragged_bar_index = -1;
    dragged_edge = Edge::None;
}

void Bar_chart::beginDraggingBarCorner(int bar_index, Corner corner)
{
    auto &bar = bars[bar_index];

    if      (corner == Corner::Bottom_left  || corner == Corner::Top_left    ) x_at_drag_start = bar.x1;
    else if (corner == Corner::Bottom_right || corner == Corner::Top_right   ) x_at_drag_start = bar.x2;
    if      (corner == Corner::Bottom_left  || corner == Corner::Bottom_right) y_at_drag_start = bar.y1;
    else if (corner == Corner::Top_left     || corner == Corner::Top_right   ) y_at_drag_start = bar.y2;
}

void Bar_chart::dragBarCorner()
{
}

void Bar_chart::endDraggingBarCorner()
{
}

void Bar_chart::beginDraggingBar(int bar_index)
{
    mouse_interaction = Mouse_interaction::Dragging_bar;

    dragged_bar_index = bar_index;

    const auto& bar = bars[bar_index];
    x_at_drag_start = bar.x1;
    y_at_drag_start = bar.y1;
}

void Bar_chart::dragBar()
{
    auto &io = ImGui::GetIO();

    const auto click_pos = io.MouseClickedPos[0];
    const auto mouse_pos = io.MousePos;

    float delta_x =   (mouse_pos.x - click_pos.x) / pixelsPerUnitHorz();
    float delta_y = - (mouse_pos.y - click_pos.y) / pixelsPerUnitVert();

    float new_x = roundXValue(x_at_drag_start + delta_x);
    float new_y = roundYValue(y_at_drag_start + delta_y);

    Event event;
    event.type = Event::Moving_bar;
    event.bar_index = dragged_bar_index;
    event.bar = bars[dragged_bar_index];

    event.bar.x2 = new_x + event.bar.width();
    event.bar.x1 = new_x;
    event.bar.y2 = new_y + event.bar.height();
    event.bar.y1 = new_y;

    if (event_handler(event))
    {
        bars[dragged_bar_index] = event.bar;
    }
}

void Bar_chart::endDraggingBar()
{
    // TODO: commit

    mouse_interaction = Mouse_interaction::None;
}

void Bar_chart::beginAddNewBarOperation()
{
    mouse_interaction = Mouse_interaction::Defining_new_bar;
}

void Bar_chart::updateAddNewBarOperation()
{
    auto& io = ImGui::GetIO();

    const auto click_pos = io.MouseClickedPos[0];
    const auto mouse_pos = io.MousePos;

    Event event;
    event.type = Event::Defining_new_bar;
    event.bar_index = -1;

    bool left_to_right = mouse_pos.x >= click_pos.x;
    bool bottom_to_top = mouse_pos.y <= click_pos.y;
    event.bar.x1 = roundXValue(screenToPlotUnitsX(left_to_right ? click_pos.x : mouse_pos.x));
    event.bar.x2 = roundXValue(screenToPlotUnitsX(left_to_right ? mouse_pos.x : click_pos.x));
    event.bar.y1 = roundYValue(screenToPlotUnitsY(bottom_to_top ? click_pos.y : mouse_pos.y));
    event.bar.y2 = roundYValue(screenToPlotUnitsY(bottom_to_top ? mouse_pos.y : click_pos.y));
    assert(event.bar.x1 <= event.bar.x2 && event.bar.y1 <= event.bar.y2);

    if (new_bar.x2 > new_bar.x1 && new_bar.y2 > new_bar.y1) {
        if (event_handler(event)) {
            new_bar = event.bar;
            const auto rect = barToScreenRect(new_bar);
            drawBar(rect, false);
        }
    }
}

void Bar_chart::endAddNewBarOperation()
{
    // TODO...: commit
    mouse_interaction = Mouse_interaction::None;
}

void Bar_chart::drawZoneHighlight(const ImVec4 &zone)
{
    auto fdl = ImGui::GetForegroundDrawList();

    fdl->AddRectFilled({zone.x, zone.y}, {zone.z, zone.w}, 0x8080FFFF); // TODO: color constant or setting
}

auto Bar_chart::roundXValue(float x) -> float
{
    return rounding_unit_x * round(x / rounding_unit_x);
}

auto Bar_chart::roundYValue(float y) -> float
{
    return rounding_unit_y * round(y / rounding_unit_y);
}

bool Bar_chart::hotspotBelongsToEdge(Bar_hotspot spot, Edge edge)
{
    using HS = Bar_hotspot;
    static const Set<Bar_hotspot> LEFT_SPOTS{ HS::Bottom_left_corner, HS::Top_left_corner, HS::Left_edge };
    static const Set<Bar_hotspot> TOP_SPOTS{ HS::Top_left_corner, HS::Top_edge, HS::Top_right_corner };
    static const Set<Bar_hotspot> RIGHT_SPOTS{ HS::Top_right_corner, Right_edge, Bottom_right_corner };
    static const Set<Bar_hotspot> BOTTOM_SPOTS{ HS::Bottom_left_corner, HS::Bottom_edge, HS::Bottom_right_corner };

    switch (edge) {
    case Edge::Top   : return TOP_SPOTS.contains(spot);
    case Edge::Right : return RIGHT_SPOTS.contains(spot);
    case Edge::Bottom: return BOTTOM_SPOTS.contains(spot);
    case Edge::Left  : return LEFT_SPOTS.contains(spot);
    }

    return false;
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
