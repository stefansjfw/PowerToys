#include "pch.h"
#include "custom_system_menu.h"

#include <interface/powertoy_module_interface.h>

std::unordered_map<int, PowertoyModuleIface*> CustomSystemMenuUtils::CustomItemsPerModule{};

namespace {
  constexpr int KSeparatorPos = 1;
  constexpr int KNewItemPos   = 2;
  int GenerateItemId(PowertoyModuleIface* module) {
    return 0x00000001;
  }
}

bool CustomSystemMenuUtils::InjectSepparator(PowertoyModuleIface* module, HWND aWindow)
{
  HMENU systemMenu = GetSystemMenu(aWindow, false);
  if (systemMenu) {
    MENUITEMINFO separator;
    separator.cbSize = sizeof(separator);
    separator.fMask = MIIM_ID | MIIM_FTYPE;
    separator.fType = MFT_SEPARATOR;
    separator.wID = GenerateItemId(module); // Not random entirely, all items bound to same module MUST have same ID.

    if (InsertMenuItem(systemMenu, GetMenuItemCount(systemMenu) - KSeparatorPos, true, &separator)) {
      CustomItemsPerModule[separator.wID] = module;
      return true;
    }
  }
  return false;
}

bool CustomSystemMenuUtils::IncjectCustomItem(PowertoyModuleIface* module, HWND aWindow, const std::wstring& aItemName)
{
  HMENU systemMenu = GetSystemMenu(aWindow, false);
  if (systemMenu) {
    MENUITEMINFO menuItem;
    menuItem.cbSize = sizeof(menuItem);
    menuItem.fMask = MIIM_ID | MIIM_STRING | MIIM_STATE;
    menuItem.fState = MF_UNCHECKED | MF_ENABLED;
    menuItem.wID = GenerateItemId(module); // Not random entirely, all items bound to same module MUST have same ID.
    // menuItem.dwTypeData = &aItemName[0];
    menuItem.cch = aItemName.size() + 1;

    if (InsertMenuItem(systemMenu, GetMenuItemCount(systemMenu) - KNewItemPos, true, &menuItem)) {
      CustomItemsPerModule[menuItem.wID] = module;
      return true;
    }
  }
  return false;
}
void CustomSystemMenuUtils::DeleteCustomItem(HWND aWindow, const int& aItemId)
{
  HMENU systemMenu = GetSystemMenu(aWindow, false);
  if (systemMenu) {
    DeleteMenu(systemMenu, aItemId, MF_BYCOMMAND);
  }
}
void CustomSystemMenuUtils::ToggleItem(HWND aWindow, const int& aItemId)
{
  HMENU systemMenu = GetSystemMenu(aWindow, false);
  if (systemMenu) {
    // TODO: Toggle item state.
    // CheckMenuItem(systemMenu, aItemId, MF_BYCOMMAND | MF_CHECKED);
  }
}

void CustomSystemMenuUtils::CleanUp(PowertoyModuleIface* module)
{
  // TODO: Delete all custom menu items for specified window.
}

PowertoyModuleIface* CustomSystemMenuUtils::GetModuleFromItemId(const int& aItemId)
{
  return CustomItemsPerModule[aItemId];
}
