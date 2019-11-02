#include "pch.h"
#include <interface/powertoy_module_interface.h>
#include <interface/lowlevel_keyboard_event_data.h>
#include <interface/win_hook_event_data.h>
#include <common/settings_objects.h>
#include <set>
#include "trace.h"
#include "resource.h"

extern "C" IMAGE_DOS_HEADER __ImageBase;

BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved) {
  switch (ul_reason_for_call) {
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

const static wchar_t* MODULE_NAME = L"AlwaysOnTop";
const static wchar_t* MODULE_DESC = L"";
const static wchar_t* HOTKEY_NAME = L"AlwaysOnTop_HotKey";

struct AllwaysOnTopSettings {
  // Default hotkey WIN + ALT + T
  PowerToysSettings::HotkeyObject editorHotkey =
    PowerToysSettings::HotkeyObject::from_settings(true, false, true, false, VK_OEM_3, L"t");
};

namespace {
constexpr int KAlwaysOnTopSeparatorID = 0x1234;
constexpr int KAlwaysOnTopMenuID      = 0x5678;
constexpr int KMenuItemStringSize = 14;
WCHAR KMenuItemString[KMenuItemStringSize] = L"Always On Top";
}

class AlwaysOnTop : public PowertoyModuleIface {
private:

  bool                   mEnabled{ false };
  std::pair<HWND, HMENU> mCurrentlyOnTop{ nullptr, nullptr };
  std::set<HMENU>        mModified;
  AllwaysOnTopSettings   mSettings;

  void init_settings();

  intptr_t HandleKeyboardHookEvent(const LowlevelKeyboardEvent& data) noexcept;
  void HandleWinHookEvent(const WinHookEvent& data) noexcept;

  void ProcessCommand(HWND aWindow);
  bool SetWindowOnTop(HWND aWindow);
  void ResetCurrentOnTop();
  bool InjectMenuItem(HWND aWindow);
  void ResetAll();

  void LoadSettings(PCWSTR config, bool aFromFile);
  void SaveSettings();

public:

  AlwaysOnTop()
  {
    // Read settings from file.
    LoadSettings(MODULE_NAME, true);
    init_settings();
  }

  virtual void destroy() override
  {
    ResetAll();
    delete this;
  }

  virtual const wchar_t* get_name() override
  {
    return MODULE_NAME;
  }

  virtual const wchar_t** get_events() override
  {
    static const wchar_t* events[] = { ll_keyboard,
                                       win_hook_event,
                                       nullptr };

    return events;
  }

  virtual bool get_config(wchar_t* buffer, int* buffer_size) override
  {
    HINSTANCE hinstance = reinterpret_cast<HINSTANCE>(&__ImageBase);

    PowerToysSettings::Settings settings(hinstance, get_name());
    settings.add_hotkey(HOTKEY_NAME, IDS_SETTING_ALWAYS_ON_TOP_HOTKEY, mSettings.editorHotkey);

    return settings.serialize_to_buffer(buffer, buffer_size);
  }

  virtual void set_config(const wchar_t* config) override
  {
    LoadSettings(config, false);
    SaveSettings();

    PowerToysSettings::PowerToyValues values =
      PowerToysSettings::PowerToyValues::from_json_string(config);
    if (values.is_object_value(HOTKEY_NAME)) {
      mSettings.editorHotkey = PowerToysSettings::HotkeyObject::from_json(values.get_json(HOTKEY_NAME));
    }
    UnregisterHotKey(NULL, 1);
    RegisterHotKey(NULL, 1, mSettings.editorHotkey.get_modifiers(), mSettings.editorHotkey.get_code());
  }

  virtual void call_custom_action(const wchar_t* action) override
  {

  }

  virtual void enable()
  {
    mEnabled = true;
  }

  virtual void disable()
  {
    ResetAll();
    mEnabled = false;
  }

  virtual bool is_enabled() override
  {
    return mEnabled;
  }

  virtual intptr_t signal_event(const wchar_t* name, intptr_t data) override
  {
    if (mEnabled) {
      if (wcscmp(name, ll_keyboard) == 0) {
        return HandleKeyboardHookEvent(*(reinterpret_cast<LowlevelKeyboardEvent*>(data)));
      }
      else if (wcscmp(name, win_hook_event) == 0) {
        HandleWinHookEvent(*(reinterpret_cast<WinHookEvent*>(data)));
      }
    }
    return 0;
  }
};

intptr_t AlwaysOnTop::HandleKeyboardHookEvent(const LowlevelKeyboardEvent& data) noexcept
{
  if (data.wParam == WM_KEYDOWN) {
    MSG msg = { 0 };
    while (GetMessage(&msg, NULL, 0, 0) != 0)
    {
      bool isHotkey = false;
      if (msg.message == WM_HOTKEY) {
        ProcessCommand(GetForegroundWindow());
        isHotkey = true;
      }
      DispatchMessage(&msg);
      if (isHotkey) {
        break;
      }
    }
  }
  return 0;
}

void AlwaysOnTop::HandleWinHookEvent(const WinHookEvent& data) noexcept
{
  HWND window = GetForegroundWindow();
  switch (data.event) {
    case EVENT_SYSTEM_MENUSTART:
    {
      (void)InjectMenuItem(window);
      break;
    }
    case EVENT_OBJECT_INVOKED:
    {
      if (data.idChild == KAlwaysOnTopMenuID) {
        ProcessCommand(window);
      }
      break;
    }
  }
}

void AlwaysOnTop::ProcessCommand(HWND aWindow)
{
  bool alreadyOnTop = (mCurrentlyOnTop.first == aWindow);
  ResetCurrentOnTop();
  if (!alreadyOnTop) {
    (void)SetWindowOnTop(aWindow);
  }
}

bool AlwaysOnTop::SetWindowOnTop(HWND aWindow)
{
  if (SetWindowPos(aWindow, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE)) {
    mCurrentlyOnTop = std::make_pair(aWindow, GetSystemMenu(aWindow, false));
    (void)CheckMenuItem(mCurrentlyOnTop.second, KAlwaysOnTopMenuID, MF_BYCOMMAND | MF_CHECKED);
    return true;
  }
  return false;
}

void AlwaysOnTop::ResetCurrentOnTop()
{
  if (mCurrentlyOnTop.first &&
      SetWindowPos(mCurrentlyOnTop.first, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE)) {
    if (mCurrentlyOnTop.second) {
      (void)CheckMenuItem(mCurrentlyOnTop.second, KAlwaysOnTopMenuID, MF_BYCOMMAND | MF_UNCHECKED);
    }
  }
  mCurrentlyOnTop = { nullptr, nullptr };
}

bool AlwaysOnTop::InjectMenuItem(HWND aWindow)
{
  HMENU systemMenu = GetSystemMenu(aWindow, false);
  EnableMenuItem(systemMenu, KAlwaysOnTopMenuID, MF_BYCOMMAND | MF_ENABLED); // Some apps disables newly added menu items (e.g. telegram)
                                                                             // so re-enable 'AlwaysOnTop' every time system meny is opened
  if (systemMenu && (mModified.find(systemMenu) == mModified.end())) {
    MENUITEMINFO menuItem;
    menuItem.cbSize = sizeof(menuItem);
    menuItem.fMask = MIIM_ID | MIIM_STRING | MIIM_STATE;
    menuItem.fState = MF_UNCHECKED | MF_ENABLED;
    menuItem.wID = KAlwaysOnTopMenuID;
    menuItem.dwTypeData = KMenuItemString;
    menuItem.cch = KMenuItemStringSize;

    MENUITEMINFO separator;
    separator.cbSize = sizeof(separator);
    separator.fMask = MIIM_ID | MIIM_FTYPE;
    separator.fType = MFT_SEPARATOR;
    separator.wID = KAlwaysOnTopSeparatorID;

    InsertMenuItem(systemMenu, GetMenuItemCount(systemMenu) - 1, true, &separator);
    InsertMenuItem(systemMenu, GetMenuItemCount(systemMenu) - 2, true, &menuItem);
    mModified.insert(systemMenu);
    return true;
  }
  return false;
}

void AlwaysOnTop::ResetAll()
{
  ResetCurrentOnTop();
  for (const auto& menu : mModified) {
    DeleteMenu(menu, KAlwaysOnTopSeparatorID, MF_BYCOMMAND);
    DeleteMenu(menu, KAlwaysOnTopMenuID, MF_BYCOMMAND);
  }
  mModified.clear();
}

void AlwaysOnTop::LoadSettings(PCWSTR config, bool aFromFile)
{
  try {
    PowerToysSettings::PowerToyValues values = aFromFile ?
      PowerToysSettings::PowerToyValues::load_from_settings_file(get_name()) :
      PowerToysSettings::PowerToyValues::from_json_string(config);

    if (values.is_object_value(HOTKEY_NAME))
    {
      mSettings.editorHotkey = PowerToysSettings::HotkeyObject::from_json(values.get_json(HOTKEY_NAME));
    }
  }
  catch (std::exception & e) {}
}

void AlwaysOnTop::SaveSettings()
{
  PowerToysSettings::PowerToyValues values(get_name());
  values.add_property(HOTKEY_NAME, mSettings.editorHotkey);
  try {
    values.save_to_settings_file();
  }
  catch (std::exception & e) {}
}

void AlwaysOnTop::init_settings()
{

}

extern "C" __declspec(dllexport) PowertoyModuleIface* __cdecl powertoy_create()
{
  return new AlwaysOnTop();
}
