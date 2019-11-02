#pragma once

#include <windows.h>
#include <string>
#include <unordered_map>

class PowertoyModuleIface;

class CustomSystemMenuUtils
{
public:
  static bool InjectSepparator (PowertoyModuleIface* module, HWND aWindow);
  static bool IncjectCustomItem(PowertoyModuleIface* module, HWND aWindow, const std::wstring& aItemName);
  static void DeleteCustomItem (HWND aWindow, const int& aItemId);
  static void ToggleItem       (HWND aWindow, const int& aItemId);
  static void CleanUp          (PowertoyModuleIface* module);

  static PowertoyModuleIface* GetModuleFromItemId  (const int& aItemId);
  static std::wstring         GetItemNameFromItemId(const int& aItemId);
private:
  using CustomItemInfo = std::tuple<PowertoyModuleIface*, std::wstring>;

  static std::unordered_map<int, CustomItemInfo> CustomItemsPerModule;
};