#include "pch.h"
#include "powertoys_events.h"
#include "lowlevel_keyboard_event.h"
#include "win_hook_event.h"
#include "custom_system_menu.h"

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

void PowertoysEvents::register_sys_menu_action_module(PowertoyModuleIface* module, const std::wstring& config)
{
  std::unique_lock lock(mutex);
  sysMenuActionModules[module] = config;
}

void PowertoysEvents::unregister_sys_menu_action_module(PowertoyModuleIface* module)
{
  std::unique_lock lock(mutex);
  auto it = sysMenuActionModules.find(module);
  if (it != sysMenuActionModules.end()) {
    // TODO: Clean all custom menu items allocated by this module!
    sysMenuActionModules.erase(it);
  }
}

void PowertoysEvents::handle_sys_menu_action(const WinHookEvent& data)
{
  using namespace web::json;
  if (data.event == EVENT_SYSTEM_MENUSTART) {
    for (auto& [module, config] : sysMenuActionModules) {
      try {
        value json_config = value::parse(config);
        array array_config = json_config.at(U("custom_items")).as_array();
        for (auto item : array_config)
        {
          auto itemId     = item[L"ID"].as_integer();
          auto itemName   = item[L"name"].as_string();
          std::wstring itemHotkey{};
          if (item.has_string_field(L"hotkey")) {
            itemHotkey = item[L"hotkey"].as_string();
            itemName += L"\t" + itemHotkey; // TODO: On hotkey change, update menu item shortcut!
          }
          CustomSystemMenuUtils::IncjectCustomItem(module, data.hwnd, itemName, itemId);
        }
      }
      catch (std::exception&) {}
    }
  }
  else if (data.event == EVENT_OBJECT_INVOKED)
  {
    PowertoyModuleIface* module = CustomSystemMenuUtils::GetModuleFromItemId(data.idChild);
    if (module) {
      module->handle_custom_system_menu_action(
        CustomSystemMenuUtils::GetItemNameFromItemid(data.idChild).c_str());
      CustomSystemMenuUtils::ToggleItem(data.hwnd, data.idChild);
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
