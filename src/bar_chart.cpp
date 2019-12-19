#include <imgui.h>
#include <intercharts/bar_chart.h>

void Bar_chart::renderValues()
{
  auto wdl = ImGui::GetWindowDrawList();
  auto fdl = ImGui::GetForegroundDrawList();
  auto &io = ImGui::GetIO();

  int i_rect = 0;
  for (const auto &plot_rect : bars)
  {

    const auto rect = barToScreenRect(plot_rect);
    const auto interaction_rects = getBarInteractionZoneRects(rect);

    if (ImGui::IsItemActive())
    {
      const auto click_pos = io.MouseClickedPos[0];
      if (!dragging)
      {
        for (auto i = 0U; i < interaction_rects.size(); i++)
        {
          // TODO: support disabling each zone
          const auto &rect = interaction_rects[i];
          if (screenRectContainsPos(rect, click_pos))
            beginDragOperation(i_rect, (Bar_zone)(Bar_zone::Left_edge + i));
        }
      }
      if (dragging)
      {
        updateDragOperation();
        fdl->AddLine(click_pos, io.MousePos, ImGui::GetColorU32(ImGuiCol_Button), 4.0f);
      }
    }
    else
    {
      if (dragging)
        endDragOperation();
      const auto mouse_pos = io.MousePos;
      for (auto i = 0U; i < interaction_rects.size(); i++)
      {
        // TODO: support disabling each zone
        const auto &rect = interaction_rects[i];
        if (screenRectContainsPos(rect, mouse_pos))
          drawZoneHighlight(rect);
      }
    }

    // TODO: visually support dragging operations
    wdl->AddRectFilled({rect.x, rect.y}, {rect.z, rect.w}, i_rect == dragged_bar_index ? 0x800000FF : 0xFF0000FF);
    wdl->AddRect({rect.x, rect.y}, {rect.z, rect.w}, 0xFFFFFFFF);

    ++i_rect;
  }
}

void Bar_chart::beginDragOperation(int bar_index, Bar_zone zone)
{
    const auto &bar = bars[bar_index];
    switch (zone)
    {
    case Bar_zone::Left_edge:
        x_at_drag_start = bar.x1;
        break;
    case Bar_zone::Top_edge:
        y_at_drag_start = bar.y2;
        break;
    case Bar_zone::Right_edge:
        x_at_drag_start = bar.x2;
        break;
    case Bar_zone::Bottom_edge:
        y_at_drag_start = bar.y1;
        break;
    case Bar_zone::Interior:
        x_at_drag_start = bar.x1;
        y_at_drag_start = bar.y1;
        break;
    default:
        assert(false);
    }

    dragging = true;
    dragged_bar_index = bar_index;
    dragged_zone = zone;
}

void Bar_chart::updateDragOperation()
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

        float delta_x = (mouse_pos.x - click_pos.x) / pixelsPerUnitHorz();
        float delta_y = (mouse_pos.y - click_pos.y) / pixelsPerUnitVert();

        float new_x = roundUnitsX(x_at_drag_start + delta_x);
        float new_y = roundUnitsY(y_at_drag_start - delta_y);

        tryDragZoneTo(dragged_zone, new_x, new_y);

        // TODO: temporary, mostly for debugging -> replace
        fdl->AddText(nullptr, labelFontSize(), {mouse_pos.x + ds * 3, mouse_pos.y - ds * 2 - labelFontSize()}, 0xFFFFFFFF,
            (formatFloat(new_x) + "," + formatFloat(new_y)).c_str());
    }
}

void Bar_chart::endDragOperation()
{
    // TODO: commit changed value

    dragging = false;
    dragged_bar_index = -1;
    dragged_zone = Bar_zone::None;
}

void Bar_chart::drawZoneHighlight(const ImVec4 &zone)
{
    auto fdl = ImGui::GetForegroundDrawList();

    fdl->AddRectFilled({zone.x, zone.y}, {zone.z, zone.w}, 0x8080FFFF); // TODO: color constant or setting
}

bool Bar_chart::tryDragZoneTo(Bar_zone zone, float &new_x, float &new_y)
{
    auto &bar = bars[dragged_bar_index];

    // TODO: callback-based item-to-bar conversion
    if (bar_change_applier(bar, dragged_bar_index, zone, new_x, new_y))
    {
        switch (zone)
        {
        case Left_edge:
            bar.x1 = new_x;
            break;
        case Right_edge:
            bar.x2 = new_x;
            break;
        case Top_edge:
            bar.y2 = new_y;
            break;
        case Bottom_edge:
            bar.y1 = new_y;
            break;
        case Interior:
            bar.x2 += new_x - bar.x1;
            bar.x1 = new_x;
            bar.y2 += new_y - bar.y1;
            bar.y1 = new_y;
            break;
        default:
            assert(false);
        }
    }

    return false;
}

auto Bar_chart::roundUnitsX(float x) -> float
{
    return rounding_unit_x * round(x / rounding_unit_x);
}

auto Bar_chart::roundUnitsY(float y) -> float
{
    return rounding_unit_y * round(y / rounding_unit_y);
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

auto Bar_chart::barToScreenRect(const Bar &rect) const -> ImVec4
{
    const auto &pa = plottingArea();
    auto ppux = pixelsPerUnitHorz();
    auto ppuy = pixelsPerUnitVert();

    return {
        pa.x + rect.x1 * ppux, // x = left
        pa.w - rect.y2 * ppuy, // y = top
        pa.x + rect.x2 * ppux, // z = right
        pa.w - rect.y1 * ppuy, // w = bottom
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
