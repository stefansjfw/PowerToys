#pragma once
#include "FancyZones.h"
#include "lib/ZoneSet.h"

namespace ZoneWindowUtils
{
    std::wstring GenerateUniqueId(HMONITOR monitor, PCWSTR deviceId, PCWSTR virtualDesktopId);
    std::wstring GenerateUniqueIdAllMonitorsArea(PCWSTR virtualDesktopId);
}

/**
 * Class representing single work area, which is defined by monitor and virtual desktop.
 */
interface __declspec(uuid("{7F017528-8110-4FB3-BE41-F472969C2560}")) IZoneWindow : public IUnknown
{
    /**
     * A window is being moved or resized. Track down window position and give zone layout
     * hints if dragging functionality is enabled.
     *
     * @param   window      Handle of window being moved or resized.
     */
    IFACEMETHOD(MoveSizeEnter)(HWND window) = 0;

    /**
     * A window has changed location, shape, or size. Track down window position and give zone layout
     * hints if dragging functionality is enabled.
     *
     * @param   ptScreen        Cursor coordinates.
     * @param   dragEnabled     Boolean indicating is giving hints about active zone layout enabled.
     *                          Hints are given while dragging window while holding SHIFT key.
     * @param   selectManyZones When this parameter is true, the set of highlighted zones is computed
     *                          by finding the minimum bounding rectangle of the zone(s) from which the
     *                          user started dragging and the zone(s) above which the user is hovering
     *                          at the moment this function is called. Otherwise, highlight only the zone(s)
     *                          above which the user is currently hovering.
     */
    IFACEMETHOD(MoveSizeUpdate)(POINT const& ptScreen, bool dragEnabled, bool selectManyZones) = 0;
    /**
     * The movement or resizing of a window has finished. Assign window to the zone of it
     * is dropped within zone borders.
     *
     * @param window   Handle of window being moved or resized.
     * @param ptScreen Cursor coordinates where window is dropped.
     */
    IFACEMETHOD(MoveSizeEnd)(HWND window, POINT const& ptScreen) = 0;
    /**
     * Assign window to the zone based on zone index inside zone layout.
     *
     * @param   window Handle of window which should be assigned to zone.
     * @param   zoneId Id of the Zone within zone layout.
     */
    IFACEMETHOD_(void, MoveWindowIntoZone)(HWND window, size_t zoneId) = 0;
    /**
     * Assign window to the zones based on the set of zone indices inside zone layout.
     *
     * @param   window   Handle of window which should be assigned to zone.
     * @param   zoneIds  The set of zone Ids within zone layout.
     */
    IFACEMETHOD_(void, MoveWindowIntoZones)(HWND window, const std::vector<size_t>& zoneIds) = 0;
    /**
     * Assign window to the zone based on direction (using WIN + LEFT/RIGHT arrow), based on zone Ids,
     * not their on-screen position.
     *
     * @param   window Handle of window which should be assigned to zone.
     * @param   vkCode Pressed arrow key.
     * @param   cycle  Whether we should move window to the first zone if we reached last zone in layout.
     *
     * @returns Boolean which is always true if cycle argument is set, otherwise indicating if there is more
     *          zones left in the zone layout in which window can move.
     */
    IFACEMETHOD_(bool, MoveWindowIntoZoneByIds)(HWND window, DWORD vkCode, bool cycle) = 0;
    /**
     * Assign window to the zone based on direction (using WIN + LEFT/RIGHT/UP/DOWN arrow), based on
     * their on-screen position.
     *
     * @param   window Handle of window which should be assigned to zone.
     * @param   vkCode Pressed arrow key.
     * @param   cycle  Whether we should move window to the first zone if we reached last zone in layout.
     *
     * @returns Boolean which is always true if cycle argument is set, otherwise indicating if there is more
     *          zones left in the zone layout in which window can move.
     */
    IFACEMETHOD_(bool, MoveWindowIntoZoneByDirection)(HWND window, DWORD vkCode, bool cycle) = 0;
    /**
     * Extend or shrink the window to an adjacent zone based on direction (using CTRL+WIN+ALT + LEFT/RIGHT/UP/DOWN arrow), based on
     * their on-screen position.
     *
     * @param   window Handle of window which should be assigned to zone.
     * @param   vkCode Pressed arrow key.
     *
     * @returns Boolean indicating whether the window was rezoned. False could be returned when there are no more
     *          zones available in the given direction.
     */
    IFACEMETHOD_(bool, ExtendWindowByDirection)(HWND window, DWORD vkCode) = 0;
    /**
     * Cycle through active zone layouts (giving hints about each layout).
     *
     * @param   vkCode Pressed key representing layout index.
     */
    IFACEMETHOD_(void, CycleActiveZoneSet)(DWORD vkCode) = 0;

    /**
     * Save information about zone in which window was assigned, when closing the window.
     * Used once we open same window again to assign it to its previous zone.
     *
     * @param   window Window handle.
     */
    IFACEMETHOD_(void, SaveWindowProcessToZoneIndex)(HWND window) = 0;
    /**
     * @returns Unique work area identifier. Format: <device-id>_<resolution>_<virtual-desktop-id>
     */
    IFACEMETHOD_(std::wstring, UniqueId)() const = 0;
    /**
     * @returns Active zone layout for this work area.
     */
    IFACEMETHOD_(IZoneSet*, ActiveZoneSet)() const = 0;
    IFACEMETHOD_(void, ShowZoneWindow)() = 0;
    IFACEMETHOD_(void, HideZoneWindow)() = 0;
    /**
     * Update currently active zone layout for this work area.
     */
    IFACEMETHOD_(void, UpdateActiveZoneSet)() = 0;
    /**
     * Clear the selected zones when this ZoneWindow loses focus.
     */
    IFACEMETHOD_(void, ClearSelectedZones)() = 0;
};

winrt::com_ptr<IZoneWindow> MakeZoneWindow(IZoneWindowHost* host, HINSTANCE hinstance, HMONITOR monitor,
    const std::wstring& uniqueId, const std::wstring& parentUniqueId, bool flashZones) noexcept;
