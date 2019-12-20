#pragma once

#include <array>
#include "chart.h"

class Bar_chart : public Chart
{
public:

    struct Bar
    {
        float x1, y1; // bottom left
        float x2, y2; // top right
        Bar() = default;
        Bar(float a, float b, float c, float d) : x1(a), y1(b), x2(c), y2(d) {}
    };

    enum Bar_zone
    {
        None = 0,
        Left_edge,
        Top_edge,
        Right_edge,
        Bottom_edge,
        Interior
    };

    using Bar_change_applier = std::function<bool(Bar & bar, int bar_index, Bar_zone, float& new_x, float& new_y)>;

    // Lifecycle

    using Chart::Chart;

    // Setup

    auto& setBarChangeApplier(Bar_change_applier vtor)
    {
        bar_change_applier = vtor;
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

    // auto screenPosToXValue(float x) const { return x / pixelsPerUnitHorz(); } // TODO: take range into account
    // auto screenPosToYValue(float y) const { return y / pixelsPerUnitVert(); } // TODO: take range into account

    auto roundXValue(float) -> float;
    auto roundYValue(float) -> float;

private:

    enum class Mouse_interaction { 
        None = 0, 
        Dragging_bar_zone,          // dragging either the bar itself ("interior" zone), or one of the edges (see Bar_zone)
        Adding_new_bar,             // click-and-drag to define a new Bar
    };

    using Interaction_zone_list = std::array<ImVec4, 5>;

    bool tryStartDragOperationOnExistingBar(int i_rect, const Interaction_zone_list& zones);
    void highlightHoveredInteractionZone(const Interaction_zone_list& zones);

    void beginDragOperation(int rect, Bar_zone);
    void updateDragOperation();
    void endDragOperation();

    void beginAddNewBarOperation();
    void updateAddNewBarOperation();
    void endAddNewBarOperation();

    void drawZoneHighlight(const ImVec4& zone);

    bool tryDragZoneTo(Bar_zone, float& new_x, float& new_y);

    auto getBarInteractionZoneRects(const ImVec4&) -> std::array<ImVec4, 5>;

    auto barToScreenRect(const Bar&) const -> ImVec4;
    auto leftEdgeRect(const ImVec4&) const -> ImVec4;
    auto topEdgeRect(const ImVec4&) const -> ImVec4;
    auto rightEdgeRect(const ImVec4&) const -> ImVec4;
    auto bottomEdgeRect(const ImVec4&) const -> ImVec4;
    auto interiorRect(const ImVec4&) const -> ImVec4;

    bool screenRectContainsPos(const ImVec4& rect, const ImVec2& pos) const;

    Bar_change_applier bar_change_applier = [](Bar&, int, Bar_zone, float&, float&) { return false; };
    std::vector<Bar> bars;
    float rounding_unit_x = 1, rounding_unit_y = 1;

    Mouse_interaction mouse_interaction = Mouse_interaction::None;

    // Dragging edges of a Bar
    int dragged_bar_index = -1; // Bar the user is interacting with
    Bar_zone dragged_zone = Bar_zone::None;
    float x_at_drag_start, y_at_drag_start;

    // Adding a new bar via click-and-drag
    Bar new_bar;
};
