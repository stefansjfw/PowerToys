#include "pch.h"
#include "lowlevel_mouse_event.h"
#include "powertoys_events.h"

namespace {
  HHOOK hook_handle = nullptr;
  HHOOK hook_handle_copy = nullptr; // Make sure we do use nullptr in CallNextHookEx call.

  LRESULT CALLBACK hook_proc(int nCode, WPARAM wParam, LPARAM lParam) {
    LowlevelMouseEvent event;
    if (nCode == HC_ACTION) {
      event.lParam = reinterpret_cast<MSLLHOOKSTRUCT*>(lParam);
      event.wParam = wParam;
      if (powertoys_events().signal_event(ll_mouse, reinterpret_cast<intptr_t>(&event)) != 0) {
        return 1;
      }
    }
    return CallNextHookEx(hook_handle_copy, nCode, wParam, lParam);
  }
}

void start_lowlevel_mouse_hook() {
  if (!hook_handle) {
    hook_handle = SetWindowsHookEx(WH_MOUSE_LL, hook_proc, GetModuleHandle(NULL), NULL);
    hook_handle_copy = hook_handle;
    if (!hook_handle) {
      throw std::runtime_error("Cannot install mouse listener.");
    }
  }
}

void stop_lowlevel_mouse_hook() {
  if (hook_handle) {
    UnhookWindowsHookEx(hook_handle);
    hook_handle = nullptr;
  }
}
