#pragma once

#include "Zone.h"
#include "JsonHelpers.h"


interface __declspec(uuid("{E4839EB7-669D-49CF-84A9-71A2DFD851A3}")) IZoneSet : public IUnknown
{
    IFACEMETHOD_(GUID, Id)() = 0;
    IFACEMETHOD_(WORD, LayoutId)() = 0;
    IFACEMETHOD(AddZone)(winrt::com_ptr<IZone> zone, bool front) = 0;
    IFACEMETHOD(RemoveZone)(winrt::com_ptr<IZone> zone) = 0;
    IFACEMETHOD_(winrt::com_ptr<IZone>, ZoneFromPoint)(POINT pt) = 0;
    IFACEMETHOD_(winrt::com_ptr<IZone>, ZoneFromWindow)(HWND window) = 0;
    IFACEMETHOD_(int, GetZoneIndexFromWindow)(HWND window) = 0;
    IFACEMETHOD_(std::vector<winrt::com_ptr<IZone>>, GetZones)() = 0;
    IFACEMETHOD_(JSONHelpers::ZoneSetLayoutType, GetLayout)() = 0;
    IFACEMETHOD_(winrt::com_ptr<IZoneSet>, MakeCustomClone)() = 0;
    IFACEMETHOD_(void, MoveZoneToFront)(winrt::com_ptr<IZone> zone) = 0;
    IFACEMETHOD_(void, MoveZoneToBack)(winrt::com_ptr<IZone> zone) = 0;
    IFACEMETHOD_(void, MoveWindowIntoZoneByIndex)(HWND window, HWND zoneWindow, int index) = 0;
    IFACEMETHOD_(void, MoveWindowIntoZoneByDirection)(HWND window, HWND zoneWindow, DWORD vkCode) = 0;
    IFACEMETHOD_(void, MoveSizeEnd)(HWND window, HWND zoneWindow, POINT ptClient) = 0;
    IFACEMETHOD_(void, CalculateZones)(MONITORINFO monitorInfo, int zoneCount, int spacing) = 0;
};

#define VERSION_PERSISTEDDATA 0x0000F00D
struct ZoneSetPersistedData
{
    static constexpr inline size_t MAX_ZONES = 40;

    DWORD Version{VERSION_PERSISTEDDATA};
    WORD LayoutId{};
    DWORD ZoneCount{};
    JSONHelpers::ZoneSetLayoutType Layout{};
    RECT Zones[MAX_ZONES]{};
};

struct ZoneSetConfig
{
    ZoneSetConfig(
        GUID id,
        WORD layoutId,
        HMONITOR monitor,
        PCWSTR resolutionKey,
        JSONHelpers::ZoneSetLayoutType layout,
        int zoneCount) noexcept :
            Id(id),
            LayoutId(layoutId),
            Monitor(monitor),
            ResolutionKey(resolutionKey),
            Layout(layout),
            ZoneCount(zoneCount)
    {
    }

    GUID Id{};
    WORD LayoutId{}; //TODO(stefan) UNNEEDED?
    HMONITOR Monitor{};
    PCWSTR ResolutionKey{}; //TODO(stefan) THIS WILL BE UNNEEDED WHEN REGSTRY LOGIC IS DEPRECATED. STILL NEEDED NOW FOR BACKWARD COMPATIBILITY
    JSONHelpers::ZoneSetLayoutType Layout{};
    int ZoneCount{};
    bool IsCustom{};
};

winrt::com_ptr<IZoneSet> MakeZoneSet(ZoneSetConfig const& config) noexcept;