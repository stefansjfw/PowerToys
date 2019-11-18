#pragma once

#include <common/settings_helpers.h>

#include <winnt.h>
#include <cpprest/json.h>
#include <string>
#include <strsafe.h>
#include <variant>
#include <unordered_map>

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

  enum class CustomSetType : int
  {
    Canvas = 0,
    Grid
  };

  using TZoneCount = int;
  using TZoneUUID = std::wstring;
  using TAppPath = std::wstring;
  using TDeviceID = std::wstring;

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

  struct ActiveZoneSetData {
    TZoneUUID zoneSetUuid;
    bool showSpacing; //TODO(stefan): Check if we realy preserve these per device?
    int spacing;
  };

  struct ActiveZoneSetJSON {
    TDeviceID deviceId;
    ActiveZoneSetData data;
  };

  static const std::wstring FANCY_ZONES_DATA_FILE = L"PersistFancyZones.json";

  class FancyZonesData {
  public:
    FancyZonesData();

    const std::wstring& GetPersistFancyZonesJSONPath();
    web::json::value GetPersistFancyZonesJSON();

    bool GetAppLastZone(HWND window, PCWSTR appPath, _Out_ PINT iZoneIndex);
    bool SetAppLastZone(HWND window, PCWSTR appPath, DWORD zoneIndex); //TODO(stefan): Missing zone uuid (pass as arg)

    void SetActiveZoneSet(const std::wstring& unique_id, const UUID& uuid); //TODO(stefan): deviceID missing and what about spacing fields?
    UUID GetActiveZoneSet(const std::wstring& unique_id);

    void ParseAppliedZoneSets(const web::json::value& fancyZonesDataJSON);
    web::json::value SerializeAppliedZoneSets(); //TODO(stefan): Missing fromJson and toJson funcs
    void ParseAppZoneHistory(const web::json::value& fancyZonesDataJSON);
    web::json::value SerializeAppZoneHistory();
    void ParseActiveZoneSets(const web::json::value& fancyZonesDataJSON);
    web::json::value SerializeActiveZoneSets();

    void LoadFancyZonesData();
    void SaveFancyZonesData();
  private:
    void MigrateAppZoneHistoryFromRegistry(); //TODO(stefan): If uuid is needed here, it needs to be resolved here some how
    void MigrateActiveZoneSetsFromRegistry();

    web::json::value AppliedZoneSetToJson(const AppliedZoneSetJSON& zoneSet);
    AppliedZoneSetJSON AppliedZoneSetFromJson(web::json::value zoneSet);

    web::json::value AppZoneHistoryToJson(const AppZoneHistoryJSON& appZoneHistory);
    AppZoneHistoryJSON AppZoneHistoryFromJson(web::json::value zoneSet);

    web::json::value ActiveZoneSetToJson(const ActiveZoneSetJSON& zoneSet);
    ActiveZoneSetJSON ActiveZoneSetFromJson(web::json::value zoneSet);

    std::unordered_map<TZoneUUID, AppliedZoneSetData> appliedZoneSetMap{};
    std::unordered_map<TAppPath, AppZoneHistoryData> appZoneHistoryMap{};
    std::unordered_map<TZoneUUID, ActiveZoneSetData> activeZoneSetMap{};

    std::wstring jsonFilePath;
  };

  FancyZonesData& FancyZonesDataInstance();
}
