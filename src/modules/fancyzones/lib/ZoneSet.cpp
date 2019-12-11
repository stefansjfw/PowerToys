#include "pch.h"

namespace {
  constexpr int MAX_ZONE_COUNT = 50;
  constexpr int C_MULTIPLIER = 10000;

  struct GridLayoutInfo {
    int rows;
    int columns;
    int rowsPercents[MAX_ZONE_COUNT];
    int columnsPercents[MAX_ZONE_COUNT];
    int cellChildMap[MAX_ZONE_COUNT][MAX_ZONE_COUNT];
  };

  // PriorityGrid layout is unique for zoneCount <= 11. For zoneCount > 11 PriorityGrid is same as Grid
  GridLayoutInfo predefinedPriorityGridLayouts[11] =
  {
    /* 1 */
    {
      .rows = 1,
      .columns = 1,
      .rowsPercents = { 10000 },
      .columnsPercents = { 10000 },
      .cellChildMap = { 0 }
    },
    /* 2 */
    {
      .rows = 1,
      .columns = 2,
      .rowsPercents = { 10000 },
      .columnsPercents = { 6667, 3333 },
      .cellChildMap = { 0, 1 }
    },
    /* 3 */
    {
      .rows = 1,
      .columns = 3,
      .rowsPercents = { 10000 },
      .columnsPercents = { 2500, 5000, 2500 },
      .cellChildMap = { 0, 1, 2 }
    },
    /* 4 */
    {
      .rows = 2,
      .columns = 3,
      .rowsPercents = { 5000, 5000 },
      .columnsPercents = { 2500, 5000, 2500 },
      .cellChildMap = { {0, 1, 2}, {0, 1, 3} }
    },
    /* 5 */
    {
      .rows = 2,
      .columns = 3,
      .rowsPercents = { 5000, 5000 },
      .columnsPercents = { 2500, 5000, 2500 },
      .cellChildMap = { {0, 1, 2}, {3, 1, 4} }
    },
    /* 6 */
    {
      .rows = 3,
      .columns = 3,
      .rowsPercents = { 3333, 3334, 3333 },
      .columnsPercents = { 2500, 5000, 2500 },
      .cellChildMap = { {0, 1, 2}, {0, 1, 3}, {4, 1, 5} }
    },
    /* 7 */
    {
      .rows = 3,
      .columns = 3,
      .rowsPercents = { 3333, 3334, 3333 },
      .columnsPercents = { 2500, 5000, 2500 },
      .cellChildMap = { {0, 1, 2}, {3, 1, 4}, {5, 1, 6} }
    },
    /* 8 */
    {
      .rows = 3,
      .columns = 4,
      .rowsPercents = { 3333, 3334, 3333 },
      .columnsPercents = { 2500, 2500, 2500, 2500 },
      .cellChildMap = { {0, 1, 2, 3}, {4, 1, 2, 5}, {6, 1, 2, 7} }
    },
    /* 9 */
    {
      .rows = 3,
      .columns = 4,
      .rowsPercents = { 3333, 3334, 3333 },
      .columnsPercents = { 2500, 2500, 2500, 2500 },
      .cellChildMap = { {0, 1, 2, 3}, {4, 1, 2, 5}, {6, 1, 7, 8} }
    },
    /* 10 */
    {
      .rows = 3,
      .columns = 4,
      .rowsPercents = { 3333, 3334, 3333 },
      .columnsPercents = { 2500, 2500, 2500, 2500 },
      .cellChildMap = { {0, 1, 2, 3}, {4, 1, 5, 6}, {7, 1, 8, 9} }
    },
    /* 11 */
    {
      .rows = 3,
      .columns = 4,
      .rowsPercents = { 3333, 3334, 3333 },
      .columnsPercents = { 2500, 2500, 2500, 2500 },
      .cellChildMap = { {0, 1, 2, 3}, {4, 1, 5, 6}, {7, 8, 9, 10} }
    }
  };
}

struct ZoneSet : winrt::implements<ZoneSet, IZoneSet>
{
public:
    ZoneSet(ZoneSetConfig const& config) : m_config(config)
    {
    }

    ZoneSet(ZoneSetConfig const& config, std::vector<winrt::com_ptr<IZone>> zones) :
        m_config(config),
        m_zones(zones)
    {
    }

    IFACEMETHODIMP_(GUID) Id() noexcept { return m_config.Id; }
    IFACEMETHODIMP_(WORD) LayoutId() noexcept { return m_config.LayoutId; }
    IFACEMETHODIMP AddZone(winrt::com_ptr<IZone> zone, bool front) noexcept;
    IFACEMETHODIMP RemoveZone(winrt::com_ptr<IZone> zone) noexcept;
    IFACEMETHODIMP_(winrt::com_ptr<IZone>) ZoneFromPoint(POINT pt) noexcept;
    IFACEMETHODIMP_(winrt::com_ptr<IZone>) ZoneFromWindow(HWND window) noexcept;
    IFACEMETHODIMP_(int) GetZoneIndexFromWindow(HWND window) noexcept;
    IFACEMETHODIMP_(std::vector<winrt::com_ptr<IZone>>) GetZones() noexcept { return m_zones; }
    IFACEMETHODIMP_(JSONHelpers::ZoneSetLayoutType) GetLayout() noexcept { return m_config.Layout; }
    IFACEMETHODIMP_(winrt::com_ptr<IZoneSet>) MakeCustomClone() noexcept;
    IFACEMETHODIMP_(void) MoveZoneToFront(winrt::com_ptr<IZone> zone) noexcept;
    IFACEMETHODIMP_(void) MoveZoneToBack(winrt::com_ptr<IZone> zone) noexcept;
    IFACEMETHODIMP_(void) MoveWindowIntoZoneByIndex(HWND window, HWND zoneWindow, int index) noexcept;
    IFACEMETHODIMP_(void) MoveWindowIntoZoneByDirection(HWND window, HWND zoneWindow, DWORD vkCode) noexcept;
    IFACEMETHODIMP_(void) MoveSizeEnd(HWND window, HWND zoneWindow, POINT ptClient) noexcept;
    IFACEMETHODIMP_(void) CalculateZones(MONITORINFO monitorInfo, int zoneCount, int spacing) noexcept;

private:
    void CalculateFocusLayout(Rect workArea) noexcept;
    void CalculateColumnsAndRowsLayout(Rect workArea, JSONHelpers::ZoneSetLayoutType type, int zoneCount, int spacing) noexcept;
    void CalculateGridLayout(Rect workArea, JSONHelpers::ZoneSetLayoutType type, int zoneCount, int spacing) noexcept;
    void CalculateUniquePriorityGridLayout(Rect workArea, int zoneCount, int spacing) noexcept;

    void CalculateGridZones(Rect workArea, GridLayoutInfo gridLayoutInfo, int spacing);

    std::vector<winrt::com_ptr<IZone>> m_zones;
    ZoneSetConfig m_config;
};

IFACEMETHODIMP ZoneSet::AddZone(winrt::com_ptr<IZone> zone, bool front) noexcept
{
    // XXXX: need to reorder ids when inserting...
    if (front)
    {
        m_zones.insert(m_zones.begin(), zone);
    }
    else
    {
        m_zones.emplace_back(zone);
    }

    // Important not to set Id 0 since we store it in the HWND using SetProp.
    // SetProp(0) doesn't really work.
    zone->SetId(m_zones.size());
    return S_OK;
}

IFACEMETHODIMP ZoneSet::RemoveZone(winrt::com_ptr<IZone> zone) noexcept
{
    auto iter = std::find(m_zones.begin(), m_zones.end(), zone);
    if (iter != m_zones.end())
    {
        m_zones.erase(iter);
        return S_OK;
    }
    return E_INVALIDARG;
}

IFACEMETHODIMP_(winrt::com_ptr<IZone>) ZoneSet::ZoneFromPoint(POINT pt) noexcept
{
    winrt::com_ptr<IZone> smallestKnownZone = nullptr;
    // To reduce redundant calculations, we will store the last known zones area.
    int smallestKnownZoneArea = INT32_MAX;
    for (auto iter = m_zones.begin(); iter != m_zones.end(); iter++)
    {
        if (winrt::com_ptr<IZone> zone = iter->try_as<IZone>())
        {
            RECT* newZoneRect = &zone->GetZoneRect();
            if (PtInRect(newZoneRect, pt))
            {
                if (smallestKnownZone == nullptr)
                {
                    smallestKnownZone = zone;

                    RECT* r = &smallestKnownZone->GetZoneRect();
                    smallestKnownZoneArea = (r->right-r->left)*(r->bottom-r->top);
                }
                else
                {
                    int newZoneArea = (newZoneRect->right-newZoneRect->left)*(newZoneRect->bottom-newZoneRect->top);

                    if (newZoneArea<smallestKnownZoneArea)
                    {
                        smallestKnownZone = zone;
                        newZoneArea = smallestKnownZoneArea;
                    }
                }
            }
        }
    }

    return smallestKnownZone;
}

IFACEMETHODIMP_(winrt::com_ptr<IZone>) ZoneSet::ZoneFromWindow(HWND window) noexcept
{
    for (auto iter = m_zones.begin(); iter != m_zones.end(); iter++)
    {
        if (winrt::com_ptr<IZone> zone = iter->try_as<IZone>())
        {
            if (zone->ContainsWindow(window))
            {
                return zone;
            }
        }
    }
    return nullptr;
}

IFACEMETHODIMP_(winrt::com_ptr<IZoneSet>) ZoneSet::MakeCustomClone() noexcept
{
    if (SUCCEEDED_LOG(CoCreateGuid(&m_config.Id)))
    {
        m_config.IsCustom = true;
        return winrt::make_self<ZoneSet>(m_config, m_zones);
    }
    return nullptr;
}

IFACEMETHODIMP_(void) ZoneSet::MoveZoneToFront(winrt::com_ptr<IZone> zone) noexcept
{
    auto iter = std::find(m_zones.begin(), m_zones.end(), zone);
    if (iter != m_zones.end())
    {
        std::rotate(m_zones.begin(), iter, iter + 1);
    }
}

IFACEMETHODIMP_(void) ZoneSet::MoveZoneToBack(winrt::com_ptr<IZone> zone) noexcept
{
    auto iter = std::find(m_zones.begin(), m_zones.end(), zone);
    if (iter != m_zones.end())
    {
        std::rotate(iter, iter + 1, m_zones.end());
    }
}

IFACEMETHODIMP_(int) ZoneSet::GetZoneIndexFromWindow(HWND window) noexcept
{
    int zoneIndex = 0;
    for (auto iter = m_zones.begin(); iter != m_zones.end(); iter++, zoneIndex++)
    {
        if (winrt::com_ptr<IZone> zone = iter->try_as<IZone>())
        {
            if (zone->ContainsWindow(window))
            {
                return zoneIndex;
            }
        }
    }
    return -1;
}

IFACEMETHODIMP_(void) ZoneSet::MoveWindowIntoZoneByIndex(HWND window, HWND windowZone, int index) noexcept
{
    if (index >= static_cast<int>(m_zones.size()))
    {
        index = 0;
    }

    if (index < m_zones.size())
    {
        if (auto zone = m_zones.at(index))
        {
            zone->AddWindowToZone(window, windowZone, false);
        }
    }
}

IFACEMETHODIMP_(void) ZoneSet::MoveWindowIntoZoneByDirection(HWND window, HWND windowZone, DWORD vkCode) noexcept
{
    winrt::com_ptr<IZone> oldZone;
    winrt::com_ptr<IZone> newZone;

    auto iter = std::find(m_zones.begin(), m_zones.end(), ZoneFromWindow(window));
    if (iter == m_zones.end())
    {
        iter = (vkCode == VK_RIGHT) ? m_zones.begin() : m_zones.end() - 1;
    }
    else if (oldZone = iter->as<IZone>())
    {
        if (vkCode == VK_LEFT)
        {
            if (iter == m_zones.begin())
            {
                iter = m_zones.end();
            }
            iter--;
        }
        else if (vkCode == VK_RIGHT)
        {
            iter++;
            if (iter == m_zones.end())
            {
                iter = m_zones.begin();
            }
        }
    }

    if (newZone = iter->as<IZone>())
    {
        if (oldZone)
        {
            oldZone->RemoveWindowFromZone(window, false);
        }
        newZone->AddWindowToZone(window, windowZone, true);
    }
}

IFACEMETHODIMP_(void) ZoneSet::MoveSizeEnd(HWND window, HWND zoneWindow, POINT ptClient) noexcept
{
    if (auto zoneDrop = ZoneFromWindow(window))
    {
        zoneDrop->RemoveWindowFromZone(window, !IsZoomed(window));
    }

    if (auto zone = ZoneFromPoint(ptClient))
    {
        zone->AddWindowToZone(window, zoneWindow, true);
    }
}

IFACEMETHODIMP_(void) ZoneSet::CalculateZones(MONITORINFO monitorInfo, int zoneCount, int spacing) noexcept
{
  Rect const workArea(monitorInfo.rcWork);

  switch (m_config.Layout)
  {
  case JSONHelpers::ZoneSetLayoutType::Focus:
    CalculateFocusLayout(workArea);
    break;
  case JSONHelpers::ZoneSetLayoutType::Columns:
  case JSONHelpers::ZoneSetLayoutType::Rows:
    CalculateColumnsAndRowsLayout(workArea, m_config.Layout, zoneCount, spacing);
    break;
  case JSONHelpers::ZoneSetLayoutType::Grid:
  case JSONHelpers::ZoneSetLayoutType::PriorityGrid:
    CalculateGridLayout(workArea, m_config.Layout, zoneCount, spacing);
  }
}

void ZoneSet::CalculateFocusLayout(Rect workArea) noexcept
{
  int ZoneCount = m_config.ZoneCount;
  LONG left{ static_cast<LONG>(workArea.width() * 0.1) };
  LONG top{ static_cast<LONG>(workArea.height() * 0.1) };
  LONG right{ static_cast<LONG>(workArea.width() * 0.6) };
  LONG bottom{ static_cast<LONG>(workArea.height() * 0.6) };
  RECT focusZoneRect{ left, top, right, bottom };
  int focusRectXIncrement = (ZoneCount <= 1) ? 0 : (int)(workArea.width() * 0.2) / (ZoneCount - 1);
  int focusRectYIncrement = (ZoneCount <= 1) ? 0 : (int)(workArea.height() * 0.2) / (ZoneCount - 1);

  for (int i = 0; i < ZoneCount; i++)
  {
    AddZone(MakeZone(focusZoneRect), false);
    focusZoneRect.left += focusRectXIncrement;
    focusZoneRect.right += focusRectXIncrement;
    focusZoneRect.bottom += focusRectYIncrement;
    focusZoneRect.top += focusRectYIncrement;
  }
}

void ZoneSet::CalculateColumnsAndRowsLayout(Rect workArea, JSONHelpers::ZoneSetLayoutType type, int zoneCount, int spacing) noexcept
{
  int gutter = spacing;
  int c_multiplier = 10000;

  int zonePercent = c_multiplier / zoneCount;

  LONG totalWidth;
  LONG totalHeight;

  LONG cellWidth;
  LONG cellHeight;

  if (type == JSONHelpers::ZoneSetLayoutType::Columns) {
    totalWidth = workArea.width() - (gutter * 2) - (spacing * (zoneCount - 1));
    totalHeight = workArea.height() - (gutter * 2);
    cellWidth = totalWidth * zonePercent / c_multiplier;
    cellHeight = totalHeight;
  }
  else { //Rows
    totalWidth = workArea.width() - (gutter * 2);
    totalHeight = workArea.height() - (gutter * 2) - (spacing * (zoneCount - 1));
    cellWidth = totalWidth;
    cellHeight = totalHeight * zonePercent / c_multiplier;
  }

  LONG top = spacing;
  LONG left = spacing;
  LONG bottom = top + cellHeight;
  LONG right = left + cellWidth;

  for (int zone = 0; zone < zoneCount; zone++)
  {
    RECT focusZoneRect{ left, top, right, bottom };
    AddZone(MakeZone(focusZoneRect), false);

    if (type == JSONHelpers::ZoneSetLayoutType::Columns) {
      left += cellWidth + spacing;
      right = left + cellWidth;
    }
    else { //Rows
      top += cellHeight + spacing;
      bottom = top + cellHeight;
    }
  }
}

void ZoneSet::CalculateGridLayout(Rect workArea, JSONHelpers::ZoneSetLayoutType type, int zoneCount, int spacing) noexcept
{
  if (type == JSONHelpers::ZoneSetLayoutType::PriorityGrid && zoneCount <= 11) {
    CalculateUniquePriorityGridLayout(workArea, zoneCount, spacing);
    return;
  }

  GridLayoutInfo gridLayoutInfo;
  gridLayoutInfo.rows = 1;
  gridLayoutInfo.columns = 1;
  while (zoneCount / gridLayoutInfo.rows >= gridLayoutInfo.rows)
  {
    gridLayoutInfo.rows++;
  }
  gridLayoutInfo.rows--;
  gridLayoutInfo.columns = zoneCount / gridLayoutInfo.rows;
  if (zoneCount % gridLayoutInfo.rows == 0)
  {
    // even grid
  }
  else
  {
    gridLayoutInfo.columns++;
  }

  for (int row = 0; row < gridLayoutInfo.rows; row++) {
    gridLayoutInfo.rowsPercents[row] = C_MULTIPLIER / gridLayoutInfo.rows;
  }
  for (int col = 0; col < gridLayoutInfo.columns; col++) {
    gridLayoutInfo.columnsPercents[col] = C_MULTIPLIER / gridLayoutInfo.columns;
  }
  
  int index = 0;
  for (int col = gridLayoutInfo.columns - 1; col >= 0; col--)
  {
    for (int row = gridLayoutInfo.rows - 1; row >= 0; row--)
    {
      gridLayoutInfo.cellChildMap[row][col] = index++;
      if (index == zoneCount)
      {
        index--;
      }

    }
  }
  CalculateGridZones(workArea, gridLayoutInfo, spacing);
}

void ZoneSet::CalculateUniquePriorityGridLayout(Rect workArea, int zoneCount, int spacing) noexcept
{
  CalculateGridZones(workArea, predefinedPriorityGridLayouts[zoneCount - 1], spacing);
}

void ZoneSet::CalculateGridZones(Rect workArea, GridLayoutInfo gridLayoutInfo, int spacing)
{

  int gutter = spacing;

  LONG totalWidth = static_cast<LONG>(workArea.width()) - (gutter * 2) - (spacing * (gridLayoutInfo.columns - 1));
  LONG totalHeight = static_cast<LONG>(workArea.height()) - (gutter * 2) - (spacing * (gridLayoutInfo.rows - 1));
  struct Info {
    LONG Extent;
    LONG Start;
    LONG End;
  };
  Info rowInfo[MAX_ZONE_COUNT];
  Info columnInfo[MAX_ZONE_COUNT];
  
  LONG top = gutter;
  for (int row = 0; row < gridLayoutInfo.rows; row++)
  {
    rowInfo[row].Start = top;
    rowInfo[row].Extent = totalHeight * gridLayoutInfo.rowsPercents[row] / C_MULTIPLIER;
    rowInfo[row].End = rowInfo[row].Start + rowInfo[row].Extent;
    top += rowInfo[row].Extent + spacing;
  }

  LONG left = gutter;
  for (int col = 0; col < gridLayoutInfo.columns; col++)
  {
    columnInfo[col].Start = left;
    columnInfo[col].Extent = totalWidth * gridLayoutInfo.columnsPercents[col] / C_MULTIPLIER;
    columnInfo[col].End = columnInfo[col].Start + columnInfo[col].Extent;
    left += columnInfo[col].Extent + spacing;
  }

  for (int row = 0; row < gridLayoutInfo.rows; row++)
  {
    for (int col = 0; col < gridLayoutInfo.columns; col++)
    {
      int i = gridLayoutInfo.cellChildMap[row][col];
      if (((row == 0) || (gridLayoutInfo.cellChildMap[row - 1][col] != i)) &&
        ((col == 0) || (gridLayoutInfo.cellChildMap[row][col - 1] != i)))
      {
        left = columnInfo[col].Start;
        top = rowInfo[row].Start;

        int maxRow = row;
        while (((maxRow + 1) < gridLayoutInfo.rows) && (gridLayoutInfo.cellChildMap[maxRow + 1][col] == i))
        {
          maxRow++;
        }
        int maxCol = col;
        while (((maxCol + 1) < gridLayoutInfo.columns) && (gridLayoutInfo.cellChildMap[row][maxCol + 1] == i))
        {
          maxCol++;
        }

        LONG right = columnInfo[maxCol].End;
        LONG bottom = rowInfo[maxRow].End;
        RECT focusZoneRect{ left, top, right, bottom };
        AddZone(MakeZone(focusZoneRect), false);
      }
    }
  }
}

winrt::com_ptr<IZoneSet> MakeZoneSet(ZoneSetConfig const& config) noexcept
{
    return winrt::make_self<ZoneSet>(config);
}
