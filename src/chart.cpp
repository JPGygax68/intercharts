#include <string>
#include <iostream>
#include <iomanip>
#include <array>
#include <cassert>
#include <cmath>
#include <sstream>
#include <imgui.h>
#include <intercharts/chart.h>

Chart::Chart(float x2, float y2, float x1, float y1) : x_min(x1), x_max(x2), y_min(y1), y_max(y2), dpi_scaling_factor(1)
{
}

void Chart::render(float w, float h, float dpi_scaling)
{
    // TODO: make into properties
    const int spacing = 4;

    updateGeometry(w, h, dpi_scaling);

    ImGui::InvisibleButton("Chart", ImVec2{w_scaled, h_scaled});

    drawGrid();

    renderAndInteractWithValues();

    afterRenderingValues();
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

    pixels_per_unit_x = w_grid / (x_max - x_min); // meter/pixel ratio
    pixels_per_unit_y = h_grid / 200.0f;          // TODO: set upper limit by parameter

    x_grid = p_scr.x + margin;
    y_grid = p_scr.y;
    y_xaxis_labels = y_grid + h_grid + ds * spacing;
}

void Chart::drawGrid()
{
    auto dl = ImGui::GetWindowDrawList();
    auto ds = dpi_scaling_factor;

    // Vertical grid lines (on X axis)
    for (float x = x_min; x <= x_max; x += x_grid_minor)
    {

        bool at_medium_gridline = (int)x % x_grid_medium == 0;
        bool at_major_gridline = (int)x % x_grid_major == 0;

        float thickness = ds * 1.0f;
        if (at_medium_gridline)
            thickness = ds * 1.6f;
        if (at_major_gridline)
            thickness = ds * 2.5f;

        float x_line = x_grid + pixels_per_unit_x * x;
        dl->AddLine(ImVec2{x_line, y_grid}, ImVec2{x_line, y_grid + h_grid}, 0xFF888888, thickness);

        if (at_medium_gridline)
            drawLabel(x_line, y_xaxis_labels, formatFloat(x, 0).c_str());
    }

    // TODO: horizontal grid lines (Y axis)

    // TODO: separate method so "box" can be drawn "over" the values?
    dl->AddRect(ImVec2{x_grid, y_grid}, ImVec2{x_grid + w_grid, y_grid + h_grid}, 0xFF888888);
}

void Chart::drawLabel(float x, float y, const char *label)
{
    auto ds = dpi_scaling_factor;

    auto dl = ImGui::GetWindowDrawList();
    auto size = ImGui::GetFont()->CalcTextSizeA(ds * label_font_size, FLT_MAX, FLT_MAX, label);

    dl->AddText(nullptr, ds * label_font_size, ImVec2{x - size.x / 2, y}, 0xFFFFFFFF, label);
}

auto Chart::plotToScreenUnitsX(float x) const -> float
{
    return plottingArea().x + x * pixelsPerUnitHorz(); // TODO: support origin != (0, 0)
}

auto Chart::plotToScreenUnitsY(float y) const -> float
{
    return plottingArea().w - y * pixelsPerUnitVert(); // TODO: support origin != (0, 0)
}

auto Chart::screenToPlotUnitsX(float x) const -> float
{
    return (x - plottingArea().x) / pixelsPerUnitHorz();
}

auto Chart::screenToPlotUnitsY(float y) const -> float
{
    return (plottingArea().w - y) / pixelsPerUnitVert();
}

