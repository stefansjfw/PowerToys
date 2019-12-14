#pragma once

#include <common/settings_helpers.h>
#include <common/json.h>

#include <string>
#include <strsafe.h>
#include <unordered_map>
#include <variant>
#include <optional>
#include <vector>
#include <winnt.h>

namespace JSONHelpers
{
    constexpr int MAX_ZONE_COUNT = 50;

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
        Grid = 0,
        Canvas
    };

    using TZoneCount = int;
    using TZoneUUID = std::wstring;
    using TAppPath = std::wstring;
    using TDeviceID = std::wstring;

    struct CanvasLayoutInfo
    {
        int referenceWidth;
        int referenceHeight;
        struct Rect
        {
            int x;
            int y;
            int width;
            int height;
        };
        std::vector<CanvasLayoutInfo::Rect> zones;
    };

    struct GridLayoutInfo
    {
        int rows;
        int columns;
        int rowsPercents[MAX_ZONE_COUNT];
        int columnsPercents[MAX_ZONE_COUNT];
        int cellChildMap[MAX_ZONE_COUNT][MAX_ZONE_COUNT];
    };

    struct CustomZoneSetData
    {
        std::wstring name;
        CustomLayoutType type;
        std::variant<CanvasLayoutInfo, GridLayoutInfo> info;
    };

    struct CustomZoneSetJSON
    {
        TZoneUUID uuid;
        CustomZoneSetData data;
    };

    // TODO(stefan): This needs to be moved to ZoneSet.h (probably)
    struct ZoneSetData
    {
        TZoneUUID uuid;
        ZoneSetLayoutType type; // TODO(stefan): Change this to string in JSON so user can understand it
        std::optional<int> zoneCount;

    public:
        static std::wstring TypeToString(ZoneSetLayoutType type);
        static ZoneSetLayoutType TypeFromString(std::wstring typeStr);

        static ZoneSetLayoutType LayoutIdToType(int layoutID);
        static int TypeToLayoutId(JSONHelpers::ZoneSetLayoutType layoutType);
    };

    struct AppZoneHistoryData
    {
        TZoneUUID zoneSetUuid; //TODO(stefan): is this nessecary? It doesn't exist with registry impl.
        int zoneIndex;
        //TODO(stefan): Also, do we need DeviceID here? Do we want to support that - app history per monitor?
    };

    struct AppZoneHistoryJSON
    {
        TAppPath appPath;
        AppZoneHistoryData data;
    };

    struct DeviceInfoData
    {
        ZoneSetData activeZoneSet;
        bool showSpacing;
        int spacing;
        int zoneCount;
    };

    struct DeviceInfoJSON
    {
        TDeviceID deviceId;
        DeviceInfoData data;
    };

    using TDeviceInfosMap = std::unordered_map<TZoneUUID, DeviceInfoData>;
    using TCustomZoneSetsMap = std::unordered_map<TZoneUUID, CustomZoneSetData>;
    using TAppliedZoneSetsMap = std::unordered_map<TZoneUUID, ZoneSetData>;

    static const std::wstring FANCY_ZONES_DATA_FILE = L"PersistFancyZones.json";

    class FancyZonesData
    {
    public:
        FancyZonesData();

        const std::wstring& GetPersistFancyZonesJSONPath() const;
        json::JsonObject GetPersistFancyZonesJSON();

        TDeviceInfosMap& GetDeviceInfoMap()
        {
            return deviceInfoMap;
        }

        const TDeviceInfosMap& GetDeviceInfoMap() const
        {
            return deviceInfoMap;
        }

        TCustomZoneSetsMap& GetCustomZoneSetsMap()
        {
            return customZoneSetsMap;
        }

        const TCustomZoneSetsMap& GetCustomZoneSetsMap() const
        {
            return customZoneSetsMap;
        }

        bool GetAppLastZone(HWND window, PCWSTR appPath, _Out_ PINT iZoneIndex) const;
        bool SetAppLastZone(HWND window, PCWSTR appPath, DWORD zoneIndex); //TODO(stefan): Missing zone uuid (pass as arg)

        void SetActiveZoneSet(const std::wstring& uniqueID, const std::wstring& uuid); //TODO(stefan): deviceID missing and what about spacing fields?

        void SetDeviceInfoToTmpFile(const DeviceInfoJSON& deviceInfo, const std::wstring& tmpFilePath);
        void GetDeviceInfoFromTmpFile(const std::wstring& uniqueID, const std::wstring& tmpFilePath);

        void GetCustomZoneSetFromTmpFile(const std::wstring& tmpFilePath, const std::wstring& uuid);

        void ParseAppZoneHistory(const json::JsonObject& fancyZonesDataJSON);
        json::JsonArray SerializeAppZoneHistory() const;
        void ParseDeviceInfos(const json::JsonObject& fancyZonesDataJSON);
        json::JsonArray SerializeDeviceInfos() const;
        void ParseCustomZoneSets(const json::JsonObject& fancyZonesDataJSON);
        json::JsonArray SerializeCustomZoneSets() const;

        void LoadFancyZonesData();
        void SaveFancyZonesData() const;

    private:
        void TmpMigrateAppliedZoneSetsFromRegistry(TAppliedZoneSetsMap& appliedZoneSetMap);
        void MigrateAppZoneHistoryFromRegistry(); //TODO(stefan): If uuid is needed here, it needs to be resolved here some how
        void MigrateDeviceInfoFromRegistry();
        void MigrateCustomZoneSetsFromRegistry();

        json::JsonObject ZoneSetDataToJson(const ZoneSetData& zoneSet) const;
        ZoneSetData ZoneSetDataFromJson(const json::JsonObject& zoneSet) const;

        json::JsonObject AppZoneHistoryToJson(const AppZoneHistoryJSON& appZoneHistory) const;
        AppZoneHistoryJSON AppZoneHistoryFromJson(const json::JsonObject& zoneSet) const;

        json::JsonObject DeviceInfoToJson(const DeviceInfoJSON& device) const;
        DeviceInfoJSON DeviceInfoFromJson(const json::JsonObject& device) const;

        json::JsonObject CustomZoneSetToJson(const CustomZoneSetJSON& device) const;
        CustomZoneSetJSON CustomZoneSetFromJson(const json::JsonObject& customZoneSet) const;

        std::unordered_map<TAppPath, AppZoneHistoryData> appZoneHistoryMap{};
        TDeviceInfosMap deviceInfoMap{};
        TCustomZoneSetsMap customZoneSetsMap{};

        std::wstring jsonFilePath;
    };

    FancyZonesData& FancyZonesDataInstance();
}
