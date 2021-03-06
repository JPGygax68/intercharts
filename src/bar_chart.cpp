#include <iostream>
#include <imgui.h>
#include <intercharts/bar_chart.h>


// TODO: reordering of bars
// TODO: deleting bars


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
            if (mouse_interaction != Mouse_interaction::None) endBarDraggingOp();
            highlightHoveredHotspot(rect); // interaction_rects); // TODO: replace with code that does not rely on hotspot rects
        }

        drawBar(rect, bar_index == dragging_op.bar_index ? 0x800000FF : 0xFF0000FF);

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
        const auto rect = barToScreenRect(dragging_op.bar);
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

void Bar_chart::drawMouseDragHint(const std::string& hint)
{
    auto &io = ImGui::GetIO();
    auto fdl = ImGui::GetForegroundDrawList();
    auto ds = dpiScalingFactor();
    fdl->AddText(nullptr, labelFontSize(), {io.MousePos.x + ds * 3, io.MousePos.y - ds * 2 - labelFontSize()}, 0xFFFFFFFF, hint.c_str());
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

bool Bar_chart::highlightHoveredHotspot(const ImVec4& bar_rect)
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

bool Bar_chart::tryBeginDraggingBarEdge(int bar_index, const ImVec4& bar_rect, Edge edge)
{
    auto clicked_pos = ImGui::GetIO().MouseClickedPos[0];

    if (screenRectContainsPos(edgeRect(bar_rect, edge), clicked_pos)) {

        mouse_interaction = Mouse_interaction::Dragging_bar_edge;
        dragging_op.bar_index = bar_index;
        dragging_op.edge = edge;
        dragging_op.bar = bars[bar_index];

        return true;
    }

    return false;
}

void Bar_chart::dragBarEdge()
{
    if (isMouseDragging())
    {
        auto edge = dragging_op.edge;

        Event event;
        event.type = Event::Reshaping_bar;
        event.edges.set(edge);
        event.bar_index = dragging_op.bar_index;
        event.bar = dragging_op.bar;

        const auto& bar = bars[dragging_op.bar_index];
        if      (edge == Edge::Left  ) event.bar.x1 = addHorzMouseDrag(bar.x1);
        else if (edge == Edge::Right ) event.bar.x2 = addHorzMouseDrag(bar.x2);
        if      (edge == Edge::Bottom) event.bar.y1 = addVertMouseDrag(bar.y1);
        else if (edge == Edge::Top   ) event.bar.y2 = addVertMouseDrag(bar.y2);

        if (event_handler(event)) {
            dragging_op.bar = event.bar;
        }

        drawMouseDragHint("DRAGGING EDGE"); // formatFloat(dragging_op.bar.x) + "," + formatFloat(dragging_op.bar.y));
    }
}

void Bar_chart::endBarDraggingOp()
{
    commitBarEdits();
    mouse_interaction = Mouse_interaction::None;
    dragging_op.bar_index = -1;
    dragging_op.edge = Edge::None;
}

bool Bar_chart::tryBeginDraggingBarCorner(int bar_index, const ImVec4& bar_rect, Edge left_or_right, Edge bottom_or_top)
{
    auto clicked_pos = ImGui::GetIO().MouseClickedPos[0];

    if (screenRectContainsPos(cornerRect(bar_rect, left_or_right, bottom_or_top), clicked_pos)) {
        auto &bar = bars[bar_index];
        dragging_op.bar_index = bar_index;
        dragging_op.corner = Corner{ left_or_right, bottom_or_top };
        dragging_op.bar = bar;
        mouse_interaction = Mouse_interaction::Dragging_bar_corner;
    }

    return false;
}

void Bar_chart::dragBarCorner()
{
    if (isMouseDragging()) {

        Event event;
        event.type = Event::Reshaping_bar;
        event.bar = dragging_op.bar;
        event.bar_index = dragging_op.bar_index;
        event.edges.set(dragging_op.corner.left_or_right);
        event.edges.set(dragging_op.corner.bottom_or_top);

        const auto& bar = bars[dragging_op.bar_index];
        const auto& corner = dragging_op.corner;
        if      (corner.left_or_right == Edge::Left  ) event.bar.x1 = addHorzMouseDrag(bar.x1);
        else if (corner.left_or_right == Edge::Right ) event.bar.x2 = addHorzMouseDrag(bar.x2);
        if      (corner.bottom_or_top == Edge::Bottom) event.bar.y1 = addVertMouseDrag(bar.y1);
        else if (corner.bottom_or_top == Edge::Top   ) event.bar.y2 = addVertMouseDrag(bar.y2);

        if (event_handler(event)) {
            dragging_op.bar = event.bar;
        }
    }
}

bool Bar_chart::tryBeginDraggingBar(int bar_index, const ImVec4& bar_rect)
{
    auto clicked_pos = ImGui::GetIO().MouseClickedPos[0];

    if (screenRectContainsPos(interiorRect(bar_rect), clicked_pos)) {

        mouse_interaction = Mouse_interaction::Dragging_bar;
        dragging_op.bar_index = bar_index;
        dragging_op.bar = bars[bar_index];

        return true;
    }

    return false;
}

void Bar_chart::dragBar()
{
    if (isMouseDragging()) {
    
        Event event;
        event.type = Event::Moving_bar;
        event.bar_index = dragging_op.bar_index;
        event.bar = moveBarBy(bars[event.bar_index], mouseDragInPlotUnits());

        if (event_handler(event))
        {
            dragging_op.bar = event.bar;
        }
    }
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
    event.bar.x1 = roundX(screenToPlotUnitsX(left_to_right ? click_pos.x : mouse_pos.x));
    event.bar.x2 = roundX(screenToPlotUnitsX(left_to_right ? mouse_pos.x : click_pos.x));
    event.bar.y1 = roundY(screenToPlotUnitsY(bottom_to_top ? click_pos.y : mouse_pos.y));
    event.bar.y2 = roundY(screenToPlotUnitsY(bottom_to_top ? mouse_pos.y : click_pos.y));
    assert(event.bar.x1 <= event.bar.x2 && event.bar.y1 <= event.bar.y2);

    if (event.bar.x2 > event.bar.x1 && event.bar.y2 > event.bar.y1) {
        if (event_handler(event)) {
            dragging_op.bar = event.bar;
        }
    }
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
    event.bar_index = dragging_op.bar_index;
    event.bar = dragging_op.bar;
    if (event_handler(event)) {
        if (dragging_op.bar_index < 0) 
            bars.push_back(event.bar);
        else
            bars[dragging_op.bar_index] = event.bar;
        return true;
    }
    return false;
}

auto Bar_chart::roundX(float x) const -> float
{
    return rounding_unit_x * round(x / rounding_unit_x);
}

auto Bar_chart::roundY(float y) const -> float
{
    return rounding_unit_y * round(y / rounding_unit_y);
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
