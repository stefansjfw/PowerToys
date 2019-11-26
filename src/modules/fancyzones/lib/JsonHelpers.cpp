#include "pch.h"
#include "JsonHelpers.h"

#include <shlwapi.h>
#include <filesystem>
#include <fstream>
#include <regex>

namespace {
  struct Monitors {
    HMONITOR* data;
    int count;
  };

  BOOL CALLBACK MyInfoEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData) {
    Monitors* monitors = reinterpret_cast<Monitors*>(dwData);
    monitors->data[monitors->count++] = hMonitor;
    return true;
  }

  // From Settings.cs
  constexpr int c_focusModelId = 0xFFFF;
  constexpr int c_rowsModelId = 0xFFFE;
  constexpr int c_columnsModelId = 0xFFFD;
  constexpr int c_gridModelId = 0xFFFC;
  constexpr int c_priorityGridModelId = 0xFFFB;
  constexpr int c_blankCustomModelId = 0xFFFA;

  JSONHelpers::ZoneSetLayoutType FromLayoutIdToZoneSetLayoutType(int layoutID) {
    using JSONHelpers::ZoneSetLayoutType;

    switch (layoutID) {
    case c_focusModelId:
      return ZoneSetLayoutType::Focus;
    case c_columnsModelId:
      return ZoneSetLayoutType::Columns;
    case c_rowsModelId:
      return ZoneSetLayoutType::Rows;
    case c_gridModelId:
      return ZoneSetLayoutType::Grid;
    case c_priorityGridModelId:
      return ZoneSetLayoutType::PriorityGrid;
    default:
      return ZoneSetLayoutType::Custom;
    }
  }

  int FromZoneSetLayoutTypeToLayoutId(JSONHelpers::ZoneSetLayoutType layoutType, JSONHelpers::AppliedZoneSetData zoneSet) {
    using JSONHelpers::ZoneSetLayoutType;

    switch (layoutType) {
    case ZoneSetLayoutType::Focus:
      return c_focusModelId;
    case ZoneSetLayoutType::Columns:
      return c_columnsModelId;
    case ZoneSetLayoutType::Rows:
      return c_rowsModelId;
    case ZoneSetLayoutType::Grid:
      return c_gridModelId;
    case ZoneSetLayoutType::PriorityGrid:
      return c_priorityGridModelId;
    case ZoneSetLayoutType::Custom:
      return std::stoi(std::get<JSONHelpers::TZoneUUID>(zoneSet.info));
    }
    return -1; // Error
  }
}

namespace JSONHelpers {
  int AppliedZoneSetData::TypeToLayoutID() const {
    return FromZoneSetLayoutTypeToLayoutId(type, *this);
  }

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

  void FancyZonesData::SetActiveZoneSet(const std::wstring& uniqueID, const std::wstring& uuid /*TODO(stefan): deviceId instead of uniqueId in the future*/) {
    if (!uuid.empty()) {
      deviceInfoMap[uniqueID].activeZoneSetUuid = uuid;
    }
  }

  void FancyZonesData::GetAppliedZoneSetFromTmpFile(const std::wstring& tmpFilePath) {
    if (std::filesystem::exists(tmpFilePath)) {
      std::ifstream tmpFile(tmpFilePath, std::ios::binary);
      web::json::value appliedZoneSetJson = web::json::value::parse(tmpFile);
      AppliedZoneSetJSON appliedZoneSet = AppliedZoneSetFromJson(appliedZoneSetJson);
      appliedZoneSetMap[appliedZoneSet.uuid] = appliedZoneSet.data;

      tmpFile.close();
      DeleteFileW(tmpFilePath.c_str());
    }
  }

  void FancyZonesData::SetDeviceInfoToTmpFile(const DeviceInfoJSON& deviceInfo, const std::wstring& tmpFilePath) {
    std::ofstream tmpFile;
    tmpFile.open(tmpFilePath);
    web::json::value deviceInfoJson = DeviceInfoToJson(deviceInfo);
    deviceInfoJson.serialize(tmpFile);
    tmpFile.close();
  }


  void FancyZonesData::GetDeviceInfoFromTmpFile(const std::wstring& uniqueID, const std::wstring& tmpFilePath) {
    if (!deviceInfoMap.contains(uniqueID)) {
      deviceInfoMap[uniqueID] = DeviceInfoData{ L"{}", true, 16, 3 }; // Creates entry in map when ZoneWindow is created
    }

    if (std::filesystem::exists(tmpFilePath)) {
      std::ifstream tmpFile(tmpFilePath, std::ios::binary);
      web::json::value zoneSetJson = web::json::value::parse(tmpFile);
      DeviceInfoJSON deviceInfo = DeviceInfoFromJson(zoneSetJson);
      deviceInfoMap[uniqueID] = deviceInfo.data;

      tmpFile.close();
      DeleteFileW(tmpFilePath.c_str());
    }
  }

  void FancyZonesData::GetCustomZoneSetFromTmpFile(const std::wstring& tmpFilePath) {
    if (std::filesystem::exists(tmpFilePath)) {
      std::ifstream tmpFile(tmpFilePath, std::ios::binary);
      web::json::value customZoneSetJson = web::json::value::parse(tmpFile);
      CustomZoneSetJSON customZoneSet = CustomZoneSetFromJson(customZoneSetJson);
      customZoneSetsMap[customZoneSet.uuid] = customZoneSet.data;

      tmpFile.close();
      DeleteFileW(tmpFilePath.c_str());
    }
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
    for (const auto& [zoneSetID, zoneSetData] : appliedZoneSetMap) {
      appliedZoneSets[i++] = AppliedZoneSetToJson(AppliedZoneSetJSON{ zoneSetID, zoneSetData });
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
    for (const auto& [appPath, appZoneHistoryData] : appZoneHistoryMap) {
      appHistoryArray[i++] = AppZoneHistoryToJson(AppZoneHistoryJSON{ appPath, appZoneHistoryData });
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
    for (const auto& [deviceID, deviceData] : deviceInfoMap) {
      DeviceInfosJSON[i++] = DeviceInfoToJson(DeviceInfoJSON{ deviceID, deviceData });
    }

    return DeviceInfosJSON;
  }

  void FancyZonesData::ParseCustomZoneSets(const web::json::value& fancyZonesDataJSON) {
    if (fancyZonesDataJSON.has_field(L"custom-zone-sets") && !fancyZonesDataJSON.at(U("custom-zone-sets")).is_null()) {
      web::json::array customZoneSets = fancyZonesDataJSON.at(U("custom-zone-sets")).as_array();

      for (const auto& zoneSetJSON : customZoneSets) {
        const auto& zoneSet = CustomZoneSetFromJson(zoneSetJSON);
        customZoneSetsMap[zoneSet.uuid] = zoneSet.data;
      }
    }
  }

  web::json::value FancyZonesData::SerializeCustomZoneSets() const {
    web::json::value CustomZoneSetsJSON{};

    int i = 0;
    for (const auto& [zoneSetId, zoneSetData] : customZoneSetsMap) {
      CustomZoneSetsJSON[i++] = CustomZoneSetToJson(CustomZoneSetJSON{ zoneSetId, zoneSetData });
    }

    return CustomZoneSetsJSON;
  }

  void FancyZonesData::LoadFancyZonesData()
  {
    std::wstring jsonFilePath = GetPersistFancyZonesJSONPath();

    if (!std::filesystem::exists(jsonFilePath)) {

      MigrateCustomZoneSetsFromRegistry(); // Custom zone sets have to be migrated before applied zone sets!
      MigrateAppliedZoneSetsFromRegistry();
      MigrateAppZoneHistoryFromRegistry();
      MigrateDeviceInfoFromRegistry();

      SaveFancyZonesData();
    }
    else {
      web::json::value fancyZonesDataJSON = GetPersistFancyZonesJSON();

      ParseAppliedZoneSets(fancyZonesDataJSON);
      ParseAppZoneHistory(fancyZonesDataJSON);
      ParseDeviceInfos(fancyZonesDataJSON);
      ParseCustomZoneSets(fancyZonesDataJSON);

    }
  }

  void FancyZonesData::SaveFancyZonesData() const {
    std::ofstream jsonFile{ jsonFilePath };

    web::json::value root{};

    root[L"applied-zone-sets"] = SerializeAppliedZoneSets();
    root[L"app-zone-history"] = SerializeAppZoneHistory();
    root[L"devices"] = SerializeDeviceInfos();
    root[L"custom-zone-sets"] = SerializeCustomZoneSets();

    root.serialize(jsonFile);

    jsonFile.close();
  }

  void FancyZonesData::MigrateAppliedZoneSetsFromRegistry() {
    std::wregex ex(L"^[0-9]{3,4}_[0-9]{3,4}$");

    wchar_t key[256];
    StringCchPrintf(key, ARRAYSIZE(key), L"%s", RegistryHelpers::REG_SETTINGS);
    HKEY hkey;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, key, 0, KEY_ALL_ACCESS, &hkey) == ERROR_SUCCESS) {
      wchar_t resolutionKey[256]{};
      DWORD resolutionKeyLength = ARRAYSIZE(resolutionKey);
      DWORD i = 0;
      while (RegEnumKeyW(hkey, i++, resolutionKey, resolutionKeyLength) == ERROR_SUCCESS)
      {
        std::wstring resolution{ resolutionKey };
        wchar_t appliedZoneSetskey[256];
        StringCchPrintf(appliedZoneSetskey, ARRAYSIZE(appliedZoneSetskey), L"%s\\%s", RegistryHelpers::REG_SETTINGS, resolutionKey);
        HKEY appliedZoneSetsHkey;
        if (std::regex_match(resolution, ex) && RegOpenKeyExW(HKEY_CURRENT_USER, appliedZoneSetskey, 0, KEY_ALL_ACCESS, &appliedZoneSetsHkey) == ERROR_SUCCESS) {
          ZoneSetPersistedData data;
          DWORD dataSize = sizeof(data);
          wchar_t value[256]{};
          DWORD valueLength = ARRAYSIZE(value);
          DWORD i = 0;
          while (RegEnumValueW(appliedZoneSetsHkey, i++, value, &valueLength, nullptr, nullptr, reinterpret_cast<BYTE*>(&data), &dataSize) == ERROR_SUCCESS) {
            AppliedZoneSetData appliedZoneSetData;
            appliedZoneSetData.name = L"NAME_TBD"; // TODO(stefan): Fill this if even necessary
            appliedZoneSetData.type = FromLayoutIdToZoneSetLayoutType(data.LayoutId);
            if (appliedZoneSetData.type == ZoneSetLayoutType::Custom) {
              appliedZoneSetData.info = std::to_wstring(data.LayoutId);
            }
            else {
              appliedZoneSetData.info = data.ZoneCount;
            }
            appliedZoneSetMap[value] = appliedZoneSetData;
            dataSize = sizeof(data);
            valueLength = ARRAYSIZE(value);
          }
        }
        resolutionKeyLength = ARRAYSIZE(resolutionKey);
      }
    }
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

  void FancyZonesData::MigrateCustomZoneSetsFromRegistry() {
    wchar_t key[256];
    StringCchPrintf(key, ARRAYSIZE(key), L"%s\\%s", RegistryHelpers::REG_SETTINGS, L"Layouts");
    HKEY hkey;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, key, 0, KEY_ALL_ACCESS, &hkey) == ERROR_SUCCESS) {
      BYTE data[256];
      DWORD dataSize = ARRAYSIZE(data);
      wchar_t value[256]{};
      DWORD valueLength = ARRAYSIZE(value);
      DWORD i = 0;
      while (RegEnumValueW(hkey, i++, value, &valueLength, nullptr, nullptr, reinterpret_cast<BYTE*>(&data), &dataSize) == ERROR_SUCCESS) {
        CustomZoneSetData zoneSetData;
        zoneSetData.name = std::wstring{ value };
        zoneSetData.type = static_cast<CustomLayoutType>(data[2]);
        // int version =  data[0] * 256 + data[1]; - Not used anymore

        //TODO(stefan): We need this layoutID for migration. It's unique for custom zone sets.
        //Should we use this for id or just for migration (won't be present in .json file!) and generate UUID
        std::wstring uuid = std::to_wstring(data[3] * 256 + data[4]);
        switch (zoneSetData.type) {
        case CustomLayoutType::Grid:
        {
          GridLayoutInfo zoneSetInfo;
          int j = 5;

          zoneSetInfo.rows = data[j++];
          zoneSetInfo.columns = data[j++];

          zoneSetInfo.rowPercents.reserve(zoneSetInfo.rows);
          for (int row = 0; row < zoneSetInfo.rows; row++)
          {
            zoneSetInfo.rowPercents.push_back(data[j++] * 256 + data[j++]);
          }

          zoneSetInfo.columnPercents.reserve(zoneSetInfo.columns);
          for (int col = 0; col < zoneSetInfo.columns; col++)
          {
            zoneSetInfo.columnPercents.push_back(data[j++] * 256 + data[j++]);
          }

          for (int row = 0; row < zoneSetInfo.rows; row++)
          {
            for (int col = 0; col < zoneSetInfo.columns; col++)
            {
              zoneSetInfo.cellChildMap[row][col] = data[j++];
            }
          }
          zoneSetData.info = zoneSetInfo;
          break;
        }
        case CustomLayoutType::Canvas:
        {
          CanvasLayoutInfo info;

          int j = 5;
          info.referenceWidth = data[j++] * 256 + data[j++];
          info.referenceHeight = data[j++] * 256 + data[j++];

          int count = data[j++];
          info.zones.reserve(count);
          while (count-- > 0)
          {
            info.zones.push_back(CanvasLayoutInfo::Rect{
              data[j++] * 256 + data[j++],
              data[j++] * 256 + data[j++],
              data[j++] * 256 + data[j++],
              data[j++] * 256 + data[j++] });
          }
          zoneSetData.info = info;
          break;
        }
        default:
          abort(); // TODO(stefan): Exception safety
        }
        customZoneSetsMap[uuid] = zoneSetData;
        valueLength = ARRAYSIZE(value);
        dataSize = ARRAYSIZE(data);
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

  web::json::value FancyZonesData::CustomZoneSetToJson(const CustomZoneSetJSON& customZoneSet) const {
    web::json::value result = web::json::value::object();

    result[L"uuid"] = web::json::value::string(customZoneSet.uuid);
    result[L"name"] = web::json::value::string(customZoneSet.data.name);
    switch (customZoneSet.data.type) {
    case CustomLayoutType::Canvas:
    {
      result[L"type"] = web::json::value::string(L"canvas");

      CanvasLayoutInfo canvasInfo = std::get<CanvasLayoutInfo>(customZoneSet.data.info);
      web::json::value infoJson;
      infoJson[L"ref-width"] = web::json::value::number(canvasInfo.referenceWidth);
      infoJson[L"ref-height"] = web::json::value::number(canvasInfo.referenceHeight);
      web::json::value zonesJson;
      int i = 0;
      for (const auto& [x, y, width, height] : canvasInfo.zones) {
        web::json::value zoneJson;
        zoneJson[L"X"] = web::json::value::number(x);
        zoneJson[L"Y"] = web::json::value::number(y);
        zoneJson[L"width"] = web::json::value::number(width);
        zoneJson[L"height"] = web::json::value::number(height);
        zonesJson[i++] = zoneJson;
      }
      infoJson[L"zones"] = zonesJson;
      result[L"info"] = infoJson;

      break;
    }
    case CustomLayoutType::Grid:
    {
      result[L"type"] = web::json::value::string(L"grid");

      GridLayoutInfo gridInfo = std::get<GridLayoutInfo>(customZoneSet.data.info);
      web::json::value infoJson;
      infoJson[L"rows"] = web::json::value::number(gridInfo.rows);
      infoJson[L"columns"] = web::json::value::number(gridInfo.columns);
      web::json::value rowsPercentageJson;
      int i = 0;
      for (const auto& rowsPercentsElem : gridInfo.rowPercents) {
        rowsPercentageJson[i++] = web::json::value::number(rowsPercentsElem);
      }
      infoJson[L"rows-percentage"] = rowsPercentageJson;

      web::json::value columnPercentageJson;
      i = 0;
      for (const auto& columnsPercentsElem : gridInfo.columnPercents) {
        columnPercentageJson[i++] = web::json::value::number(columnsPercentsElem);
      }
      infoJson[L"columns-percentage"] = columnPercentageJson;

      i = 0;
      web::json::value cellChildMapJson;
      for (const auto& cellChildMapRow : gridInfo.cellChildMap) {
        int j = 0;
        web::json::value cellChildMapRowJson;
        for (const auto& cellChildMapRowElem : cellChildMapRow) {
          cellChildMapRowJson[j++] = web::json::value::number(cellChildMapRowElem);
        }
        cellChildMapJson[i++] = cellChildMapRowJson;
      }
      infoJson[L"cell-child-map"] = cellChildMapJson;

      result[L"info"] = infoJson;
      break;
    }
    default:
      abort(); //TODO(stefan): Exception safety
    }


    return result;
  }

  CustomZoneSetJSON FancyZonesData::CustomZoneSetFromJson(web::json::value customZoneSet) const {
    CustomZoneSetJSON result;

    result.uuid = customZoneSet[L"uuid"].as_string();
    result.data.name = customZoneSet[L"name"].as_string();
    std::wstring zoneSetType = customZoneSet[L"type"].as_string();
    web::json::value infoJson = customZoneSet[L"info"];
    if (zoneSetType.compare(L"canvas") == 0) {
      result.data.type = CustomLayoutType::Canvas;

      CanvasLayoutInfo info;
      info.referenceWidth = infoJson[L"ref-width"].as_integer();
      info.referenceHeight = infoJson[L"ref-height"].as_integer();
      web::json::array zonesJson = infoJson[L"zones"].as_array();
      for (auto zoneJson : zonesJson) {
        CanvasLayoutInfo::Rect zone{ zoneJson[L"X"].as_integer(), zoneJson[L"Y"].as_integer(), zoneJson[L"width"].as_integer(), zoneJson[L"height"].as_integer() };
        info.zones.push_back(zone);
      }
      result.data.info = info;
    }
    else if (zoneSetType.compare(L"grid")) {
      result.data.type = CustomLayoutType::Grid;

      GridLayoutInfo info;
      info.rows = infoJson[L"rows"].as_integer();
      info.columns = infoJson[L"columns"].as_integer();
      web::json::array rowsPercentage = infoJson[L"rows-percentage"].as_array();
      for (auto percentage : rowsPercentage) {
        info.rowPercents.push_back(percentage.as_integer());
      }
      web::json::array columnsPercentage = infoJson[L"columns-percentage"].as_array();
      for (auto percentage : columnsPercentage) {
        info.columnPercents.push_back(percentage.as_integer());
      }
      web::json::array cellChildMap = infoJson[L"cell-child-map"].as_array();
      for (auto mapRow : cellChildMap) {
        std::vector<int> cellChildMapRow;
        for (auto rowElem : mapRow.as_array()) {
          cellChildMapRow.push_back(rowElem.as_integer());
        }
        info.cellChildMap.push_back(cellChildMapRow);
      }
      result.data.info = info;
    }
    else {
      abort(); //TODO(stefan): exception safety
    }

    return result;
  }
}