#include "pch.h"
#include "JsonHelpers.h"

#include <shlwapi.h>
#include <filesystem>
#include <fstream>

namespace JSONHelpers {
  FancyZonesData& FancyZonesDataInstance() {
    static FancyZonesData instance;
    return instance;
  }

  FancyZonesData::FancyZonesData() {
    std::wstring result = PTSettingsHelper::get_module_save_folder_location(L"FancyZones");
    jsonFilePath = result + L"\\" + std::wstring(FANCY_ZONES_DATA_FILE);
  }


  const std::wstring& FancyZonesData::GetPersistFancyZonesJSONPath() {
    return jsonFilePath;
  }

  web::json::value FancyZonesData::GetPersistFancyZonesJSON() {
    std::wstring save_file_location = GetPersistFancyZonesJSONPath();

    std::ifstream save_file(save_file_location, std::ios::binary);
    web::json::value result = web::json::value::parse(save_file);
    save_file.close();
    return result;
  }
  bool FancyZonesData::GetAppLastZone(HWND window, PCWSTR appPath, _Out_ PINT iZoneIndex) {
    *iZoneIndex = -1;

    if (auto monitor = MonitorFromWindow(window, MONITOR_DEFAULTTONULL))
    {
      TAppPath path{ appPath };
      if (appZoneHistoryMap.contains(path)) {
        *iZoneIndex = appZoneHistoryMap[path].zoneIndex;
        return true;
      }
    }
    return false;
  }

  // Pass -1 for the zoneIndex to delete the entry from the map
  bool FancyZonesData::SetAppLastZone(HWND window, PCWSTR appPath, DWORD zoneIndex) {
    if (auto monitor = MonitorFromWindow(window, MONITOR_DEFAULTTONULL))
    {
      if (zoneIndex == -1) {
        appZoneHistoryMap.erase(TAppPath{ appPath });
      }
      else {
        appZoneHistoryMap[TAppPath{ appPath }] = AppZoneHistoryData{ L"NESTO_NESTO", static_cast<int>(zoneIndex) }; //TODO(stefan)
      }
      return true;
    }
    return false;
  }

  void FancyZonesData::SetActiveZoneSet(const std::wstring& unique_id, const std::wstring& uuid /*TODO(stefan): OVO TREBA DA SE ZAMENI SA DEVICE ID*/) {
    if (!uuid.empty()) {
      activeZoneSetMap[unique_id] = ActiveZoneSetData{ uuid, false, 0 }; //TODO(stefan)
    }
  }

  UUID FancyZonesData::GetActiveZoneSet(const std::wstring& unique_id) {
    if (activeZoneSetMap.contains(unique_id)) {
      const WCHAR* uuidStr = activeZoneSetMap[unique_id].zoneSetUuid.c_str();
      GUID uuid{};

      CLSIDFromString(uuidStr, &uuid);
      return uuid;
    }
    return GUID_NULL;
  }


  void FancyZonesData::ParseAppliedZoneSets(const web::json::value& fancyZonesDataJSON) {
    if (fancyZonesDataJSON.has_field(L"applied-zone-sets") && !fancyZonesDataJSON.at(U("applied-zone-sets")).is_null()) {
      web::json::array appliedZoneSets = fancyZonesDataJSON.at(U("applied-zone-sets")).as_array();

      for (const auto& appliedZoneSet : appliedZoneSets) {
        const auto& zoneSet = AppliedZoneSetFromJson(appliedZoneSet);
        appliedZoneSetMap[zoneSet.uuid] = zoneSet.data;
      }
    }
  }

  web::json::value FancyZonesData::SerializeAppliedZoneSets() {
    web::json::value appliedZoneSets{};

    int i = 0;
    for (const auto& zoneSet : appliedZoneSetMap) {
      //appliedZoneSets[i++] = AppliedZoneSetToJson(AppliedZoneSetJSON{ zoneSet.first, zoneSet.second });
    }

    return appliedZoneSets;
  }

  void FancyZonesData::ParseAppZoneHistory(const web::json::value& fancyZonesDataJSON) {
    if (fancyZonesDataJSON.has_field(L"app-zone-history") && !fancyZonesDataJSON.at(U("app-zone-history")).is_null()) {
      web::json::array appLastZones = fancyZonesDataJSON.at(U("app-zone-history")).as_array();

      for (const auto& appLastZone : appLastZones) {
        const auto& appZoneHistory = AppZoneHistoryFromJson(appLastZone);
        appZoneHistoryMap[appZoneHistory.appPath] = appZoneHistory.data;
      }
    }
  }

  web::json::value FancyZonesData::SerializeAppZoneHistory() {
    web::json::value appHistoryArray;

    int i = 0;
    for (const auto& app : appZoneHistoryMap) {
      appHistoryArray[i++] = AppZoneHistoryToJson(AppZoneHistoryJSON{ app.first, app.second });
    }

    return appHistoryArray;
  }

  void FancyZonesData::ParseActiveZoneSets(const web::json::value& fancyZonesDataJSON) {
    if (fancyZonesDataJSON.has_field(L"active-zone-sets") && !fancyZonesDataJSON.at(U("active-zone-sets")).is_null()) {
      web::json::array activeZoneSets = fancyZonesDataJSON.at(U("active-zone-sets")).as_array();

      for (const auto& activeZoneSet : activeZoneSets) {
        const auto& zoneSet = ActiveZoneSetFromJson(activeZoneSet);
        activeZoneSetMap[zoneSet.deviceId] = zoneSet.data;
      }
    }
  }

  web::json::value FancyZonesData::SerializeActiveZoneSets() {
    web::json::value activeZoneSetsJSON{};

    int i = 0;
    for (const auto& zoneSet : activeZoneSetMap) {
      activeZoneSetsJSON[i++] = ActiveZoneSetToJson(ActiveZoneSetJSON{ zoneSet.first, zoneSet.second });
    }

    return activeZoneSetsJSON;
  }


  void FancyZonesData::LoadFancyZonesData()
  {
    std::wstring jsonFilePath = GetPersistFancyZonesJSONPath();

    if (!std::filesystem::exists(jsonFilePath)) {

      MigrateAppZoneHistoryFromRegistry();
      MigrateActiveZoneSetsFromRegistry();

      SaveFancyZonesData();
    }
    else {
      web::json::value fancyZonesDataJSON = GetPersistFancyZonesJSON();

      ParseAppZoneHistory(fancyZonesDataJSON);
      ParseActiveZoneSets(fancyZonesDataJSON);
      ParseAppliedZoneSets(fancyZonesDataJSON);

    }
  }

  void FancyZonesData::SaveFancyZonesData() {
    std::ofstream jsonFile{ jsonFilePath };

    web::json::value root{};

    root[L"app-zone-history"] = SerializeAppZoneHistory();
    root[L"active-zone-sets"] = SerializeActiveZoneSets();

    root.serialize(jsonFile);

    jsonFile.close();
  }

  void FancyZonesData::MigrateAppZoneHistoryFromRegistry() {
    HMONITOR buffer[10];
    Monitors monitors;
    monitors.data = buffer;
    monitors.count = 0;
    EnumDisplayMonitors(NULL, NULL, &MyInfoEnumProc, reinterpret_cast<LPARAM>(&monitors));

    for (int i = 0; i < monitors.count; i++) {
      HMONITOR monitor = monitors.data[i];
      wchar_t key[256];
      StringCchPrintf(key, ARRAYSIZE(key), L"%s\\%s\\%x", RegistryHelpers::REG_SETTINGS, RegistryHelpers::APP_ZONE_HISTORY_SUBKEY, monitor);
      HKEY hkey;
      if (RegOpenKeyExW(HKEY_CURRENT_USER, key, 0, KEY_ALL_ACCESS, &hkey) == ERROR_SUCCESS) {
        DWORD zoneIndex;
        DWORD dataSize = sizeof(DWORD);
        wchar_t value[256]{};
        DWORD valueLength = ARRAYSIZE(value);
        DWORD i = 0;
        while (RegEnumValueW(hkey, i++, value, &valueLength, nullptr, nullptr, reinterpret_cast<BYTE*>(&zoneIndex), &dataSize) == ERROR_SUCCESS)
        {
          appZoneHistoryMap[std::wstring{ value }] = AppZoneHistoryData{ L"NEST_NEST", static_cast<int>(zoneIndex) };

          valueLength = ARRAYSIZE(value);
          dataSize = sizeof(zoneIndex);
        }
      }
    }
  }

  void FancyZonesData::MigrateActiveZoneSetsFromRegistry() {
    wchar_t key[256];
    StringCchPrintf(key, ARRAYSIZE(key), L"%s", RegistryHelpers::REG_SETTINGS);
    HKEY hkey;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, key, 0, KEY_ALL_ACCESS, &hkey) == ERROR_SUCCESS) {
      wchar_t value[256]{};
      DWORD valueLength = ARRAYSIZE(value);
      DWORD i = 0;
      while (RegEnumKeyW(hkey, i++, value, valueLength) == ERROR_SUCCESS)
      {
        std::wstring uniqueID{ value };
        if (uniqueID.find(L"0000-0000-0000") != std::wstring::npos) { //TODO(stefan): This is ugly!! Hope Andrey will resolve this with deviceID
          wchar_t activeZoneSetId[256];
          DWORD bufferSize = sizeof(activeZoneSetId);
          DWORD showSpacing;
          DWORD spacing;
          DWORD size = sizeof(DWORD);

          wchar_t key[256]{};
          StringCchPrintf(key, ARRAYSIZE(key), L"%s\\%s", RegistryHelpers::REG_SETTINGS, value);
          if (SHRegGetUSValueW(key, L"ActiveZoneSetId", nullptr, &activeZoneSetId, &bufferSize, FALSE, nullptr, 0) != ERROR_SUCCESS) {
            activeZoneSetId[0] = '\0';
          }
          if (SHRegGetUSValueW(key, L"ShowSpacing", nullptr, &showSpacing, &size, FALSE, nullptr, 0) != ERROR_SUCCESS) {
            showSpacing = 0;
          }
          if (SHRegGetUSValueW(key, L"Spacing", nullptr, &spacing, &size, FALSE, nullptr, 0) != ERROR_SUCCESS) {
            spacing = 0;
          }

          activeZoneSetMap[uniqueID] = ActiveZoneSetData{ std::wstring { activeZoneSetId }, static_cast<bool>(showSpacing), static_cast<int>(spacing) };

          valueLength = ARRAYSIZE(value);
        }
      }
    }
  }

  web::json::value FancyZonesData::AppliedZoneSetToJson(const AppliedZoneSetJSON& zoneSet) {
    web::json::value result = web::json::value::object();

    result[L"uuid"] = web::json::value::string(zoneSet.uuid);
    result[L"name"] = web::json::value::string(zoneSet.data.name);
    result[L"type"] = web::json::value::number(static_cast<int>(zoneSet.data.type));
    if (zoneSet.data.type == ZoneSetLayoutType::Custom) {
      result[L"info"] = web::json::value::string(std::get<TZoneUUID>(zoneSet.data.info));
    }
    else {
      result[L"info"] = web::json::value::number(std::get<TZoneCount>(zoneSet.data.info));
    }

    return result;
  }

  AppliedZoneSetJSON FancyZonesData::AppliedZoneSetFromJson(web::json::value zoneSet) {
    AppliedZoneSetJSON result;

    result.uuid = zoneSet[L"uuid"].as_string();
    result.data.name = zoneSet[L"name"].as_string();
    result.data.type = static_cast<ZoneSetLayoutType>(zoneSet[L"type"].as_integer());
    if (result.data.type == ZoneSetLayoutType::Custom) {
      result.data.info = zoneSet[L"info"].as_string();
    }
    else {
      result.data.info = zoneSet[L"info"].as_integer();
    }

    return result;
  }

  web::json::value FancyZonesData::AppZoneHistoryToJson(const AppZoneHistoryJSON& appZoneHistory) {
    web::json::value result = web::json::value::object();

    result[L"app-path"] = web::json::value::string(appZoneHistory.appPath);
    result[L"zoneset-uuid"] = web::json::value::string(appZoneHistory.data.zoneSetUuid);
    result[L"zone-index"] = web::json::value::number(appZoneHistory.data.zoneIndex);

    return result;
  }

  AppZoneHistoryJSON FancyZonesData::AppZoneHistoryFromJson(web::json::value zoneSet) {
    AppZoneHistoryJSON result;

    result.appPath = zoneSet[L"app-path"].as_string();
    result.data.zoneSetUuid = zoneSet[L"zoneset-uuid"].as_string();
    result.data.zoneIndex = zoneSet[L"zone-index"].as_integer();

    return result;
  }

  web::json::value FancyZonesData::ActiveZoneSetToJson(const ActiveZoneSetJSON& zoneSet) {
    web::json::value result = web::json::value::object();

    result[L"device-id"] = web::json::value::string(zoneSet.deviceId);
    result[L"zoneset-uuid"] = web::json::value::string(zoneSet.data.zoneSetUuid);
    result[L"show-spacing"] = web::json::value::boolean(zoneSet.data.showSpacing);
    result[L"spacing"] = web::json::value::number(zoneSet.data.spacing);

    return result;
  }

  ActiveZoneSetJSON FancyZonesData::ActiveZoneSetFromJson(web::json::value zoneSet) {
    ActiveZoneSetJSON result;

    result.deviceId = zoneSet[L"device-id"].as_string();
    result.data.zoneSetUuid = zoneSet[L"zoneset-uuid"].as_string();
    result.data.showSpacing = zoneSet[L"show-spacing"].as_bool();
    result.data.spacing = zoneSet[L"spacing"].as_integer();

    return result;
  }

}