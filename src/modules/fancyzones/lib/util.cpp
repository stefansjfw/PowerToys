#include "pch.h"
#include "util.h"
#include "Settings.h"

#include <common/common.h>
#include <common/dpi_aware.h>

#include <array>
#include <sstream>
#include <complex>

#include "FancyZonesDataTypes.h"

// Non-Localizable strings
namespace NonLocalizable
{
    const wchar_t PowerToysAppPowerLauncher[] = L"POWERLAUNCHER.EXE";
    const wchar_t PowerToysAppFZEditor[] = L"FANCYZONESEDITOR.EXE";
}

namespace
{
    bool IsZonableByProcessPath(const std::wstring& processPath, const std::vector<std::wstring>& excludedApps)
    {
        // Filter out user specified apps
        CharUpperBuffW(const_cast<std::wstring&>(processPath).data(), (DWORD)processPath.length());
        if (find_app_name_in_path(processPath, excludedApps))
        {
            return false;
        }
        if (find_app_name_in_path(processPath, { NonLocalizable::PowerToysAppPowerLauncher }))
        {
            return false;
        }
        if (find_app_name_in_path(processPath, { NonLocalizable::PowerToysAppFZEditor }))
        {
            return false;
        }
        return true;
    }
}

namespace FancyZonesUtils
{
    std::wstring TrimDeviceId(const std::wstring& deviceId)
    {
        // We're interested in the unique part between the first and last #'s
        // Example input: \\?\DISPLAY#DELA026#5&10a58c63&0&UID16777488#{e6f07b5f-ee97-4a90-b076-33f57bf4eaa7}
        // Example output: DELA026#5&10a58c63&0&UID16777488
        static const std::wstring defaultDeviceId = L"FallbackDevice";
        if (deviceId.empty())
        {
            return defaultDeviceId;
        }

        size_t start = deviceId.find(L'#');
        size_t end = deviceId.rfind(L'#');
        if (start != std::wstring::npos && end != std::wstring::npos && start != end)
        {
            size_t size = end - (start + 1);
            return deviceId.substr(start + 1, size);
        }
        else
        {
            return defaultDeviceId;
        }
    }
    
    typedef BOOL(WINAPI* GetDpiForMonitorInternalFunc)(HMONITOR, UINT, UINT*, UINT*);
    UINT GetDpiForMonitor(HMONITOR monitor) noexcept
    {
        UINT dpi{};
        if (wil::unique_hmodule user32{ LoadLibrary(L"user32.dll") })
        {
            if (auto func = reinterpret_cast<GetDpiForMonitorInternalFunc>(GetProcAddress(user32.get(), "GetDpiForMonitorInternal")))
            {
                func(monitor, 0, &dpi, &dpi);
            }
        }

        if (dpi == 0)
        {
            if (wil::unique_hdc hdc{ GetDC(nullptr) })
            {
                dpi = GetDeviceCaps(hdc.get(), LOGPIXELSX);
            }
        }

        return (dpi == 0) ? DPIAware::DEFAULT_DPI : dpi;
    }

    void OrderMonitors(std::vector<std::pair<HMONITOR, RECT>>& monitorInfo)
    {
        const size_t nMonitors = monitorInfo.size();
        // blocking[i][j] - whether monitor i blocks monitor j in the ordering, i.e. monitor i should go before monitor j
        std::vector<std::vector<bool>> blocking(nMonitors, std::vector<bool>(nMonitors, false));

        // blockingCount[j] - the number of monitors which block monitor j
        std::vector<size_t> blockingCount(nMonitors, 0);

        for (size_t i = 0; i < nMonitors; i++)
        {
            RECT rectI = monitorInfo[i].second;
            for (size_t j = 0; j < nMonitors; j++)
            {
                RECT rectJ = monitorInfo[j].second;
                blocking[i][j] = rectI.top < rectJ.bottom && rectI.left < rectJ.right && i != j;
                if (blocking[i][j])
                {
                    blockingCount[j]++;
                }
            }
        }

        // used[i] - whether the sorting algorithm has used monitor i so far
        std::vector<bool> used(nMonitors, false);

        // the sorted sequence of monitors
        std::vector<std::pair<HMONITOR, RECT>> sortedMonitorInfo;

        for (size_t iteration = 0; iteration < nMonitors; iteration++)
        {
            // Indices of candidates to become the next monitor in the sequence
            std::vector<size_t> candidates;

            // First, find indices of all unblocked monitors
            for (size_t i = 0; i < nMonitors; i++)
            {
                if (blockingCount[i] == 0 && !used[i])
                {
                    candidates.push_back(i);
                }
            }

            // In the unlikely event that there are no unblocked monitors, declare all unused monitors as candidates.
            if (candidates.empty())
            {
                for (size_t i = 0; i < nMonitors; i++)
                {
                    if (!used[i])
                    {
                        candidates.push_back(i);
                    }
                }
            }

            // Pick the lexicographically smallest monitor as the next one
            size_t smallest = candidates[0];
            for (size_t j = 1; j < candidates.size(); j++)
            {
                size_t current = candidates[j];

                // Compare (top, left) lexicographically
                if (std::tie(monitorInfo[current].second.top, monitorInfo[current].second.left) <
                    std::tie(monitorInfo[smallest].second.top, monitorInfo[smallest].second.left))
                {
                    smallest = current;
                }
            }

            used[smallest] = true;
            sortedMonitorInfo.push_back(monitorInfo[smallest]);
            for (size_t i = 0; i < nMonitors; i++)
            {
                if (blocking[smallest][i])
                {
                    blockingCount[i]--;
                }
            }
        }

        monitorInfo = std::move(sortedMonitorInfo);
    }

    void SizeWindowToRect(HWND window, RECT rect) noexcept
    {
        WINDOWPLACEMENT placement{};
        ::GetWindowPlacement(window, &placement);

        // Wait if SW_SHOWMINIMIZED would be removed from window (Issue #1685)
        for (int i = 0; i < 5 && (placement.showCmd == SW_SHOWMINIMIZED); ++i)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            ::GetWindowPlacement(window, &placement);
        }

        // Do not restore minimized windows. We change their placement though so they restore to the correct zone.
        if ((placement.showCmd != SW_SHOWMINIMIZED) &&
            (placement.showCmd != SW_MINIMIZE))
        {
            placement.showCmd = SW_RESTORE;
        }

        // Remove maximized show command to make sure window is moved to the correct zone.
        if (placement.showCmd == SW_SHOWMAXIMIZED)
        {
            placement.showCmd = SW_RESTORE;
            placement.flags &= ~WPF_RESTORETOMAXIMIZED;
        }

        placement.rcNormalPosition = rect;
        placement.flags |= WPF_ASYNCWINDOWPLACEMENT;

        ::SetWindowPlacement(window, &placement);
        // Do it again, allowing Windows to resize the window and set correct scaling
        // This fixes Issue #365
        ::SetWindowPlacement(window, &placement);
    }

    bool HasNoVisibleOwner(HWND window) noexcept
    {
        auto owner = GetWindow(window, GW_OWNER);
        if (owner == nullptr)
        {
            return true; // There is no owner at all
        }
        if (!IsWindowVisible(owner))
        {
            return true; // Owner is invisible
        }
        RECT rect;
        if (!GetWindowRect(owner, &rect))
        {
            return false; // Could not get the rect, return true (and filter out the window) just in case
        }
        // It is enough that the window is zero-sized in one dimension only.
        return rect.top == rect.bottom || rect.left == rect.right;
    }

    bool IsStandardWindow(HWND window)
    {
        if (GetAncestor(window, GA_ROOT) != window || !IsWindowVisible(window))
        {
            return false;
        }
        auto style = GetWindowLong(window, GWL_STYLE);
        auto exStyle = GetWindowLong(window, GWL_EXSTYLE);
        // WS_POPUP need to have a border or minimize/maximize buttons,
        // otherwise the window is "not interesting"
        if ((style & WS_POPUP) == WS_POPUP &&
            (style & WS_THICKFRAME) == 0 &&
            (style & WS_MINIMIZEBOX) == 0 &&
            (style & WS_MAXIMIZEBOX) == 0)
        {
            return false;
        }
        if ((style & WS_CHILD) == WS_CHILD ||
            (style & WS_DISABLED) == WS_DISABLED ||
            (exStyle & WS_EX_TOOLWINDOW) == WS_EX_TOOLWINDOW ||
            (exStyle & WS_EX_NOACTIVATE) == WS_EX_NOACTIVATE)
        {
            return false;
        }
        std::array<char, 256> class_name;
        GetClassNameA(window, class_name.data(), static_cast<int>(class_name.size()));
        if (is_system_window(window, class_name.data()))
        {
            return false;
        }
        auto process_path = get_process_path(window);
        // Check for Cortana:
        if (strcmp(class_name.data(), "Windows.UI.Core.CoreWindow") == 0 &&
            process_path.ends_with(L"SearchUI.exe"))
        {
            return false;
        }

        return true;
    }

    bool IsCandidateForLastKnownZone(HWND window, const std::vector<std::wstring>& excludedApps) noexcept
    {
        auto zonable = IsStandardWindow(window) && HasNoVisibleOwner(window);
        if (!zonable)
        {
            return false;
        }

        return IsZonableByProcessPath(get_process_path(window), excludedApps);
    }

    bool IsCandidateForZoning(HWND window, const std::vector<std::wstring>& excludedApps) noexcept
    {
        if (!IsStandardWindow(window))
        {
            return false;
        }

        return IsZonableByProcessPath(get_process_path(window), excludedApps);
    }

    bool IsWindowMaximized(HWND window) noexcept
    {
        WINDOWPLACEMENT placement{};
        if (GetWindowPlacement(window, &placement) &&
            placement.showCmd == SW_SHOWMAXIMIZED)
        {
            return true;
        }
        return false;
    }

    void SaveWindowSizeAndOrigin(HWND window) noexcept
    {
        HANDLE handle = GetPropW(window, ZonedWindowProperties::PropertyRestoreSizeID);
        if (handle)
        {
            // Size already set, skip
            return;
        }

        RECT rect;
        if (GetWindowRect(window, &rect))
        {
            int width = rect.right - rect.left;
            int height = rect.bottom - rect.top;
            int originX = rect.left;
            int originY = rect.top;

            DPIAware::InverseConvert(MonitorFromWindow(window, MONITOR_DEFAULTTONULL), width, height);
            DPIAware::InverseConvert(MonitorFromWindow(window, MONITOR_DEFAULTTONULL), originX, originY);

            std::array<int, 2> windowSizeData = { width, height };
            std::array<int, 2> windowOriginData = { originX, originY };
            HANDLE rawData;
            memcpy(&rawData, windowSizeData.data(), sizeof rawData);
            SetPropW(window, ZonedWindowProperties::PropertyRestoreSizeID, rawData);
            memcpy(&rawData, windowOriginData.data(), sizeof rawData);
            SetPropW(window, ZonedWindowProperties::PropertyRestoreOriginID, rawData);
        }
    }

    void RestoreWindowSize(HWND window) noexcept
    {
        auto windowSizeData = GetPropW(window, ZonedWindowProperties::PropertyRestoreSizeID);
        if (windowSizeData)
        {
            std::array<int, 2> windowSize;
            memcpy(windowSize.data(), &windowSizeData, sizeof windowSize);

            // {width, height}
            DPIAware::Convert(MonitorFromWindow(window, MONITOR_DEFAULTTONULL), windowSize[0], windowSize[1]);

            RECT rect;
            if (GetWindowRect(window, &rect))
            {
                rect.right = rect.left + windowSize[0];
                rect.bottom = rect.top + windowSize[1];
                SizeWindowToRect(window, rect);
            }

            ::RemoveProp(window, ZonedWindowProperties::PropertyRestoreSizeID);
        }
    }

    void RestoreWindowOrigin(HWND window) noexcept
    {
        auto windowOriginData = GetPropW(window, ZonedWindowProperties::PropertyRestoreOriginID);
        if (windowOriginData)
        {
            std::array<int, 2> windowOrigin;
            memcpy(windowOrigin.data(), &windowOriginData, sizeof windowOrigin);

            // {width, height}
            DPIAware::Convert(MonitorFromWindow(window, MONITOR_DEFAULTTONULL), windowOrigin[0], windowOrigin[1]);

            RECT rect;
            if (GetWindowRect(window, &rect))
            {
                int xOffset = windowOrigin[0] - rect.left;
                int yOffset = windowOrigin[1] - rect.top;

                rect.left += xOffset;
                rect.right += xOffset;
                rect.top += yOffset;
                rect.bottom += yOffset;
                SizeWindowToRect(window, rect);
            }

            ::RemoveProp(window, ZonedWindowProperties::PropertyRestoreOriginID);
        }
    }

    bool IsValidGuid(const std::wstring& str)
    {
        GUID id;
        return SUCCEEDED(CLSIDFromString(str.c_str(), &id));
    }

    std::optional<FancyZonesDataTypes::DeviceIdData> GenerateUniqueId(HMONITOR monitor, const std::wstring& deviceId, const GUID& virtualDesktopId)
    {
        MONITORINFOEXW mi;
        mi.cbSize = sizeof(mi);
        if (GetMonitorInfo(monitor, &mi))
        {
            Rect const monitorRect(mi.rcMonitor);
            // Unique identifier format: <parsed-device-id>_<width>_<height>_<virtual-desktop-id>
            return FancyZonesDataTypes::DeviceIdData{ TrimDeviceId(deviceId), monitorRect.width(), monitorRect.height(), virtualDesktopId };
        }
        return std::nullopt;
    }

    std::optional<FancyZonesDataTypes::DeviceIdData> GenerateUniqueIdAllMonitorsArea(const GUID& virtualDesktopId)
    {
        RECT combinedResolution = GetAllMonitorsCombinedRect<&MONITORINFO::rcMonitor>();
        int combinedResolutionWidth = static_cast<int>(combinedResolution.right - combinedResolution.left);
        int combinedResolutionHeight = static_cast<int>(combinedResolution.bottom - combinedResolution.top);

        return FancyZonesDataTypes::DeviceIdData{ ZonedWindowProperties::MultiMonitorDeviceID, combinedResolutionWidth, combinedResolutionHeight, virtualDesktopId };
    }

    std::optional<FancyZonesDataTypes::DeviceIdData> GenerateMonitorId(MONITORINFOEX mi, HMONITOR monitor, const GUID& virtualDesktopId)
    {
        DISPLAY_DEVICE displayDevice = { sizeof(displayDevice) };
        PCWSTR deviceId = nullptr;

        bool validMonitor = true;
        if (EnumDisplayDevices(mi.szDevice, 0, &displayDevice, 1))
        {
            if (displayDevice.DeviceID[0] != L'\0')
            {
                deviceId = displayDevice.DeviceID;
            }
        }

        if (!deviceId)
        {
            deviceId = GetSystemMetrics(SM_REMOTESESSION) ?
                           L"\\\\?\\DISPLAY#REMOTEDISPLAY#" :
                           L"\\\\?\\DISPLAY#LOCALDISPLAY#";
        }

        return GenerateUniqueId(monitor, deviceId, virtualDesktopId);
    }

    size_t ChooseNextZoneByPosition(DWORD vkCode, RECT windowRect, const std::vector<RECT>& zoneRects) noexcept
    {
        using complex = std::complex<double>;
        const size_t invalidResult = zoneRects.size();
        const double inf = 1e100;
        const double eccentricity = 2.0;

        auto rectCenter = [](RECT rect) {
            return complex {
                0.5 * rect.left + 0.5 * rect.right,
                0.5 * rect.top + 0.5 * rect.bottom
            };
        };

        auto distance = [&](complex arrowDirection, complex zoneDirection) {
            double result = inf;

            try
            {
                double scalarProduct = (arrowDirection * conj(zoneDirection)).real();
                if (scalarProduct <= 0.0)
                {
                    return inf;
                }

                // no need to divide by abs(arrowDirection) because it's = 1
                double cosAngle = scalarProduct / abs(zoneDirection);
                double tanAngle = abs(tan(acos(cosAngle)));

                if (tanAngle > 10)
                {
                    // The angle is too wide
                    return inf;
                }

                // find the intersection with the ellipse with given eccentricity and major axis along arrowDirection
                double intersectY = 2 * eccentricity / (1.0 + eccentricity * eccentricity * tanAngle * tanAngle);
                double distanceEstimate = scalarProduct / intersectY;

                if (std::isfinite(distanceEstimate))
                {
                    result = distanceEstimate;
                }
            }
            catch (...)
            {
            }

            return result;
        };
        std::vector<std::pair<size_t, complex>> candidateCenters;
        for (size_t i = 0; i < zoneRects.size(); i++)
        {
            auto center = rectCenter(zoneRects[i]);

            // Offset the zone slightly, to differentiate in case there are overlapping zones
            center += 0.001 * (i + 1);

            candidateCenters.emplace_back(i, center);
        }

        complex directionVector, windowCenter = rectCenter(windowRect);

        switch (vkCode)
        {
        case VK_UP:
            directionVector = { 0.0, -1.0 };
            break;
        case VK_DOWN:
            directionVector = { 0.0, 1.0 };
            break;
        case VK_LEFT:
            directionVector = { -1.0, 0.0 };
            break;
        case VK_RIGHT:
            directionVector = { 1.0, 0.0 };
            break;
        default:
            return invalidResult;
        }

        size_t closestIdx = invalidResult;
        double smallestDistance = inf;

        for (auto [zoneIdx, zoneCenter] : candidateCenters)
        {
            double dist = distance(directionVector, zoneCenter - windowCenter);
            if (dist < smallestDistance)
            {
                smallestDistance = dist;
                closestIdx = zoneIdx;
            }
        }

        return closestIdx;
    }

    RECT PrepareRectForCycling(RECT windowRect, RECT zoneWindowRect, DWORD vkCode) noexcept
    {
        LONG deltaX = 0, deltaY = 0;
        switch (vkCode)
        {
        case VK_UP:
            deltaY = zoneWindowRect.bottom - zoneWindowRect.top;
            break;
        case VK_DOWN:
            deltaY = zoneWindowRect.top - zoneWindowRect.bottom;
            break;
        case VK_LEFT:
            deltaX = zoneWindowRect.right - zoneWindowRect.left;
            break;
        case VK_RIGHT:
            deltaX = zoneWindowRect.left - zoneWindowRect.right;
        }

        windowRect.left += deltaX;
        windowRect.right += deltaX;
        windowRect.top += deltaY;
        windowRect.bottom += deltaY;

        return windowRect;
    }
}
