#include "pch.h"

struct ZoneSet : winrt::implements<ZoneSet, IZoneSet>
{
public:
    ZoneSet(ZoneSetConfig const& config) : m_config(config)
    {
        /*if (config.ZoneCount > 0)
        {
            InitialPopulateZones();
        }*/
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
    IFACEMETHODIMP_(void) Save() noexcept;
    IFACEMETHODIMP_(void) MoveZoneToFront(winrt::com_ptr<IZone> zone) noexcept;
    IFACEMETHODIMP_(void) MoveZoneToBack(winrt::com_ptr<IZone> zone) noexcept;
    IFACEMETHODIMP_(void) MoveWindowIntoZoneByIndex(HWND window, HWND zoneWindow, int index) noexcept;
    IFACEMETHODIMP_(void) MoveWindowIntoZoneByDirection(HWND window, HWND zoneWindow, DWORD vkCode) noexcept;
    IFACEMETHODIMP_(void) MoveSizeEnd(HWND window, HWND zoneWindow, POINT ptClient) noexcept;
    IFACEMETHODIMP_(void) CalculateZones(MONITORINFO monitorInfo, int zoneCount, int spacing) noexcept;

private:
    void InitialPopulateZones() noexcept;
    void GenerateGridZones(MONITORINFO const& mi) noexcept;
    void DoGridLayout(SIZE const& zoneArea, int numCols, int numRows) noexcept;

    void CalculateFocusLayout(Rect workArea);
    void CalculateColumnsAndRowsLayout(Rect workArea, JSONHelpers::ZoneSetLayoutType type, int zoneCount, int spacing);

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

IFACEMETHODIMP_(void) ZoneSet::Save() noexcept
{
    size_t const zoneCount = m_zones.size();
    if (zoneCount == 0)
    {
        RegistryHelpers::DeleteZoneSet(m_config.ResolutionKey, m_config.Id);
    }
    else
    {
        ZoneSetPersistedData data{};
        data.LayoutId = m_config.LayoutId;
        data.ZoneCount = static_cast<DWORD>(zoneCount);
        data.Layout = m_config.Layout;

        int i = 0;
        for (auto iter = m_zones.begin(); iter != m_zones.end(); iter++)
        {
            winrt::com_ptr<IZone> zone = iter->as<IZone>();
            CopyRect(&data.Zones[i++], &zone->GetZoneRect());
        }

        wil::unique_cotaskmem_string guid;
        if (SUCCEEDED_LOG(StringFromCLSID(m_config.Id, &guid)))
        {
            if (wil::unique_hkey hkey{ RegistryHelpers::CreateKey(m_config.ResolutionKey) })
            {
                RegSetValueExW(hkey.get(), guid.get(), 0, REG_BINARY, reinterpret_cast<BYTE*>(&data), sizeof(data));
            }
        }
    }
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
  }
}

void ZoneSet::CalculateFocusLayout(Rect workArea)
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

void ZoneSet::CalculateColumnsAndRowsLayout(Rect workArea, JSONHelpers::ZoneSetLayoutType type, int zoneCount, int spacing)
{
  int gutter = spacing;
  int c_multiplier = 10000;

  int zonePercent = c_multiplier / zoneCount;

  LONG totalWidth;// = workArea.width() - (gutter * 2) - (spacing * (zoneCount - 1));
  LONG totalHeight;// = workArea.height() - (gutter * 2);

  LONG cellWidth; // = totalWidth * colsPercent / c_multiplier;
  LONG cellHeight;// = totalHeight * rowPercent / c_multiplier;

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

void ZoneSet::InitialPopulateZones() noexcept
{
    // TODO: reconcile the pregenerated FZ layouts with the editor

    MONITORINFO mi{};
    mi.cbSize = sizeof(mi);
    if (GetMonitorInfoW(m_config.Monitor, &mi))
    {
        if ((m_config.Layout == JSONHelpers::ZoneSetLayoutType::Grid) || (m_config.Layout == JSONHelpers::ZoneSetLayoutType::Rows))
        {
            GenerateGridZones(mi);
        }

        Save();
    }
}

void ZoneSet::GenerateGridZones(MONITORINFO const& mi) noexcept
{
    Rect workArea(mi.rcWork);

    int numCols, numRows;
    if (m_config.Layout == JSONHelpers::ZoneSetLayoutType::Grid)
    {
        switch (m_config.ZoneCount)
        {
            case 1: numCols = 1; numRows = 1; break;
            case 2: numCols = 2; numRows = 1; break;
            case 3: numCols = 2; numRows = 2; break;
            case 4: numCols = 2; numRows = 2; break;
            case 5: numCols = 3; numRows = 3; break;
            case 6: numCols = 3; numRows = 3; break;
            case 7: numCols = 3; numRows = 3; break;
            case 8: numCols = 3; numRows = 3; break;
            case 9: numCols = 3; numRows = 3; break;
        }

        if ((m_config.ZoneCount == 2) && (workArea.height() > workArea.width()))
        {
            numCols = 1;
            numRows = 2;
        }
    }
    else if (m_config.Layout == JSONHelpers::ZoneSetLayoutType::Rows)
    {
        numCols = m_config.ZoneCount;
        numRows = 1;
    }

    SIZE const zoneArea = {
        workArea.width(),
        workArea.height()
    };

    DoGridLayout(zoneArea, numCols, numRows);
}

void ZoneSet::DoGridLayout(SIZE const& zoneArea, int numCols, int numRows) noexcept
{
    auto x = 0;
    auto y = 0;
    auto const zoneWidth = (zoneArea.cx / numCols);
    auto const zoneHeight = (zoneArea.cy / numRows);
    for (auto i = 1; i <= m_config.ZoneCount; i++)
    {
        auto col = numCols - (i % numCols);
        RECT const zoneRect = { x, y, x + zoneWidth, y + zoneHeight };
        AddZone(MakeZone(zoneRect), false);

        x += zoneWidth;
        if (col == numCols)
        {
            x = 0;
            y += zoneHeight;
        }
    }
}

winrt::com_ptr<IZoneSet> MakeZoneSet(ZoneSetConfig const& config) noexcept
{
    return winrt::make_self<ZoneSet>(config);
}
