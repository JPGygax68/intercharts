#pragma once

#include <vector>
#include <functional>


// TODO: wrap everything into a namespace

class Chart;
class Value_plotter;

class Chart {
public:
    // Lifecycle
    Chart(float x_max, float y_max, float x_min = 0, float y_min = 0);

    // Configuration
    void setLabelFontSize(float size) { label_font_size = size; }

    // Data binding
    void setValuePlotter(Value_plotter* plotter) { value_plotter = plotter; }

    // Rendering 
    void render(float w, float h, float dpi_scaling = 1.0f);

    // Queries (interface with Value Plotters)

    auto plottingArea() const { return ImVec4(x_grid, y_grid, x_grid+ w_grid, y_grid + h_grid); }
    auto dpiScalingFactor() const { return dpi_scaling_factor; }
    auto pixelsPerUnitHorz() const { return pixels_per_unit_x; }
    auto pixelsPerUnitVert() const { return pixels_per_unit_y; }
    auto labelFontSize() const { return dpi_scaling_factor * label_font_size; }

private:

    void updateGeometry(float w, float h, float dpi_scaling);
    void drawLabel(float x, float y, const char* label);
    void drawGrid();

    // Scale definition
    float x_min, y_min, x_max, y_max;
    int x_grid_minor = 100;
    int x_grid_medium = 500;
    int x_grid_major = 1'000;
    float label_font_size = 14.0f;

    // Value plotter
    Value_plotter *value_plotter = nullptr;

    // Per-render 
    float dpi_scaling_factor;   // provided as render parameter
    float margin;               // left and right, to leave space for horizontal labels
    float pixels_per_unit_x;
    float pixels_per_unit_y;
    float w_scaled, h_scaled;   // full width and height in pixel units (DPI-scaled)
    float x_grid, y_grid;       // left and top of grid in screen coordinates
    float w_grid, h_grid;       // in pixels
    float y_xaxis_labels;
};


class Value_plotter {
public:
    Value_plotter(Chart& chart_) : _chart{ chart_ } {}

    virtual void render() = 0;

protected:
    auto& chart() const { return _chart; }

    auto plotToScreenUnitsX(float x) const -> float;
    auto plotToScreenUnitsY(float y) const -> float;

private:
    Chart& _chart;
};


class Bar_plotter: public Value_plotter {
public:
    struct Bar {
        float x1, y1;   // bottom left
        float x2, y2;   // top right
        Bar(float a, float b, float c, float d) : x1(a), y1(b), x2(c), y2(d) {}
    };

    enum Bar_zone { None = 0, Left_edge, Top_edge, Right_edge, Bottom_edge, Interior }; 

    using Bar_change_applier = std::function<bool(Bar& bar, int bar_index, Bar_zone, float& new_x, float& new_y)>;

    // Lifecycle 

    using Value_plotter::Value_plotter;

    // Setup 

    auto& setBarChangeApplier(Bar_change_applier vtor) { bar_change_applier = vtor; return *this; }

    auto& setRoundingUnitX(float unit) { rounding_unit_x = unit; return *this; }
    auto& setRoundingUnitY(float unit) { rounding_unit_y = unit; return *this; }

    auto& addRectangle(float x_low, float y_low, float x_high, float y_high) {
        bars.emplace_back(x_low, y_low, x_high, y_high);
        return *this;
    }

    void clearRectangles() { bars.clear(); }

    // Display & interaction

    void render() override;

private:

    void beginDragOperation(int rect, Bar_zone);
    void updateDragOperation();
    void endDragOperation();

    void drawZoneHighlight(const ImVec4& zone);

    bool tryDragZoneTo(Bar_zone, float& new_x, float& new_y);

    auto roundUnitsX(float) -> float;
    auto roundUnitsY(float) -> float;

    auto getBarInteractionZoneRects(const ImVec4&) -> std::array<ImVec4, 5>;

    auto barToScreenRect(const Bar&) const -> ImVec4;
    auto leftEdgeRect  (const ImVec4&) const -> ImVec4;
    auto topEdgeRect   (const ImVec4&) const -> ImVec4;
    auto rightEdgeRect (const ImVec4&) const -> ImVec4;
    auto bottomEdgeRect(const ImVec4&) const -> ImVec4;
    auto interiorRect  (const ImVec4&) const -> ImVec4;

    bool screenRectContainsPos(const ImVec4& rect, const ImVec2& pos) const;

    Bar_change_applier bar_change_applier = [](Bar&, int, Bar_zone, float&, float&) { return false; };
    std::vector<Bar> bars;
    float rounding_unit_x = 1, rounding_unit_y = 1;

    bool dragging = false;
    int dragged_bar_index = -1;   // rectangle the user is interacting with
    Bar_zone dragged_zone = Bar_zone::None;
    float x_at_drag_start, y_at_drag_start;
};
