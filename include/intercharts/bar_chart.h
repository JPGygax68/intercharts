#pragma once

#include <array>
#include <set>
#include "set.h"
#include "chart.h"

class Bar_chart : public Chart
{
public:

    enum class Edge { None = 0, Left, Right, Bottom, Top };
    class Edges : public Set<Edge> {
    public:
        bool contains_left() const { return contains(Edge::Left); }
        bool contains_right() const { return contains(Edge::Right); }
        bool contains_bottom() const { return contains(Edge::Bottom); }
        bool contains_top() const { return contains(Edge::Top); }
    };

    //enum class Corner { None = 0, Top_left, Top_right, Bottom_right, Bottom_left };
    struct Corner {
        Edge left_or_right, bottom_or_top;
    };

    struct Bar
    {
        float x1, y1; // bottom left
        float x2, y2; // top right
        Bar() = default;
        Bar(float a, float b, float c, float d) : x1(a), y1(b), x2(c), y2(d) {}
        auto width () const { return x2 - x1; }
        auto height() const { return y2 - y1; }
        void moveXTo(float x) { x2 += x - x1; x1 = x; }
        void moveYTo(float y) { y2 += y - y1; y1 = y; }
    };

    struct Event { // TODO: more distinctive name
        enum { 
            None = 0, 
            Defining_new_bar, 
            Reshaping_bar,
            Moving_bar,
            Committing,
            Cancelling,
            Deleting_bar,

        } type = None;
        Edges edges = {};
        Bar bar;
        int bar_index = -1;

        bool definingNewBar() const { return type == Defining_new_bar; }
        bool reshapingBar() const { return type == Reshaping_bar; }
        bool movingBar() const { return type == Moving_bar; }
        bool committing() const { return type == Committing; }
        bool cancelling() const { return type == Cancelling; }
        bool deletingBar() const { return type == Deleting_bar; }
        int  barIndex() const { return bar_index >= 0 ? bar_index : -bar_index - 1; }
        bool barIsNew() const { return bar_index < 0; }
    };

    using Event_handler = std::function<bool(Event&)>;

    // Lifecycle

    using Chart::Chart;

    // Setup

    auto& onEditingEvent(Event_handler handler)
    {
        event_handler = handler;
        return *this;
    }

    auto& setRoundingUnitX(float unit)
    {
        rounding_unit_x = unit;
        return *this;
    }
    auto& setRoundingUnitY(float unit)
    {
        rounding_unit_y = unit;
        return *this;
    }

    auto& addRectangle(float x_low, float y_low, float x_high, float y_high)
    {
        bars.emplace_back(x_low, y_low, x_high, y_high);
        return *this;
    }

    void clearRectangles() { bars.clear(); }

    // Display & interaction

protected:

    void renderAndInteractWithValues() override;
    // void afterRenderingValues() override;

    void drawBar(const ImVec4& rect, ImU32 color);

    void drawMouseDragHint(const std::string& hint);

    auto roundXValue(float) const -> float;
    auto roundYValue(float) const -> float;

    void moveX(float& x, float dx) const { x = roundXValue(x + dx); }
    void moveY(float& y, float dy) const { y = roundYValue(y + dy); }

    auto add(const ImVec2& point, const ImVec2& delta) const -> ImVec2 {
        return { roundXValue(point.x + delta.x), roundYValue(point.y + delta.y) };
    }
    auto addX(float x, float dx) const -> float { return roundXValue(x + dx); }
    auto addY(float y, float dy) const -> float { return roundYValue(y + dy); }

    void moveXByMouseDrag(float& x) const { moveX(x, mouseDragInPlotUnits().x); }
    void moveYByMouseDrag(float& y) const { moveY(y, mouseDragInPlotUnits().y); }

    auto addHorzMouseDrag(float x) const -> float { return addX(x, mouseDragInPlotUnits().x); }
    auto addVertMouseDrag(float y) const -> float { return addY(y, mouseDragInPlotUnits().y); }
    auto addMouseDrag(const ImVec2& pos) const -> ImVec2 { return add(pos, mouseDragInPlotUnits()); }

private:

    enum class Mouse_interaction { 
        None = 0, 
        Dragging_bar_corner,
        Dragging_bar_edge,
        Dragging_bar,
        Defining_new_bar,             // click-and-drag to define a new Bar
    };

    using Hotspot_zone_list = std::array<ImVec4, 5>;

    bool tryStartDragOperationOnExistingBar(int i_rect);
    bool highlightHoveredHotspot(const ImVec4& bar_rect);
    bool highlightRectIfHovered(const ImVec4& rect, const ImVec2& mouse_pos);

    void endBarDraggingOp();

    bool tryBeginDraggingBarEdge(int bar_index, const ImVec4& bar_rect, Edge);
    void dragBarEdge();

    bool tryBeginDraggingBarCorner(int bar_index, const ImVec4& bar_rect, Edge left_or_right, Edge bottom_or_top);
    void dragBarCorner();

    bool tryBeginDraggingBar(int bar_index, const ImVec4& bar_rect);
    void dragBar();

    void beginAddNewBarOperation();
    void updateAddNewBarOperation();

    bool commitBarEdits();

    auto barToScreenRect(const Bar&) const -> ImVec4;
    auto cornerRect(const ImVec4& rect, Edge left_or_right, Edge bottom_or_top) const -> ImVec4;
    auto edgeRect(const ImVec4&, Edge) const -> ImVec4;
    auto leftEdgeRect(const ImVec4&) const -> ImVec4;
    auto topEdgeRect(const ImVec4&) const -> ImVec4;
    auto rightEdgeRect(const ImVec4&) const -> ImVec4;
    auto bottomEdgeRect(const ImVec4&) const -> ImVec4;
    auto interiorRect(const ImVec4&) const -> ImVec4;

    bool screenRectContainsPos(const ImVec4& rect, const ImVec2& pos) const;

    Event_handler event_handler = [](Event&) { return false; };
    std::vector<Bar> bars;
    // Bar_set bars{ compare_bar_positions };
    float rounding_unit_x = 1, rounding_unit_y = 1;

    Mouse_interaction mouse_interaction = Mouse_interaction::None;

    struct Dragging_operation {
        int bar_index = -1;
        Edge edge;
        Corner corner;
        ImVec2 starting_position;
        Bar bar;
    };

    Dragging_operation dragging_op;
};
