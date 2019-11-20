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


  const std::wstring& FancyZonesData::GetPersistFancyZonesJSONPath() const {
    return jsonFilePath;
  }

  web::json::value FancyZonesData::GetPersistFancyZonesJSON() {
    std::wstring save_file_location = GetPersistFancyZonesJSONPath();

    std::ifstream save_file(save_file_location, std::ios::binary);
    web::json::value result = web::json::value::parse(save_file);
    save_file.close();
    return result;
  }

  bool FancyZonesData::GetAppLastZone(HWND window, PCWSTR appPath, _Out_ PINT iZoneIndex) const {
    *iZoneIndex = -1;

    if (auto monitor = MonitorFromWindow(window, MONITOR_DEFAULTTONULL))
    {
      TAppPath path{ appPath };
      if (appZoneHistoryMap.contains(path)) {
        *iZoneIndex = appZoneHistoryMap.at(path).zoneIndex;
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
      deviceInfoMap[unique_id].activeZoneSetUuid = uuid; //TODO(stefan)
    }
  }

  void FancyZonesData::SetDeviceInfoToTmpFile(const std::wstring& unique_id, const std::wstring& uuid, const std::wstring& tmpFilePath) {
    std::ofstream tmpFile;
    tmpFile.open(tmpFilePath);
    web::json::value zoneSetJson = DeviceInfoToJson(DeviceInfoJSON{ unique_id, DeviceInfoData{ uuid } });
    zoneSetJson.serialize(tmpFile);
    tmpFile.close();
  }


  UUID FancyZonesData::GetActiveZoneSet(const std::wstring& unique_id, const std::wstring& tmpFilePath) {
    if (std::filesystem::exists(tmpFilePath)) {
      std::ifstream tmpFile(tmpFilePath, std::ios::binary);
      web::json::value zoneSetJson = web::json::value::parse(tmpFile);
      DeviceInfoJSON deviceInfo = DeviceInfoFromJson(zoneSetJson);
      deviceInfoMap[unique_id].activeZoneSetUuid = deviceInfo.data.activeZoneSetUuid;

      tmpFile.close();
      DeleteFileW(tmpFilePath.c_str());
    }

    if (deviceInfoMap.contains(unique_id)) {
      const WCHAR* uuidStr = deviceInfoMap[unique_id].activeZoneSetUuid.c_str();
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

  web::json::value FancyZonesData::SerializeAppliedZoneSets() const {
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

  web::json::value FancyZonesData::SerializeAppZoneHistory() const {
    web::json::value appHistoryArray;

    int i = 0;
    for (const auto& app : appZoneHistoryMap) {
      appHistoryArray[i++] = AppZoneHistoryToJson(AppZoneHistoryJSON{ app.first, app.second });
    }

    return appHistoryArray;
  }

  void FancyZonesData::ParseDeviceInfos(const web::json::value& fancyZonesDataJSON) {
    if (fancyZonesDataJSON.has_field(L"devices") && !fancyZonesDataJSON.at(U("devices")).is_null()) {
      web::json::array devices = fancyZonesDataJSON.at(U("devices")).as_array();

      for (const auto& deviceJson : devices) {
        const auto& device = DeviceInfoFromJson(deviceJson);
        deviceInfoMap[device.deviceId] = device.data;
      }
    }
  }

  web::json::value FancyZonesData::SerializeDeviceInfos() const {
    web::json::value DeviceInfosJSON{};

    int i = 0;
    for (const auto& device : deviceInfoMap) {
      DeviceInfosJSON[i++] = DeviceInfoToJson(DeviceInfoJSON{ device.first, device.second });
    }

    return DeviceInfosJSON;
  }


  void FancyZonesData::LoadFancyZonesData()
  {
    std::wstring jsonFilePath = GetPersistFancyZonesJSONPath();

    if (!std::filesystem::exists(jsonFilePath)) {

      MigrateAppZoneHistoryFromRegistry();
      MigrateDeviceInfoFromRegistry();

      SaveFancyZonesData();
    }
    else {
      web::json::value fancyZonesDataJSON = GetPersistFancyZonesJSON();

      ParseAppZoneHistory(fancyZonesDataJSON);
      ParseDeviceInfos(fancyZonesDataJSON);
      ParseAppliedZoneSets(fancyZonesDataJSON);

    }
  }

  void FancyZonesData::SaveFancyZonesData() const {
    std::ofstream jsonFile{ jsonFilePath };

    web::json::value root{};

    root[L"app-zone-history"] = SerializeAppZoneHistory();
    root[L"devices"] = SerializeDeviceInfos();

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
          appZoneHistoryMap[std::wstring{ value }] = AppZoneHistoryData{ L"NEST_NEST", static_cast<int>(zoneIndex) }; //TODO(stefan)

          valueLength = ARRAYSIZE(value);
          dataSize = sizeof(zoneIndex);
        }
      }
    }
  }

  void FancyZonesData::MigrateDeviceInfoFromRegistry() {
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
          activeZoneSetId[0] = '\0';
          DWORD bufferSize = sizeof(activeZoneSetId);
          DWORD showSpacing = 1;
          DWORD spacing = 16;
          DWORD zoneCount = 3;
          DWORD size = sizeof(DWORD);

          wchar_t key[256]{};
          StringCchPrintf(key, ARRAYSIZE(key), L"%s\\%s", RegistryHelpers::REG_SETTINGS, value);
          SHRegGetUSValueW(key, L"ActiveZoneSetId", nullptr, &activeZoneSetId, &bufferSize, FALSE, nullptr, 0);
          SHRegGetUSValueW(key, L"ShowSpacing", nullptr, &showSpacing, &size, FALSE, nullptr, 0);
          SHRegGetUSValueW(key, L"Spacing", nullptr, &spacing, &size, FALSE, nullptr, 0);
          SHRegGetUSValueW(key, L"ZoneCount", nullptr, &zoneCount, &size, FALSE, nullptr, 0);

          deviceInfoMap[uniqueID] = DeviceInfoData{ std::wstring { activeZoneSetId }, static_cast<bool>(showSpacing), static_cast<int>(spacing), static_cast<int>(zoneCount) };

          valueLength = ARRAYSIZE(value);
        }
      }
    }
  }

  web::json::value FancyZonesData::AppliedZoneSetToJson(const AppliedZoneSetJSON& zoneSet) const {
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

  AppliedZoneSetJSON FancyZonesData::AppliedZoneSetFromJson(web::json::value zoneSet) const {
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

  web::json::value FancyZonesData::AppZoneHistoryToJson(const AppZoneHistoryJSON& appZoneHistory) const {
    web::json::value result = web::json::value::object();

    result[L"app-path"] = web::json::value::string(appZoneHistory.appPath);
    result[L"zoneset-uuid"] = web::json::value::string(appZoneHistory.data.zoneSetUuid);
    result[L"zone-index"] = web::json::value::number(appZoneHistory.data.zoneIndex);

    return result;
  }

  AppZoneHistoryJSON FancyZonesData::AppZoneHistoryFromJson(web::json::value zoneSet) const {
    AppZoneHistoryJSON result;

    result.appPath = zoneSet[L"app-path"].as_string();
    result.data.zoneSetUuid = zoneSet[L"zoneset-uuid"].as_string();
    result.data.zoneIndex = zoneSet[L"zone-index"].as_integer();

    return result;
  }

  web::json::value FancyZonesData::DeviceInfoToJson(const DeviceInfoJSON& device) const {
    web::json::value result = web::json::value::object();

    result[L"device-id"] = web::json::value::string(device.deviceId);
    result[L"active-zoneset-uuid"] = web::json::value::string(device.data.activeZoneSetUuid);
    result[L"show-spacing"] = web::json::value::boolean(device.data.showSpacing);
    result[L"spacing"] = web::json::value::number(device.data.spacing);
    result[L"zone-count"] = web::json::value::number(device.data.zoneCount);

    return result;
  }

  DeviceInfoJSON FancyZonesData::DeviceInfoFromJson(web::json::value device) const {
    DeviceInfoJSON result;

    result.deviceId = device[L"device-id"].as_string();
    result.data.activeZoneSetUuid = device[L"active-zoneset-uuid"].as_string();
    result.data.showSpacing = device[L"show-spacing"].as_bool();
    result.data.spacing = device[L"spacing"].as_integer();
    result.data.zoneCount = device[L"zone-count"].as_integer();

    return result;
  }

}