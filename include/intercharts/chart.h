#pragma once

#include <vector>
#include <functional>
#include <imgui.h>

// TODO: wrap everything into a namespace

class Chart
{
public:
    // Lifecycle
    Chart(float x_max, float y_max, float x_min = 0, float y_min = 0);

    // Configuration
    void setLabelFontSize(float size) { label_font_size = size; }

    // Rendering
    void render(float w, float h, float dpi_scaling = 1.0f);

    // Queries (interface with Value Plotters)

    auto plottingArea() const { return ImVec4(x_grid, y_grid, x_grid + w_grid, y_grid + h_grid); }
    auto dpiScalingFactor() const { return dpi_scaling_factor; }
    auto pixelsPerUnitHorz() const { return pixels_per_unit_x; }
    auto pixelsPerUnitVert() const { return pixels_per_unit_y; }
    auto labelFontSize() const { return dpi_scaling_factor * label_font_size; }

protected:
    virtual void renderValues() = 0;

    auto plotToScreenUnitsX(float x) const -> float;
    auto plotToScreenUnitsY(float y) const -> float;

    static auto formatFloat(float val, int prec = 0) -> std::string
    {
        char buffer[20];
        snprintf(buffer, sizeof(buffer), "%.*f", prec, val);
        return buffer;
    }

private:
    void updateGeometry(float w, float h, float dpi_scaling);
    void drawLabel(float x, float y, const char *label);
    void drawGrid();

    // Scale definition
    float x_min, y_min, x_max, y_max;
    int x_grid_minor = 100;
    int x_grid_medium = 500;
    int x_grid_major = 1'000;
    float label_font_size = 14.0f;

    // Per-render
    float dpi_scaling_factor; // provided as render parameter
    float margin;             // left and right, to leave space for horizontal labels
    float pixels_per_unit_x;
    float pixels_per_unit_y;
    float w_scaled, h_scaled; // full width and height in pixel units (DPI-scaled)
    float x_grid, y_grid;     // left and top of grid in screen coordinates
    float w_grid, h_grid;     // in pixels
    float y_xaxis_labels;
};
