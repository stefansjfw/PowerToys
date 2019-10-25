#include "pch.h"
#include <interface/powertoy_module_interface.h>
#include <interface/lowlevel_keyboard_event_data.h>
#include <interface/lowlevel_mouse_event_data.h>
#include <interface/win_hook_event_data.h>
#include <common/settings_objects.h>
#include <set>
#include "trace.h"

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

// The PowerToy name that will be shown in the settings.
const static wchar_t* MODULE_NAME = L"AlwaysOnTop";
// Add a description that will we shown in the module settings page.
const static wchar_t* MODULE_DESC = L"<no description>";

// These are the properties shown in the Settings page.
struct AllwaysOnTopSettings {
  int test_int_prop = 10;
} g_settings;

namespace {
constexpr int KAlwaysOnTopSeparatorID = 0x1234;
constexpr int KAlwaysOnTopMenuID      = 0x5678;
}

class AlwaysOnTop : public PowertoyModuleIface {
private:

  bool                   mEnabled{ false };
  std::pair<HWND, HMENU> mCurrentlyOnTop{ nullptr, nullptr };
  std::set<HMENU>        mModified;

  void init_settings();

  intptr_t HandleKeyboardHookEvent(const LowlevelKeyboardEvent& data) noexcept;
  void HandleWinHookEvent(const WinHookEvent& data) noexcept;
  void HandleMouseHookEvent(const LowlevelMouseEvent& data) noexcept;

  bool SetWindowOnTop(HWND aWindow);
  void ResetCurrentOnTop();
  bool InjectMenuItem(HWND aWindow);
  void ResetAll();

public:
  AlwaysOnTop() {
    init_settings();
  }

  virtual void destroy() override {
    ResetAll();
    delete this;
  }

  virtual const wchar_t* get_name() override {
    return MODULE_NAME;
  }

  virtual const wchar_t** get_events() override {
    static const wchar_t* events[] = { ll_keyboard,
                                       ll_mouse,
                                       win_hook_event,
                                       nullptr };

    return events;
  }

  virtual bool get_config(wchar_t* buffer, int* buffer_size) override {
    HINSTANCE hinstance = reinterpret_cast<HINSTANCE>(&__ImageBase);

    // Create a Settings object.
    PowerToysSettings::Settings settings(hinstance, get_name());
    return settings.serialize_to_buffer(buffer, buffer_size);
  }

  virtual void set_config(const wchar_t* config) override {

  }

  virtual void call_custom_action(const wchar_t* action) override {

  }

  virtual void enable() {
    mEnabled = true;
  }

  virtual void disable() {
    ResetAll();
    mEnabled = false;
  }

  virtual bool is_enabled() override {
    return mEnabled;
  }

  // Handle incoming event, data is event-specific
  virtual intptr_t signal_event(const wchar_t* name, intptr_t data) override {
  if (mEnabled) {
    if (wcscmp(name, ll_keyboard) == 0)
    {
      return HandleKeyboardHookEvent(*(reinterpret_cast<LowlevelKeyboardEvent*>(data)));
    }
    else if (wcscmp(name, ll_mouse) == 0)
    {
      HandleMouseHookEvent(*(reinterpret_cast<LowlevelMouseEvent*>(data)));
    }
    else if (wcscmp(name, win_hook_event) == 0)
    {
      HandleWinHookEvent(*(reinterpret_cast<WinHookEvent*>(data)));
    }
  }
    return 0;
  }
};

intptr_t AlwaysOnTop::HandleKeyboardHookEvent(const LowlevelKeyboardEvent& data) noexcept
{
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
      bool alreadyOnTop = (mCurrentlyOnTop.first == window);
      ResetCurrentOnTop();
      if (!alreadyOnTop) {
        (void)SetWindowOnTop(window);
      }
    }
    break;
  }
  }
}

void AlwaysOnTop::HandleMouseHookEvent(const LowlevelMouseEvent& data) noexcept
{

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
      SetWindowPos(mCurrentlyOnTop.first, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE))
  {
    if (mCurrentlyOnTop.second) {
      (void)CheckMenuItem(mCurrentlyOnTop.second, KAlwaysOnTopMenuID, MF_BYCOMMAND | MF_UNCHECKED);
    }
  }
  mCurrentlyOnTop = { nullptr, nullptr };
}

bool AlwaysOnTop::InjectMenuItem(HWND aWindow)
{
  HMENU systemMenu = GetSystemMenu(aWindow, false);
  if (systemMenu && (mModified.find(systemMenu) == mModified.end())) {
    AppendMenu(systemMenu, MF_SEPARATOR, KAlwaysOnTopSeparatorID, {});
    AppendMenu(systemMenu, MF_BYCOMMAND | MF_UNCHECKED, KAlwaysOnTopMenuID, L"AlwaysOnTop");
    mModified.insert(systemMenu);
    return true;
  }
  return false;
}

void AlwaysOnTop::ResetAll() {
  ResetCurrentOnTop();
  for (const auto& menu : mModified) {
    DeleteMenu(menu, KAlwaysOnTopSeparatorID, MF_BYCOMMAND);
    DeleteMenu(menu, KAlwaysOnTopMenuID, MF_BYCOMMAND);
  }
  mModified.clear();
}

void AlwaysOnTop::init_settings() {
  try {
    // Load and parse the settings file for this PowerToy.
    PowerToysSettings::PowerToyValues settings =
      PowerToysSettings::PowerToyValues::load_from_settings_file(AlwaysOnTop::get_name());

    // Load the int property.
    if (settings.is_int_value(L"test_int_spinner")) {
      g_settings.test_int_prop = settings.get_int_value(L"test_int_spinner");
    }
  }
  catch (std::exception & ex) {
    // Error while loading from the settings file. Let default values stay as they are.
  }
}

extern "C" __declspec(dllexport) PowertoyModuleIface* __cdecl powertoy_create() {
  return new AlwaysOnTop();
}
