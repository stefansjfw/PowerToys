#pragma once
#include <interface/powertoy_module_interface.h>
#include <interface/win_hook_event_data.h>
#include <string>

class PowertoysEvents {
public:
  void register_receiver(const std::wstring& event, PowertoyModuleIface* module);
  void unregister_receiver(PowertoyModuleIface* module);

  void register_sys_menu_action_module  (PowertoyModuleIface* module,
                                         const std::wstring& config,
                                         sysMenuActionCallback callback);
  void unregister_sys_menu_action_module(PowertoyModuleIface* module);
  void handle_sys_menu_action           (const WinHookEvent& data);

  intptr_t signal_event(const std::wstring& event, intptr_t data);
private:
  struct SysMenuActionData {
    std::wstring config;
    sysMenuActionCallback func;
  };

  std::shared_mutex mutex;
  std::unordered_map<std::wstring, std::vector<PowertoyModuleIface*>> receivers;
  std::unordered_map<PowertoyModuleIface*, SysMenuActionData> sysMenuActionModules;
};

PowertoysEvents& powertoys_events();

void first_subscribed(const std::wstring& event);
void last_unsubscribed(const std::wstring& event);

