#pragma once

#include <common/settings_helpers.h>

#include <cpprest/json.h>
#include <string>
#include <strsafe.h>
#include <unordered_map>
#include <variant>
#include <vector>
#include <winnt.h>

namespace {

  struct Monitors {
    HMONITOR* data;
    int count;
  };

  BOOL CALLBACK MyInfoEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData)
  {
    Monitors* monitors = reinterpret_cast<Monitors*>(dwData);
    monitors->data[monitors->count++] = hMonitor;
    return true;
  }
}
namespace JSONHelpers
{
  enum class ZoneSetLayoutType : int
  {
    Focus = 0,
    Columns,
    Rows,
    Grid,
    PriorityGrid,
    Custom
  };

  enum class CustomLayoutType : int
  {
    Canvas = 0,
    Grid
  };

  using TZoneCount = int;
  using TZoneUUID = std::wstring;
  using TAppPath = std::wstring;
  using TDeviceID = std::wstring;

  struct CanvasLayoutInfo {
    int referenceWidth;
    int referenceHeight;
    struct Rect {
      int x;
      int y;
      int width;
      int height;
    };
    std::vector<CanvasLayoutInfo::Rect> zones;
  };

  struct GridLayoutInfo {
    int rows;
    int columns;
    std::vector<int> rowPercents;
    std::vector<int> columnPercents;
    std::vector<std::vector<int>> cellChildMap;
  };

  struct CustomZoneSetData {
    TZoneUUID uuid;
    std::wstring name;
    CustomLayoutType type;
    std::variant<CanvasLayoutInfo, GridLayoutInfo> info;
  };

  struct CustomZoneSetJSON {
    TZoneUUID uuid;
    CustomZoneSetData data;
  };

  // TODO(stefan): This needs to be moved to ZoneSet.h (probably)
  struct AppliedZoneSetData {
    std::wstring name;
    ZoneSetLayoutType type;
    std::variant<TZoneCount, TZoneUUID> info;
  };

  struct AppliedZoneSetJSON {
    TZoneUUID uuid;
    AppliedZoneSetData data;
  };

  struct AppZoneHistoryData {
    TZoneUUID zoneSetUuid; //TODO(stefan): is this nessecary? It doesn't exist with registry impl.
    int zoneIndex;
    //TODO(stefan): Also, do we need DeviceID here? Do we want to support that - app history per monitor?
  };

  struct AppZoneHistoryJSON {
    TAppPath appPath;
    AppZoneHistoryData data;
  };

  struct DeviceInfoData {
    TZoneUUID activeZoneSetUuid;
    bool showSpacing;
    int spacing;
    int zoneCount;
  };

  struct DeviceInfoJSON {
    TDeviceID deviceId;
    DeviceInfoData data;
  };

  static const std::wstring FANCY_ZONES_DATA_FILE = L"PersistFancyZones.json";

  class FancyZonesData {
  public:
    FancyZonesData();

    const std::wstring& GetPersistFancyZonesJSONPath() const;
    web::json::value GetPersistFancyZonesJSON();

    std::unordered_map<TZoneUUID, AppliedZoneSetData>& GetAppliedZoneSetMap() {
      return appliedZoneSetMap;
    }

    const std::unordered_map<TZoneUUID, AppliedZoneSetData>& GetAppliedZoneSetMap() const {
      return appliedZoneSetMap;
    }

    std::unordered_map<TZoneUUID, DeviceInfoData>& GetDeviceInfoMap() {
      return deviceInfoMap;
    }

    const std::unordered_map<TZoneUUID, DeviceInfoData>& GetDeviceInfoMap() const {
      return deviceInfoMap;
    }

    bool GetAppLastZone(HWND window, PCWSTR appPath, _Out_ PINT iZoneIndex) const;
    bool SetAppLastZone(HWND window, PCWSTR appPath, DWORD zoneIndex); //TODO(stefan): Missing zone uuid (pass as arg)

    void SetActiveZoneSet(const std::wstring& unique_id, const std::wstring& uuid); //TODO(stefan): deviceID missing and what about spacing fields?
    void SetDeviceInfoToTmpFile(const std::wstring& unique_id, const std::wstring& uuid, const std::wstring& tmpFilePath);
    UUID GetActiveZoneSet(const std::wstring& unique_id, const std::wstring& tmpFilePath);

    void ParseAppliedZoneSets(const web::json::value& fancyZonesDataJSON);
    web::json::value SerializeAppliedZoneSets() const; //TODO(stefan): Missing fromJson and toJson funcs
    void ParseAppZoneHistory(const web::json::value& fancyZonesDataJSON);
    web::json::value SerializeAppZoneHistory() const;
    void ParseDeviceInfos(const web::json::value& fancyZonesDataJSON);
    web::json::value SerializeDeviceInfos() const;

    void LoadFancyZonesData();
    void SaveFancyZonesData() const;
  private:
    void MigrateAppZoneHistoryFromRegistry(); //TODO(stefan): If uuid is needed here, it needs to be resolved here some how
    void MigrateDeviceInfoFromRegistry();

    web::json::value AppliedZoneSetToJson(const AppliedZoneSetJSON& zoneSet) const;
    AppliedZoneSetJSON AppliedZoneSetFromJson(web::json::value zoneSet) const;

    web::json::value AppZoneHistoryToJson(const AppZoneHistoryJSON& appZoneHistory) const;
    AppZoneHistoryJSON AppZoneHistoryFromJson(web::json::value zoneSet) const;

    web::json::value DeviceInfoToJson(const DeviceInfoJSON& device) const;
    DeviceInfoJSON DeviceInfoFromJson(web::json::value device) const;

    std::unordered_map<TZoneUUID, AppliedZoneSetData> appliedZoneSetMap{};
    std::unordered_map<TAppPath, AppZoneHistoryData> appZoneHistoryMap{};
    std::unordered_map<TDeviceID, DeviceInfoData> deviceInfoMap{};

    std::wstring jsonFilePath;
  };

  FancyZonesData& FancyZonesDataInstance();
}
