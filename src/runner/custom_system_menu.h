#pragma once

#include <windows.h>
#include <string>
#include <unordered_map>

class PowertoyModuleIface;

class CustomSystemMenuUtils
{
public:
  static bool InjectSepparator (PowertoyModuleIface* module, HWND aWindow, const int& aSeparatorId);
  static bool IncjectCustomItem(PowertoyModuleIface* module, HWND aWindow, const std::wstring& aItemName, const int& aItemId);
  static void DeleteCustomItem (HWND aWindow, const int& aItemId);
  static void ToggleItem       (HWND aWindow, const int& aItemId);
  static void CleanUp          (PowertoyModuleIface* module);

  static PowertoyModuleIface* GetModuleFromItemId  (const int& aItemId);
  static const std::wstring   GetItemNameFromItemid(const int& aItemId);
private:
  using CustomItemData = std::tuple<PowertoyModuleIface*, std::wstring>;

  static std::unordered_map<int, CustomItemData> CustomItemsPerModule;
};