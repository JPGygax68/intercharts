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

    enum class Corner { None = 0, Top_left, Top_right, Bottom_right, Bottom_left };

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

    enum Bar_hotspot
    {
        None = 0,
        Left_edge,
        Top_edge,
        Right_edge,
        Bottom_edge,
        Interior,
        Top_left_corner,
        Top_right_corner,
        Bottom_right_corner,
        Bottom_left_corner
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
    void afterRenderingValues() override;

    void drawBar(const ImVec4& rect, bool dragging);

    auto roundXValue(float) -> float;
    auto roundYValue(float) -> float;

private:

    // static bool compare_bar_positions(const Bar& a, const Bar& b) { return a.x1 < b.x1; }
    //using Bar_set = std::set<Bar, decltype(&compare_bar_positions)>;

    static bool hotspotBelongsToEdge(Bar_hotspot spot, Edge edge);

    enum class Mouse_interaction { 
        None = 0, 
        Dragging_bar_corner,
        Dragging_bar_edge,
        Dragging_bar,
        Defining_new_bar,             // click-and-drag to define a new Bar
    };

    using Hotspot_zone_list = std::array<ImVec4, 5>;

    bool tryStartDragOperationOnExistingBar(int i_rect);
    void highlightHoveredHotspot(const Hotspot_zone_list& zones);

    void beginDraggingBarEdge(int bar_index, Edge edge);
    void dragBarEdge();
    void endDraggingBarEdge();

    void beginDraggingBarCorner(int bar_index, Corner);
    void dragBarCorner();
    void endDraggingBarCorner();

    void beginDraggingBar(int bar_index);
    void dragBar();
    void endDraggingBar();

    void beginAddNewBarOperation();
    void updateAddNewBarOperation();
    void endAddNewBarOperation();

    void drawZoneHighlight(const ImVec4& zone);

    bool tryDragEdgeTo(Edge handle, float new_x, float new_y);

    auto getBarInteractionZoneRects(const ImVec4&) -> std::array<ImVec4, 5>;

    auto barToScreenRect(const Bar&) const -> ImVec4;
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

    // Dragging edges of a Bar
    int dragged_bar_index = -1; // Bar the user is interacting with
    // Bar_hotspot dragged_handle = Bar_hotspot::None;
    Edge dragged_edge;
    Corner dragged_corner;
    float x_at_drag_start, y_at_drag_start;

    // Adding a new bar via click-and-drag
    Bar new_bar;
};
