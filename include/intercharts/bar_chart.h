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

  using Bar_change_applier = std::function<bool(Bar &bar, int bar_index, Bar_zone, float &new_x, float &new_y)>;

  // Lifecycle

  using Chart::Chart;

  // Setup

  auto &setBarChangeApplier(Bar_change_applier vtor)
  {
    bar_change_applier = vtor;
    return *this;
  }

  auto &setRoundingUnitX(float unit)
  {
    rounding_unit_x = unit;
    return *this;
  }
  auto &setRoundingUnitY(float unit)
  {
    rounding_unit_y = unit;
    return *this;
  }

  auto &addRectangle(float x_low, float y_low, float x_high, float y_high)
  {
    bars.emplace_back(x_low, y_low, x_high, y_high);
    return *this;
  }

  void clearRectangles() { bars.clear(); }

  // Display & interaction

protected:
  void renderValues() override;

private:
  void beginDragOperation(int rect, Bar_zone);
  void updateDragOperation();
  void endDragOperation();

  void drawZoneHighlight(const ImVec4 &zone);

  bool tryDragZoneTo(Bar_zone, float &new_x, float &new_y);

  auto roundUnitsX(float) -> float;
  auto roundUnitsY(float) -> float;

  auto getBarInteractionZoneRects(const ImVec4 &) -> std::array<ImVec4, 5>;

  auto barToScreenRect(const Bar &) const -> ImVec4;
  auto leftEdgeRect(const ImVec4 &) const -> ImVec4;
  auto topEdgeRect(const ImVec4 &) const -> ImVec4;
  auto rightEdgeRect(const ImVec4 &) const -> ImVec4;
  auto bottomEdgeRect(const ImVec4 &) const -> ImVec4;
  auto interiorRect(const ImVec4 &) const -> ImVec4;

  bool screenRectContainsPos(const ImVec4 &rect, const ImVec2 &pos) const;

  Bar_change_applier bar_change_applier = [](Bar &, int, Bar_zone, float &, float &) { return false; };
  std::vector<Bar> bars;
  float rounding_unit_x = 1, rounding_unit_y = 1;

  bool dragging = false;
  int dragged_bar_index = -1; // rectangle the user is interacting with
  Bar_zone dragged_zone = Bar_zone::None;
  float x_at_drag_start, y_at_drag_start;
};
