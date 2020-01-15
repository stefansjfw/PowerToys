#include "pch.h"

#include "util.h"
#include "lib/ZoneSet.h"
#include "lib/RegistryHelpers.h"

#include <common/dpi_aware.h>

namespace
{
    constexpr int C_MULTIPLIER = 10000;

    /*
      struct GridLayoutInfo {
        int rows;
        int columns;
        int rowsPercents[MAX_ZONE_COUNT];
        int columnsPercents[MAX_ZONE_COUNT];
        int cellChildMap[MAX_ZONE_COUNT][MAX_ZONE_COUNT];
      };
    */

    auto l = JSONHelpers::GridLayoutInfo(JSONHelpers::GridLayoutInfo::Minimal{ .rows = 1, .columns = 1 });
    // PriorityGrid layout is unique for zoneCount <= 11. For zoneCount > 11 PriorityGrid is same as Grid
    JSONHelpers::GridLayoutInfo predefinedPriorityGridLayouts[11] = {
        /* 1 */
        JSONHelpers::GridLayoutInfo(JSONHelpers::GridLayoutInfo::Full{
            .rows = 1,
            .columns = 1,
            .rowsPercents = { 10000 },
            .columnsPercents = { 10000 },
            .cellChildMap = { { 0 } } }),
        /* 2 */
        JSONHelpers::GridLayoutInfo(JSONHelpers::GridLayoutInfo::Full{
            .rows = 1,
            .columns = 2,
            .rowsPercents = { 10000 },
            .columnsPercents = { 6667, 3333 },
            .cellChildMap = { { 0, 1 } } }),
        /* 3 */
        JSONHelpers::GridLayoutInfo(JSONHelpers::GridLayoutInfo::Full{
            .rows = 1,
            .columns = 3,
            .rowsPercents = { 10000 },
            .columnsPercents = { 2500, 5000, 2500 },
            .cellChildMap = { { 0, 1, 2 } } }),
        /* 4 */
        JSONHelpers::GridLayoutInfo(JSONHelpers::GridLayoutInfo::Full{
            .rows = 2,
            .columns = 3,
            .rowsPercents = { 5000, 5000 },
            .columnsPercents = { 2500, 5000, 2500 },
            .cellChildMap = { { 0, 1, 2 }, { 0, 1, 3 } } }),
        /* 5 */
        JSONHelpers::GridLayoutInfo(JSONHelpers::GridLayoutInfo::Full{
            .rows = 2,
            .columns = 3,
            .rowsPercents = { 5000, 5000 },
            .columnsPercents = { 2500, 5000, 2500 },
            .cellChildMap = { { 0, 1, 2 }, { 3, 1, 4 } } }),
        /* 6 */
        JSONHelpers::GridLayoutInfo(JSONHelpers::GridLayoutInfo::Full{
            .rows = 3,
            .columns = 3,
            .rowsPercents = { 3333, 3334, 3333 },
            .columnsPercents = { 2500, 5000, 2500 },
            .cellChildMap = { { 0, 1, 2 }, { 0, 1, 3 }, { 4, 1, 5 } } }),
        /* 7 */
        JSONHelpers::GridLayoutInfo(JSONHelpers::GridLayoutInfo::Full{
            .rows = 3,
            .columns = 3,
            .rowsPercents = { 3333, 3334, 3333 },
            .columnsPercents = { 2500, 5000, 2500 },
            .cellChildMap = { { 0, 1, 2 }, { 3, 1, 4 }, { 5, 1, 6 } } }),
        /* 8 */
        JSONHelpers::GridLayoutInfo(JSONHelpers::GridLayoutInfo::Full{
            .rows = 3,
            .columns = 4,
            .rowsPercents = { 3333, 3334, 3333 },
            .columnsPercents = { 2500, 2500, 2500, 2500 },
            .cellChildMap = { { 0, 1, 2, 3 }, { 4, 1, 2, 5 }, { 6, 1, 2, 7 } } }),
        /* 9 */
        JSONHelpers::GridLayoutInfo(JSONHelpers::GridLayoutInfo::Full{
            .rows = 3,
            .columns = 4,
            .rowsPercents = { 3333, 3334, 3333 },
            .columnsPercents = { 2500, 2500, 2500, 2500 },
            .cellChildMap = { { 0, 1, 2, 3 }, { 4, 1, 2, 5 }, { 6, 1, 7, 8 } } }),
        /* 10 */
        JSONHelpers::GridLayoutInfo(JSONHelpers::GridLayoutInfo::Full{
            .rows = 3,
            .columns = 4,
            .rowsPercents = { 3333, 3334, 3333 },
            .columnsPercents = { 2500, 2500, 2500, 2500 },
            .cellChildMap = { { 0, 1, 2, 3 }, { 4, 1, 5, 6 }, { 7, 1, 8, 9 } } }),
        /* 11 */
        JSONHelpers::GridLayoutInfo(JSONHelpers::GridLayoutInfo::Full{
            .rows = 3,
            .columns = 4,
            .rowsPercents = { 3333, 3334, 3333 },
            .columnsPercents = { 2500, 2500, 2500, 2500 },
            .cellChildMap = { { 0, 1, 2, 3 }, { 4, 1, 5, 6 }, { 7, 8, 9, 10 } } }),
    };
}

struct ZoneSet : winrt::implements<ZoneSet, IZoneSet>
{
public:
    ZoneSet(ZoneSetConfig const& config) :
        m_config(config)
    {
    }

    ZoneSet(ZoneSetConfig const& config, std::vector<winrt::com_ptr<IZone>> zones) :
        m_config(config),
        m_zones(zones)
    {
    }

    IFACEMETHODIMP_(GUID)
    Id() noexcept { return m_config.Id; }
    IFACEMETHODIMP_(WORD)
    LayoutId() noexcept { return m_config.LayoutId; }
    IFACEMETHODIMP AddZone(winrt::com_ptr<IZone> zone) noexcept;
    IFACEMETHODIMP_(winrt::com_ptr<IZone>)
    ZoneFromPoint(POINT pt) noexcept;
    IFACEMETHODIMP_(int)
    GetZoneIndexFromWindow(HWND window) noexcept;
    IFACEMETHODIMP_(std::vector<winrt::com_ptr<IZone>>)
    GetZones() noexcept { return m_zones; }
    IFACEMETHODIMP_(void)
    MoveWindowIntoZoneByIndex(HWND window, HWND zoneWindow, int index) noexcept;
    IFACEMETHODIMP_(void)
    MoveWindowIntoZoneByDirection(HWND window, HWND zoneWindow, DWORD vkCode) noexcept;
    IFACEMETHODIMP_(void)
    MoveSizeEnd(HWND window, HWND zoneWindow, POINT ptClient) noexcept;
    IFACEMETHODIMP_(void)
    CalculateZones(MONITORINFO monitorInfo, JSONHelpers::ZoneSetLayoutType type, int zoneCount, int spacing, const std::wstring& customZoneSetFilePath) noexcept;

private:
    void CalculateFocusLayout(Rect workArea, int zoneCount) noexcept;
    void CalculateColumnsAndRowsLayout(Rect workArea, JSONHelpers::ZoneSetLayoutType type, int zoneCount, int spacing) noexcept;
    void CalculateGridLayout(Rect workArea, JSONHelpers::ZoneSetLayoutType type, int zoneCount, int spacing) noexcept;
    void CalculateUniquePriorityGridLayout(Rect workArea, int zoneCount, int spacing) noexcept;
    void CalculateCustomLayout(Rect workArea, const std::wstring& customZoneSetFilePath, int spacing) noexcept;

    void CalculateGridZones(Rect workArea, JSONHelpers::GridLayoutInfo gridLayoutInfo, int spacing);

    winrt::com_ptr<IZone> ZoneFromWindow(HWND window) noexcept;

    std::vector<winrt::com_ptr<IZone>> m_zones;
    ZoneSetConfig m_config;
};

IFACEMETHODIMP ZoneSet::AddZone(winrt::com_ptr<IZone> zone) noexcept
{
    m_zones.emplace_back(zone);

    // Important not to set Id 0 since we store it in the HWND using SetProp.
    // SetProp(0) doesn't really work.
    zone->SetId(m_zones.size());
    return S_OK;
}

IFACEMETHODIMP_(winrt::com_ptr<IZone>)
ZoneSet::ZoneFromPoint(POINT pt) noexcept
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
                    smallestKnownZoneArea = (r->right - r->left) * (r->bottom - r->top);
                }
                else
                {
                    int newZoneArea = (newZoneRect->right - newZoneRect->left) * (newZoneRect->bottom - newZoneRect->top);

                    if (newZoneArea < smallestKnownZoneArea)
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

IFACEMETHODIMP_(int)
ZoneSet::GetZoneIndexFromWindow(HWND window) noexcept
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

IFACEMETHODIMP_(void)
ZoneSet::MoveWindowIntoZoneByIndex(HWND window, HWND windowZone, int index) noexcept
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

IFACEMETHODIMP_(void)
ZoneSet::MoveWindowIntoZoneByDirection(HWND window, HWND windowZone, DWORD vkCode) noexcept
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

IFACEMETHODIMP_(void)
ZoneSet::MoveSizeEnd(HWND window, HWND zoneWindow, POINT ptClient) noexcept
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

IFACEMETHODIMP_(void)
ZoneSet::CalculateZones(MONITORINFO monitorInfo, JSONHelpers::ZoneSetLayoutType layoutType, int zoneCount, int spacing, const std::wstring& customZoneSetFilePath) noexcept
{
    Rect const workArea(monitorInfo.rcWork);

    switch (layoutType)
    {
    case JSONHelpers::ZoneSetLayoutType::Focus:
        CalculateFocusLayout(workArea, zoneCount);
        break;
    case JSONHelpers::ZoneSetLayoutType::Columns:
    case JSONHelpers::ZoneSetLayoutType::Rows:
        CalculateColumnsAndRowsLayout(workArea, layoutType, zoneCount, spacing);
        break;
    case JSONHelpers::ZoneSetLayoutType::Grid:
    case JSONHelpers::ZoneSetLayoutType::PriorityGrid:
        CalculateGridLayout(workArea, layoutType, zoneCount, spacing);
        break;
    case JSONHelpers::ZoneSetLayoutType::Custom:
        CalculateCustomLayout(workArea, customZoneSetFilePath, spacing);
        break;
    }
}

void ZoneSet::CalculateFocusLayout(Rect workArea, int zoneCount) noexcept
{
    LONG left{ static_cast<LONG>(workArea.width() * 0.1) };
    LONG top{ static_cast<LONG>(workArea.height() * 0.1) };
    LONG right{ static_cast<LONG>(workArea.width() * 0.6) };
    LONG bottom{ static_cast<LONG>(workArea.height() * 0.6) };
    RECT focusZoneRect{ left, top, right, bottom };
    int focusRectXIncrement = (zoneCount <= 1) ? 0 : (int)(workArea.width() * 0.2) / (zoneCount - 1);
    int focusRectYIncrement = (zoneCount <= 1) ? 0 : (int)(workArea.height() * 0.2) / (zoneCount - 1);

    for (int i = 0; i < zoneCount; i++)
    {
        AddZone(MakeZone(focusZoneRect));
        focusZoneRect.left += focusRectXIncrement;
        focusZoneRect.right += focusRectXIncrement;
        focusZoneRect.bottom += focusRectYIncrement;
        focusZoneRect.top += focusRectYIncrement;
    }
}

void ZoneSet::CalculateColumnsAndRowsLayout(Rect workArea, JSONHelpers::ZoneSetLayoutType type, int zoneCount, int spacing) noexcept
{
    int gutter = spacing;

    int zonePercent = C_MULTIPLIER / zoneCount;

    LONG totalWidth;
    LONG totalHeight;

    LONG cellWidth;
    LONG cellHeight;

    if (type == JSONHelpers::ZoneSetLayoutType::Columns)
    {
        totalWidth = workArea.width() - (gutter * 2) - (spacing * (zoneCount - 1));
        totalHeight = workArea.height() - (gutter * 2);
        cellWidth = totalWidth * zonePercent / C_MULTIPLIER;
        cellHeight = totalHeight;
    }
    else
    { //Rows
        totalWidth = workArea.width() - (gutter * 2);
        totalHeight = workArea.height() - (gutter * 2) - (spacing * (zoneCount - 1));
        cellWidth = totalWidth;
        cellHeight = totalHeight * zonePercent / C_MULTIPLIER;
    }

    LONG top = spacing;
    LONG left = spacing;
    LONG bottom = top + cellHeight;
    LONG right = left + cellWidth;

    for (int zone = 0; zone < zoneCount; zone++)
    {
        RECT focusZoneRect{ left, top, right, bottom };
        AddZone(MakeZone(focusZoneRect));

        if (type == JSONHelpers::ZoneSetLayoutType::Columns)
        {
            left += cellWidth + spacing;
            right = left + cellWidth;
        }
        else
        { //Rows
            top += cellHeight + spacing;
            bottom = top + cellHeight;
        }
    }
}

void ZoneSet::CalculateGridLayout(Rect workArea, JSONHelpers::ZoneSetLayoutType type, int zoneCount, int spacing) noexcept
{
    if (type == JSONHelpers::ZoneSetLayoutType::PriorityGrid && zoneCount <= 11)
    {
        CalculateUniquePriorityGridLayout(workArea, zoneCount, spacing);
        return;
    }

    int rows = 1, columns = 1;
    while (zoneCount / rows >= rows)
    {
        rows++;
    }
    rows--;
    columns = zoneCount / rows;
    if (zoneCount % rows == 0)
    {
        // even grid
    }
    else
    {
        columns++;
    }

    JSONHelpers::GridLayoutInfo gridLayoutInfo(JSONHelpers::GridLayoutInfo::Minimal{ .rows = rows, .columns = columns });

    for (int row = 0; row < rows; row++)
    {
        gridLayoutInfo.rowsPercents()[row] = C_MULTIPLIER / rows;
    }
    for (int col = 0; col < columns; col++)
    {
        gridLayoutInfo.columnsPercents()[col] = C_MULTIPLIER / columns;
    }

    for (int i = 0; i < rows; ++i)
    {
        gridLayoutInfo.cellChildMap()[i] = std::vector<int>(columns);
    }

    int index = 0;
    for (int col = columns - 1; col >= 0; col--)
    {
        for (int row = rows - 1; row >= 0; row--)
        {
            gridLayoutInfo.cellChildMap()[row][col] = index++;
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

void ZoneSet::CalculateCustomLayout(Rect workArea, const std::wstring& customZoneSetFilePath, int spacing) noexcept
{
    wil::unique_cotaskmem_string guuidStr;
    if (SUCCEEDED_LOG(StringFromCLSID(m_config.Id, &guuidStr)))
    {
        JSONHelpers::FancyZonesDataInstance().GetCustomZoneSetFromTmpFile(customZoneSetFilePath, guuidStr.get());
        const auto& zoneSet = JSONHelpers::FancyZonesDataInstance().GetCustomZoneSetsMap()[guuidStr.get()];

        if (zoneSet.type == JSONHelpers::CustomLayoutType::Canvas)
        {
            const auto& zoneSetInfo = std::get<JSONHelpers::CanvasLayoutInfo>(zoneSet.info);
            for (const auto& zone : zoneSetInfo.zones)
            {
                int x = zone.x;
                int y = zone.y;
                int width = zone.width;
                int height = zone.height;
                DPIAware::Convert(NULL, x, y);
                DPIAware::Convert(NULL, width, height);

                RECT focusZoneRect{ x, y, x + width, y + height };
                AddZone(MakeZone(focusZoneRect));
            }
        }
        else if (zoneSet.type == JSONHelpers::CustomLayoutType::Grid)
        {
            CalculateGridZones(workArea, std::get<JSONHelpers::GridLayoutInfo>(zoneSet.info), spacing);
        }
    }
}

void ZoneSet::CalculateGridZones(Rect workArea, JSONHelpers::GridLayoutInfo gridLayoutInfo, int spacing)
{
    int gutter = spacing;

    LONG totalWidth = static_cast<LONG>(workArea.width()) - (gutter * 2) - (spacing * (gridLayoutInfo.columns() - 1));
    LONG totalHeight = static_cast<LONG>(workArea.height()) - (gutter * 2) - (spacing * (gridLayoutInfo.rows() - 1));
    struct Info
    {
        LONG Extent;
        LONG Start;
        LONG End;
    };
    Info rowInfo[JSONHelpers::MAX_ZONE_COUNT];
    Info columnInfo[JSONHelpers::MAX_ZONE_COUNT];

    LONG top = gutter;
    for (int row = 0; row < gridLayoutInfo.rows(); row++)
    {
        rowInfo[row].Start = top;
        rowInfo[row].Extent = totalHeight * gridLayoutInfo.rowsPercents()[row] / C_MULTIPLIER;
        rowInfo[row].End = rowInfo[row].Start + rowInfo[row].Extent;
        top += rowInfo[row].Extent + spacing;
    }

    LONG left = gutter;
    for (int col = 0; col < gridLayoutInfo.columns(); col++)
    {
        columnInfo[col].Start = left;
        columnInfo[col].Extent = totalWidth * gridLayoutInfo.columnsPercents()[col] / C_MULTIPLIER;
        columnInfo[col].End = columnInfo[col].Start + columnInfo[col].Extent;
        left += columnInfo[col].Extent + spacing;
    }

    for (int row = 0; row < gridLayoutInfo.rows(); row++)
    {
        for (int col = 0; col < gridLayoutInfo.columns(); col++)
        {
            int i = gridLayoutInfo.cellChildMap()[row][col];
            if (((row == 0) || (gridLayoutInfo.cellChildMap()[row - 1][col] != i)) &&
                ((col == 0) || (gridLayoutInfo.cellChildMap()[row][col - 1] != i)))
            {
                left = columnInfo[col].Start;
                top = rowInfo[row].Start;

                int maxRow = row;
                while (((maxRow + 1) < gridLayoutInfo.rows()) && (gridLayoutInfo.cellChildMap()[maxRow + 1][col] == i))
                {
                    maxRow++;
                }
                int maxCol = col;
                while (((maxCol + 1) < gridLayoutInfo.columns()) && (gridLayoutInfo.cellChildMap()[row][maxCol + 1] == i))
                {
                    maxCol++;
                }

                LONG right = columnInfo[maxCol].End;
                LONG bottom = rowInfo[maxRow].End;
                RECT focusZoneRect{ left, top, right, bottom };
                AddZone(MakeZone(focusZoneRect));
            }
        }
    }
}

winrt::com_ptr<IZone> ZoneSet::ZoneFromWindow(HWND window) noexcept
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

winrt::com_ptr<IZoneSet> MakeZoneSet(ZoneSetConfig const& config) noexcept
{
    return winrt::make_self<ZoneSet>(config);
}
