#include "pch.h"
#include "powertoys_events.h"
#include "lowlevel_keyboard_event.h"
#include "win_hook_event.h"
#include "system_menu_helper.h"

#include <common/settings_helpers.h>

void first_subscribed(const std::wstring& event) {
  if (event == ll_keyboard)
    start_lowlevel_keyboard_hook();
  else if (event == win_hook_event)
    start_win_hook_event();
}

void last_unsubscribed(const std::wstring& event) {
  if (event == ll_keyboard)
    stop_lowlevel_keyboard_hook();
  else if (event == win_hook_event)
    stop_win_hook_event();
}

PowertoysEvents& powertoys_events() {
  static PowertoysEvents powertoys_events;
  return powertoys_events;
}

void PowertoysEvents::register_receiver(const std::wstring & event, PowertoyModuleIface* module) {
  std::unique_lock lock(mutex);
  auto& subscribers = receivers[event];
  if (subscribers.empty()) {
    first_subscribed(event);
  }
  subscribers.push_back(module);
}

void PowertoysEvents::unregister_receiver(PowertoyModuleIface* module) {
  std::unique_lock lock(mutex);
  for (auto&[event, subscribers] : receivers) {
    subscribers.erase(remove(begin(subscribers), end(subscribers), module), end(subscribers));
    if (subscribers.empty()) {
      last_unsubscribed(event);
    }
  }
}

void PowertoysEvents::register_system_menu_action(PowertoyModuleIface* module)
{
  std::unique_lock lock(mutex);
  sysMenuActionModules.insert(module);
}

void PowertoysEvents::unregister_system_menu_action(PowertoyModuleIface* module)
{
  std::unique_lock lock(mutex);
  auto it = sysMenuActionModules.find(module);
  if (it != sysMenuActionModules.end()) {
    SystemMenuHelperInstace().Reset(module);
    sysMenuActionModules.erase(it);
  }
}

void PowertoysEvents::handle_system_menu_action(const WinHookEvent& data)
{
  if (data.event == EVENT_SYSTEM_MENUSTART) {
    for (auto& module : sysMenuActionModules) {
      SystemMenuHelperInstace().Customize(module, data.hwnd);
    }
  }
  else if (data.event == EVENT_OBJECT_INVOKED)
  {
    if (PowertoyModuleIface* module{ SystemMenuHelperInstace().ModuleFromItemId(data.idChild) }) {
      module->signal_system_menu_action(
        SystemMenuHelperInstace().ItemNameFromItemId(data.idChild).c_str());
      SystemMenuHelperInstace().HandleAction(GetForegroundWindow(), data.idChild);
    }
  }
}

intptr_t PowertoysEvents::signal_event(const std::wstring & event, intptr_t data) {
  intptr_t rvalue = 0;
  std::shared_lock lock(mutex);
  if (auto it = receivers.find(event); it != end(receivers)) {
    for (auto& module : it->second) {
      if (module)
        rvalue |= module->signal_event(event.c_str(), data);
    }
  }
  return rvalue;
}
