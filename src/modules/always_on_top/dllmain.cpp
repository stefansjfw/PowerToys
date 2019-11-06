#include "pch.h"
#include <interface/powertoy_module_interface.h>
#include <interface/lowlevel_keyboard_event_data.h>
#include <interface/powertoy_system_menu.h>
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
  const std::wstring KMenuItemName = L"Always on top";

  std::wstring AlwaysOnTopConfig(const std::wstring& name)
  {
    web::json::value customItem;
    customItem[L"name"]   = web::json::value::string(name);
    customItem[L"enable"] = web::json::value::boolean(true);
    customItem[L"check"]  = web::json::value::boolean(true);

    web::json::value customItems;
    customItems[0] = customItem;

    web::json::value root;
    root[L"custom_items"] = customItems;

    return root.serialize();
  }
}

class AlwaysOnTop : public PowertoyModuleIface {
private:

  bool                     mEnabled{ false };
  HWND                     mCurrentlyOnTop{ nullptr };
  AllwaysOnTopSettings     mSettings;
  std::wstring             mItemName{ KMenuItemName };
  PowertoySystemMenuIface* mSystemMenuHelper{ nullptr };

  void init_settings();

  intptr_t HandleKeyboardHookEvent(const LowlevelKeyboardEvent& data) noexcept;

  void ProcessCommand(HWND aWindow);
  bool SetWindowOnTop(HWND aWindow);
  void ResetCurrentOnTop();

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
    ResetCurrentOnTop();
    delete this;
  }

  virtual const wchar_t* get_name() override
  {
    return MODULE_NAME;
  }

  virtual const wchar_t** get_events() override
  {
    static const wchar_t* events[] = { ll_keyboard,
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

    // Perform update with new hotkey.
    mSystemMenuHelper->SetConfiguration(this, AlwaysOnTopConfig(mItemName).c_str());
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
    ResetCurrentOnTop();
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
    }
    return 0;
  }

  virtual void register_system_menu_helper(PowertoySystemMenuIface* helper) override
  {
    mSystemMenuHelper = helper;

    // Initial configuration with default hotkey.
    mSystemMenuHelper->SetConfiguration(this, AlwaysOnTopConfig(mItemName).c_str());
  }

  virtual void signal_system_menu_action(const wchar_t* name) override
  {
    if (!wcscmp(name, mItemName.c_str())) {
      ProcessCommand(GetForegroundWindow());
    }
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

void AlwaysOnTop::ProcessCommand(HWND aWindow)
{
  bool alreadyOnTop = (mCurrentlyOnTop == aWindow);
  ResetCurrentOnTop();
  if (!alreadyOnTop) {
    (void)SetWindowOnTop(aWindow);
  }
}

bool AlwaysOnTop::SetWindowOnTop(HWND aWindow)
{
  if (SetWindowPos(aWindow, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE)) {
    mCurrentlyOnTop = aWindow;
    return true;
  }
  return false;
}

void AlwaysOnTop::ResetCurrentOnTop()
{
  if (mCurrentlyOnTop &&
      SetWindowPos(mCurrentlyOnTop, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE)) {
  }
  mCurrentlyOnTop = nullptr;
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
      mItemName = KMenuItemName + L"\t" + mSettings.editorHotkey.to_string();
    }
  }
  catch (std::exception&) {}
}

void AlwaysOnTop::SaveSettings()
{
  PowerToysSettings::PowerToyValues values(get_name());
  values.add_property(HOTKEY_NAME, mSettings.editorHotkey);
  try {
    values.save_to_settings_file();
  }
  catch (std::exception&) {}
}

void AlwaysOnTop::init_settings()
{

}

extern "C" __declspec(dllexport) PowertoyModuleIface* __cdecl powertoy_create()
{
  return new AlwaysOnTop();
}
