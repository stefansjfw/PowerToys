#include "pch.h"
#include "custom_system_menu.h"

#include <interface/powertoy_module_interface.h>

std::unordered_map<HWND, std::vector<PowertoyModuleIface*>> CustomSystemMenuUtils::ProcessedWindows{};
std::unordered_map<int, std::pair<PowertoyModuleIface*, std::wstring>> CustomSystemMenuUtils::ItemInfo{};

namespace {
  constexpr int KSeparatorPos = 1;
  constexpr int KNewItemPos   = 2;
  int GenerateItemId() {
    static int generator = 0xDEADBEEF;
    return ++generator;
  }
}

bool CustomSystemMenuUtils::IncjectCustomItems(PowertoyModuleIface* module, HWND aWindow, std::vector<std::wstring> aItemNames)
{
  for (auto& [id, info] : ItemInfo) {
    EnableMenuItem(GetSystemMenu(aWindow, false), id, MF_BYCOMMAND | MF_ENABLED);
    // Some apps disables newly added menu items (e.g. telegram), so re-enable custom menus every time system meny is opened
  }

  for (const auto& m : ProcessedWindows[aWindow]) {
    if (module == m) {
      return false;
    }
  }
  InjectSepparator(module, aWindow);
  for (const auto& name : aItemNames) {
    IncjectCustomItem(module, aWindow, name);
  }
  ProcessedWindows[aWindow].push_back(module);
  return true;
}

void CustomSystemMenuUtils::ToggleItem(HWND aWindow, const int& aItemId)
{
  HMENU systemMenu = GetSystemMenu(aWindow, false);
  if (systemMenu) {
    int state = (GetMenuState(systemMenu, aItemId, MF_BYCOMMAND) == MF_CHECKED) ? MF_UNCHECKED : MF_CHECKED;
    CheckMenuItem(systemMenu, aItemId, MF_BYCOMMAND | state);
  }
}

void CustomSystemMenuUtils::CleanUp(PowertoyModuleIface* module)
{
  for (auto& [window, modules] : ProcessedWindows) {
    for (auto& [id, info] : ItemInfo) {
      HMENU sysMenu{ nullptr };
      if (info.first == module && (sysMenu = GetSystemMenu(window, false))) {
        DeleteMenu(GetSystemMenu(window, false), id, MF_BYCOMMAND);
      }
    }
  }
}

PowertoyModuleIface* CustomSystemMenuUtils::GetModuleFromItemId(const int& aItemId)
{
  auto itemIt = ItemInfo.find(aItemId);
  if (itemIt != ItemInfo.end()) {
    return itemIt->second.first;
  }
  return nullptr;
}

const std::wstring CustomSystemMenuUtils::GetItemNameFromItemid(const int& aItemId)
{
  auto itemIt = ItemInfo.find(aItemId);
  if (itemIt != ItemInfo.end()) {
    return itemIt->second.second;
  }
  return std::wstring{};
}

bool CustomSystemMenuUtils::InjectSepparator(PowertoyModuleIface* module, HWND aWindow)
{
  HMENU systemMenu = GetSystemMenu(aWindow, false);
  if (systemMenu) {
    MENUITEMINFO separator;
    separator.cbSize = sizeof(separator);
    separator.fMask = MIIM_ID | MIIM_FTYPE;
    separator.fType = MFT_SEPARATOR;
    separator.wID = GenerateItemId();

    if (InsertMenuItem(systemMenu, GetMenuItemCount(systemMenu) - KSeparatorPos, true, &separator)) {
      ItemInfo[separator.wID] = { module, L"sepparator_dummy_name" };
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
    menuItem.wID = GenerateItemId();
    menuItem.dwTypeData = const_cast<WCHAR*>(aItemName.c_str());
    menuItem.cch = aItemName.size() + 1;

    if (InsertMenuItem(systemMenu, GetMenuItemCount(systemMenu) - KNewItemPos, true, &menuItem)) {
      ItemInfo[menuItem.wID] = { module, aItemName };
      return true;
    }
  }
  return false;
}