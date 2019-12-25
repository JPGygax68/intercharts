#include <iostream>
#include <imgui.h>
#include <intercharts/bar_chart.h>


// CLASS IMPLEMENTATION

void Bar_chart::renderAndInteractWithValues()
{
    auto wdl = ImGui::GetWindowDrawList();
    auto fdl = ImGui::GetForegroundDrawList();
    auto &io = ImGui::GetIO();

    const auto click_pos = io.MouseClickedPos[0];

    int bar_index = 0;
    for (const auto &bar : bars)
    {
        const auto rect = barToScreenRect(bar);
        // const auto interaction_rects = getBarInteractionZoneRects(rect); // TODO: get rid of this (see below)

        // Was there a click? (the "item" is the InvisibleButton representing the Chart)
        if (ImGui::IsItemActive())
        {
            if (mouse_interaction == Mouse_interaction::Dragging_bar_edge) {
                dragBarEdge();
            }
            else if (mouse_interaction == Mouse_interaction::Dragging_bar_corner) {
                dragBarCorner();
            }
            else if (mouse_interaction == Mouse_interaction::Dragging_bar) {
                dragBar();
            }
            else if (mouse_interaction == Mouse_interaction::None) {
                if (tryStartDragOperationOnExistingBar(bar_index)) ;
                // else if (...);
            }
        }
        else
        {
            if      (mouse_interaction == Mouse_interaction::Dragging_bar       ) endDraggingBar();
            else if (mouse_interaction == Mouse_interaction::Dragging_bar_edge  ) endDraggingBarEdge();
            else if (mouse_interaction == Mouse_interaction::Dragging_bar_corner) endDraggingBarEdge();
            else if (mouse_interaction == Mouse_interaction::Defining_new_bar   ) endAddNewBarOperation(); // TODO: support cancelling with right mouse button?
            highlightHoveredHotspot(rect); // interaction_rects); // TODO: replace with code that does not rely on hotspot rects
        }

        drawBar(rect, bar_index == dragged_bar_index ? 0x800000FF : 0xFF0000FF);

        ++bar_index;
    }

    if (mouse_interaction == Mouse_interaction::None) {
        if (ImGui::IsItemActive()) {
            beginAddNewBarOperation();
        }
    }
    else {
        if (mouse_interaction == Mouse_interaction::Defining_new_bar) {
            updateAddNewBarOperation();
        }
        const auto rect = barToScreenRect(interaction_bar);
        drawBar(rect, 0xE00000FF);

        fdl->AddLine(click_pos, io.MousePos, ImGui::GetColorU32(ImGuiCol_Button), 4.0f);
    }
}

void Bar_chart::drawBar(const ImVec4& rect, ImU32 color)
{
    auto wdl = ImGui::GetWindowDrawList();

    wdl->AddRectFilled({rect.x, rect.y}, {rect.z, rect.w}, color);
    wdl->AddRect({rect.x, rect.y}, {rect.z, rect.w}, 0xFFFFFFFF);
}

bool Bar_chart::tryStartDragOperationOnExistingBar(int bar_index)
{
    auto ds = dpiScalingFactor();
    auto clicked_pos = ImGui::GetIO().MouseClickedPos[0];
    auto rect = barToScreenRect(bars[bar_index]);

    if (tryBeginDraggingBarCorner(bar_index, rect, Edge::Left , Edge::Top   )) return true;
    if (tryBeginDraggingBarCorner(bar_index, rect, Edge::Right, Edge::Top   )) return true;
    if (tryBeginDraggingBarCorner(bar_index, rect, Edge::Right, Edge::Bottom)) return true;
    if (tryBeginDraggingBarCorner(bar_index, rect, Edge::Left , Edge::Bottom)) return true;

    if (tryBeginDraggingBarEdge(bar_index, rect, Edge::Top   )) return true;
    if (tryBeginDraggingBarEdge(bar_index, rect, Edge::Right )) return true;
    if (tryBeginDraggingBarEdge(bar_index, rect, Edge::Bottom)) return true;
    if (tryBeginDraggingBarEdge(bar_index, rect, Edge::Left  )) return true;

    if (tryBeginDraggingBar(bar_index, rect)) return true;

    return false;
}

bool Bar_chart::highlightHoveredHotspot(const ImVec4& bar_rect) // const Hotspot_zone_list& zones)
{
    auto mouse_pos = ImGui::GetIO().MousePos;

    if (highlightRectIfHovered(leftEdgeRect  (bar_rect), mouse_pos)) return true;
    if (highlightRectIfHovered(rightEdgeRect (bar_rect), mouse_pos)) return true;
    if (highlightRectIfHovered(topEdgeRect   (bar_rect), mouse_pos)) return true;
    if (highlightRectIfHovered(bottomEdgeRect(bar_rect), mouse_pos)) return true;

    if (highlightRectIfHovered(cornerRect(bar_rect, Edge::Left , Edge::Bottom), mouse_pos)) return true;
    if (highlightRectIfHovered(cornerRect(bar_rect, Edge::Right, Edge::Bottom), mouse_pos)) return true;
    if (highlightRectIfHovered(cornerRect(bar_rect, Edge::Left , Edge::Top   ), mouse_pos)) return true;
    if (highlightRectIfHovered(cornerRect(bar_rect, Edge::Right, Edge::Top   ), mouse_pos)) return true;

    return false;
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
    //event.bar = bars[dragged_bar_index];
    event.bar = interaction_bar;

    if      (edge == Edge::Left  ) event.bar.x1 = new_x;
    else if (edge == Edge::Right ) event.bar.x2 = new_x;
    if      (edge == Edge::Bottom) event.bar.y1 = new_y;
    else if (edge == Edge::Top   ) event.bar.y2 = new_y;

    if (event_handler(event)) {
        interaction_bar = event.bar;
    }

    return false;
}

void Bar_chart::endDraggingBarEdge()
{
    commitBarEdits();
    mouse_interaction = Mouse_interaction::None;
    dragged_bar_index = -1;
    dragged_edge = Edge::None;
}

bool Bar_chart::tryBeginDraggingBarCorner(int bar_index, const ImVec4& bar_rect, Edge left_or_right, Edge bottom_or_top)
{
    auto clicked_pos = ImGui::GetIO().MouseClickedPos[0];

    if (screenRectContainsPos(cornerRect(bar_rect, left_or_right, bottom_or_top), clicked_pos)) {
        auto &bar = bars[bar_index];
        x_at_drag_start = left_or_right == Edge::Left   ? bar.x1 : bar.x2;
        y_at_drag_start = bottom_or_top == Edge::Bottom ? bar.y1 : bar.y2;
        dragged_bar_index = bar_index;
        dragged_corner = Corner{ left_or_right, bottom_or_top };
        interaction_bar = bar;
        mouse_interaction = Mouse_interaction::Dragging_bar_corner;
    }

    return false;
}

void Bar_chart::dragBarCorner()
{
    auto& io = ImGui::GetIO();

    auto mouse_pos = io.MousePos;
    auto click_pos = io.MouseClickedPos[0];

    float delta_x =   (mouse_pos.x - click_pos.x) / pixelsPerUnitHorz();
    float delta_y = - (mouse_pos.y - click_pos.y) / pixelsPerUnitVert();

    float new_x = roundXValue(x_at_drag_start + delta_x);
    float new_y = roundYValue(y_at_drag_start + delta_y);

    Event event;
    event.type = Event::Reshaping_bar;
    event.bar = interaction_bar;
    event.bar_index = dragged_bar_index;
    event.edges.set(dragged_corner.left_or_right);
    event.edges.set(dragged_corner.bottom_or_top);
    if (dragged_corner.left_or_right == Edge::Left  ) event.bar.x1 = new_x; else event.bar.x2 = new_x;
    if (dragged_corner.bottom_or_top == Edge::Bottom) event.bar.y1 = new_y; else event.bar.y2 = new_y;

    if (event_handler(event)) {
        // bars[dragged_bar_index] = event.bar;
        interaction_bar = event.bar;
    }

    // TODO...
}

void Bar_chart::endDraggingBarCorner()
{
    // TODO...

    mouse_interaction = Mouse_interaction::None;
    dragged_bar_index = -1;
    dragged_corner = {};
}

bool Bar_chart::tryBeginDraggingBarEdge(int bar_index, const ImVec4& bar_rect, Edge edge)
{
    auto clicked_pos = ImGui::GetIO().MouseClickedPos[0];

    if (screenRectContainsPos(edgeRect(bar_rect, edge), clicked_pos)) {

        mouse_interaction = Mouse_interaction::Dragging_bar_edge;
        dragged_bar_index = bar_index;
        dragged_edge = edge;
        interaction_bar = bars[bar_index];

        // TODO: still needed now that we use copy of bar?
        const auto& bar = bars[bar_index];
        if      (edge == Edge::Left  ) x_at_drag_start = bar.x1;
        else if (edge == Edge::Right ) x_at_drag_start = bar.x2;
        else if (edge == Edge::Bottom) y_at_drag_start = bar.y1;
        else if (edge == Edge::Top   ) y_at_drag_start = bar.y2;

        return true;
    }

    return false;
}

bool Bar_chart::tryBeginDraggingBar(int bar_index, const ImVec4& bar_rect)
{
    auto clicked_pos = ImGui::GetIO().MouseClickedPos[0];

    if (screenRectContainsPos(interiorRect(bar_rect), clicked_pos)) {
        mouse_interaction = Mouse_interaction::Dragging_bar;

        dragged_bar_index = bar_index;

        interaction_bar = bars[bar_index];

        // TODO: still needed now that we use copy of bar?
        const auto& bar = bars[bar_index];
        x_at_drag_start = bar.x1;
        y_at_drag_start = bar.y1;

        return true;
    }

    return false;
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
    //event.bar = bars[dragged_bar_index];
    event.bar = interaction_bar;

    event.bar.x2 = new_x + event.bar.width();
    event.bar.x1 = new_x;
    event.bar.y2 = new_y + event.bar.height();
    event.bar.y1 = new_y;

    if (event_handler(event))
    {
        interaction_bar = event.bar;
    }
}

void Bar_chart::endDraggingBar()
{
    commitBarEdits();
    mouse_interaction = Mouse_interaction::None;
    dragged_bar_index = -1;
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

    bool left_to_right = mouse_pos.x >= click_pos.x;
    bool bottom_to_top = mouse_pos.y <= click_pos.y;
    event.bar.x1 = roundXValue(screenToPlotUnitsX(left_to_right ? click_pos.x : mouse_pos.x));
    event.bar.x2 = roundXValue(screenToPlotUnitsX(left_to_right ? mouse_pos.x : click_pos.x));
    event.bar.y1 = roundYValue(screenToPlotUnitsY(bottom_to_top ? click_pos.y : mouse_pos.y));
    event.bar.y2 = roundYValue(screenToPlotUnitsY(bottom_to_top ? mouse_pos.y : click_pos.y));
    assert(event.bar.x1 <= event.bar.x2 && event.bar.y1 <= event.bar.y2);

    if (event.bar.x2 > event.bar.x1 && event.bar.y2 > event.bar.y1) {
        if (event_handler(event)) {
            interaction_bar = event.bar;
            // const auto rect = barToScreenRect(interaction_bar);
            // drawBar(rect, true);
        }
    }
}

void Bar_chart::endAddNewBarOperation()
{
    commitBarEdits();
    mouse_interaction = Mouse_interaction::None;
}

bool Bar_chart::highlightRectIfHovered(const ImVec4 &rect, const ImVec2& mouse_pos)
{
    if (screenRectContainsPos(rect, mouse_pos)) {
        auto fdl = ImGui::GetForegroundDrawList();
        fdl->AddRectFilled({ rect.x, rect.y }, { rect.z, rect.w }, 0x8080FFFF); // TODO: color constant or setting
        return true;
    }

    return false;
}

bool Bar_chart::commitBarEdits()
{
    Event event;
    event.type = Event::Committing;
    event.bar_index = dragged_bar_index;
    event.bar = interaction_bar;
    if (event_handler(event)) {
        if (dragged_bar_index < 0) 
            bars.push_back(event.bar);
        else
            bars[dragged_bar_index] = event.bar;
        return true;
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

auto Bar_chart::cornerRect(const ImVec4& bar_rect, Edge left_or_right, Edge bottom_or_top) const -> ImVec4
{
    ImVec4 rect;

    auto ds = dpiScalingFactor();

    if (left_or_right == Edge::Left) {
        rect.x = bar_rect.x - ds * 3;
        rect.z = bar_rect.x + ds * 3;
    }
    else if (left_or_right == Edge::Right) {
        rect.x = bar_rect.z - ds * 3;
        rect.z = bar_rect.z + ds * 3;
    }
    else assert(false);

    if (bottom_or_top == Edge::Top) {
        rect.y = bar_rect.y - ds * 3;
        rect.w = bar_rect.y + ds * 3;
    }
    else if (bottom_or_top == Edge::Bottom) {
        rect.y = bar_rect.w - ds * 3;
        rect.w = bar_rect.w + ds * 3;
    }
    else assert(false);

    return rect;
}

auto Bar_chart::edgeRect(const ImVec4& rect, Edge edge) const -> ImVec4
{
    switch (edge) {
    case Edge::Left: return leftEdgeRect(rect);
    case Edge::Right: return rightEdgeRect(rect);
    case Edge::Bottom: return bottomEdgeRect(rect);
    case Edge::Top: return topEdgeRect(rect);
    }

    assert(false);
    return {};
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
