#include "pch.h"
#include <common/settings_objects.h>
#include <common/common.h>
#include <interface/powertoy_module_interface.h>
#include <interface/lowlevel_keyboard_event_data.h>
#include <interface/win_hook_event_data.h>
#include <lib/ZoneSet.h>
#include <lib/RegistryHelpers.h>
#include <lib/JsonHelpers.h>
#include <filesystem>
#include <algorithm>
#include <variant>

extern "C" IMAGE_DOS_HEADER __ImageBase;

BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
        case DLL_PROCESS_ATTACH:
            Trace::RegisterProvider();
            break;

        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
            break;

        case DLL_PROCESS_DETACH:
            Trace::UnregisterProvider();
            break;
    }
    return TRUE;
}

// This function is exported and called from FancyZonesEditor.exe to save a layout from the editor.
STDAPI PersistZoneSet(
    PCWSTR activeKey, // Registry key holding ActiveZoneSet
    WORD layoutId, // LayoutModel Id
    int zoneCount, // Number of zones in zones
    int zones[],  // Array of zones serialized in left/top/right/bottom chunks
    PCWSTR activeZoneSetTmpFile, // Temp file to store active zone set
    int showSpacing, // Editor showSpacing value
    int spacing, // Editor spacing value
    int editorZoneCount,
    PCWSTR id) // Editor zone count value
{
    /*UUID id{GUID_NULL};
    UuidCreate(&id);*/

    JSONHelpers::ZoneSetData data;
    data.uuid = L"{" + std::wstring{ id } + L"}";
    data.type = JSONHelpers::ZoneSetData::LayoutIdToType(layoutId);
    if (data.type != JSONHelpers::ZoneSetLayoutType::Custom) {
      data.zoneCount = zoneCount;
    }
    //                 MonitorFromPoint({}, MONITOR_DEFAULTTOPRIMARY), WHATS THIS???

    JSONHelpers::DeviceInfoJSON deviceInfo{ activeKey, { data, showSpacing == 1 ? true : false, spacing, editorZoneCount} };
    JSONHelpers::FancyZonesDataInstance().SetDeviceInfoToTmpFile(deviceInfo, activeZoneSetTmpFile);

    return S_OK;
}

class FancyZonesModule : public PowertoyModuleIface
{
public:
    // Return the display name of the powertoy, this will be cached
    virtual PCWSTR get_name() override
    {
        return L"FancyZones";
    }

    // Return array of the names of all events that this powertoy listens for, with
    // nullptr as the last element of the array. Nullptr can also be retured for empty list.
    virtual PCWSTR* get_events() override
    {
        static PCWSTR events[] = { ll_keyboard, win_hook_event, nullptr };
        return events;
    }

    // Return JSON with the configuration options.
    // These are the settings shown on the settings page along with their current values.
    virtual bool get_config(_Out_ PWSTR buffer, _Out_ int *buffer_size) override
    {
        return m_settings->GetConfig(buffer, buffer_size);
    }

    // Passes JSON with the configuration settings for the powertoy.
    // This is called when the user hits Save on the settings page.
    virtual void set_config(PCWSTR config) override
    {
        m_settings->SetConfig(config);
    }

    // Signal from the Settings editor to call a custom action.
    // This can be used to spawn more complex editors.
    virtual void call_custom_action(const wchar_t* action) override
    {
        m_settings->CallCustomAction(action);
    }

    // Enable the powertoy
    virtual void enable()
    {
        if (!m_app)
        {
            Trace::FancyZones::EnableFancyZones(true);
            m_app = MakeFancyZones(reinterpret_cast<HINSTANCE>(&__ImageBase), m_settings.get());
            if (m_app)
            {
                m_app->Run();
            }
        }
    }

    // Disable the powertoy
    virtual void disable()
    {
        Disable(true);
    }

    // Returns if the powertoy is enabled
    virtual bool is_enabled() override
    {
        return (m_app != nullptr);
    }

    // Handle incoming event, data is event-specific
    virtual intptr_t signal_event(const wchar_t* name, intptr_t data) override
    {
        if (m_app)
        {
            if (wcscmp(name, ll_keyboard) == 0)
            {
                // Return 1 if the keypress is to be suppressed (not forwarded to Windows), otherwise return 0.
                return HandleKeyboardHookEvent(reinterpret_cast<LowlevelKeyboardEvent*>(data));
            }
            else if (wcscmp(name, win_hook_event) == 0)
            {
                // Return value is ignored
                HandleWinHookEvent(reinterpret_cast<WinHookEvent*>(data));
            }
        }
        return 0;
    }

    virtual void register_system_menu_helper(PowertoySystemMenuIface* helper) override { }
    virtual void signal_system_menu_action(const wchar_t* name) override { }

    // Destroy the powertoy and free memory
    virtual void destroy() override
    {
        Disable(false);
        delete this;
    }

    FancyZonesModule()
    {
        m_settings = MakeFancyZonesSettings(reinterpret_cast<HINSTANCE>(&__ImageBase), FancyZonesModule::get_name());
        JSONHelpers::FancyZonesDataInstance().LoadFancyZonesData();
    }

private:
    bool IsInterestingWindow(HWND window)
    {
        auto style = GetWindowLongPtr(window, GWL_STYLE);
        auto exStyle = GetWindowLongPtr(window, GWL_EXSTYLE);
        // Ignore:
        if (GetAncestor(window, GA_ROOT) != window || // windows that are not top-level
            GetWindow(window, GW_OWNER) != nullptr || // windows that have an owner - like Save As dialogs
            (style & WS_CHILD) != 0 || // windows that are child elements of other windows - like buttons
            (style & WS_DISABLED) != 0 || // windows that are disabled
            (exStyle & WS_EX_TOOLWINDOW) != 0 || // toolbar windows
            !IsWindowVisible(window)) // invisible windows
        {
            return false;
        }
        // Filter some windows like the Start menu or Cortana
        auto windowAndPath = get_filtered_base_window_and_path(window);
        if (windowAndPath.hwnd == nullptr)
        {
            return false;
        }
        // Filter out user specified apps
        CharUpperBuffW(windowAndPath.process_path.data(), (DWORD)windowAndPath.process_path.length());
        if (m_settings)
        {
            for (const auto& excluded : m_settings->GetSettings().excludedAppsArray)
            {
                if (windowAndPath.process_path.find(excluded) != std::wstring::npos)
                {
                    return false;
                }
            }
        }
        return true;
    }

    void Disable(bool const traceEvent)
    {
        if (m_app) {
            JSONHelpers::FancyZonesDataInstance().SaveFancyZonesData();
            if (traceEvent) 
            {
                Trace::FancyZones::EnableFancyZones(false);
            }
            m_app->Destroy();
            m_app = nullptr;
        }
    }

    intptr_t HandleKeyboardHookEvent(LowlevelKeyboardEvent* data) noexcept;
    void HandleWinHookEvent(WinHookEvent* data) noexcept;
    void MoveSizeStart(HWND window, POINT const& ptScreen) noexcept;
    void MoveSizeEnd(HWND window, POINT const& ptScreen) noexcept;
    void MoveSizeUpdate(POINT const& ptScreen) noexcept;

    HANDLE m_movedWindow = nullptr;
    winrt::com_ptr<IFancyZones> m_app;
    winrt::com_ptr<IFancyZonesSettings> m_settings;
};

intptr_t FancyZonesModule::HandleKeyboardHookEvent(LowlevelKeyboardEvent* data) noexcept
{
    if (data->wParam == WM_KEYDOWN)
    {
        return m_app.as<IFancyZonesCallback>()->OnKeyDown(data->lParam) ? 1 : 0;
    }
    return 0;
}

void FancyZonesModule::HandleWinHookEvent(WinHookEvent* data) noexcept
{
    POINT ptScreen;
    GetPhysicalCursorPos(&ptScreen);

    switch (data->event)
    {
    case EVENT_SYSTEM_MOVESIZESTART:
    {
        MoveSizeStart(data->hwnd, ptScreen);
    }
    break;

    case EVENT_SYSTEM_MOVESIZEEND:
    {
        MoveSizeEnd(data->hwnd, ptScreen);
    }
    break;

    case EVENT_OBJECT_LOCATIONCHANGE:
    {
        if (m_app.as<IFancyZonesCallback>()->InMoveSize())
        {
            MoveSizeUpdate(ptScreen);
        }
    }
    break;

    case EVENT_OBJECT_NAMECHANGE:
    {
        // The accessibility name of the desktop window changes whenever the user
        // switches virtual desktops.
        if (data->hwnd == GetDesktopWindow())
        {
            Trace::VirtualDesktopChanged();
            m_app.as<IFancyZonesCallback>()->VirtualDesktopChanged();
        }
    }
    break;

    case EVENT_OBJECT_UNCLOAKED:
    case EVENT_OBJECT_SHOW:
    case EVENT_OBJECT_CREATE:
    {
        if (data->idObject == OBJID_WINDOW)
        {
            if (IsInterestingWindow(data->hwnd))
            {
                m_app.as<IFancyZonesCallback>()->WindowCreated(data->hwnd);
            }
        }
    }
    break;

    default:
        break;
    }
}

void FancyZonesModule::MoveSizeStart(HWND window, POINT const& ptScreen) noexcept
{
    if (IsInterestingWindow(window))
    {
        if (auto monitor = MonitorFromPoint(ptScreen, MONITOR_DEFAULTTONULL))
        {
            m_movedWindow = window;
            m_app.as<IFancyZonesCallback>()->MoveSizeStart(window, monitor, ptScreen);
        }
    }
}

void FancyZonesModule::MoveSizeEnd(HWND window, POINT const& ptScreen) noexcept
{
    if (IsInterestingWindow(window) || (window != nullptr && window == m_movedWindow))
    {
        m_movedWindow = nullptr;
        m_app.as<IFancyZonesCallback>()->MoveSizeEnd(window, ptScreen);
    }
}

void FancyZonesModule::MoveSizeUpdate(POINT const& ptScreen) noexcept
{
    if (auto monitor = MonitorFromPoint(ptScreen, MONITOR_DEFAULTTONULL))
    {
        m_app.as<IFancyZonesCallback>()->MoveSizeUpdate(monitor, ptScreen);
    }
}

extern "C" __declspec(dllexport) PowertoyModuleIface*  __cdecl powertoy_create()
{
  return new FancyZonesModule();
}


