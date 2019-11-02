#pragma once

#include <windows.h>
#include <string>
#include <unordered_map>

class PowertoyModuleIface;

class CustomSystemMenuUtils
{
public:
  static bool IncjectCustomItems(PowertoyModuleIface* module, HWND aWindow, std::vector<std::wstring> aItemNames);
  static void DeleteCustomItem (HWND aWindow, const int& aItemId);
  static void ToggleItem       (HWND aWindow, const int& aItemId);
  static void CleanUp          (PowertoyModuleIface* module);

  static PowertoyModuleIface* GetModuleFromItemId  (const int& aItemId);
  static const std::wstring   GetItemNameFromItemid(const int& aItemId);
private:
  static bool InjectSepparator (PowertoyModuleIface* module, HWND aWindow);
  static bool IncjectCustomItem(PowertoyModuleIface* module, HWND aWindow, const std::wstring& aItemName);

  static std::unordered_map<HWND, std::vector<PowertoyModuleIface*>> ProcessedWindows;
  static std::unordered_map<int, std::pair<PowertoyModuleIface*, std::wstring>> ItemInfo;
};