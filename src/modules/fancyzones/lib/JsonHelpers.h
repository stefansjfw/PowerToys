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

    std::wstring TypeToString(ZoneSetLayoutType type);
    ZoneSetLayoutType TypeFromString(const std::wstring& typeStr);

    ZoneSetLayoutType TypeFromLayoutId(int layoutID);
    int TypeToLayoutId(JSONHelpers::ZoneSetLayoutType layoutType);

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

        static json::JsonObject ToJson(const CanvasLayoutInfo& canvasInfo);
        static CanvasLayoutInfo FromJson(const json::JsonObject& infoJson);
    };

    struct GridLayoutInfo
    {
        int rows;
        int columns;
        int rowsPercents[MAX_ZONE_COUNT];
        int columnsPercents[MAX_ZONE_COUNT];
        int cellChildMap[MAX_ZONE_COUNT][MAX_ZONE_COUNT];

        static json::JsonObject ToJson(const GridLayoutInfo& gridInfo);
        static GridLayoutInfo FromJson(const json::JsonObject& infoJson);
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

        static json::JsonObject ToJson(const CustomZoneSetJSON& device);
        static CustomZoneSetJSON FromJson(const json::JsonObject& customZoneSet);
    };

    // TODO(stefan): This needs to be moved to ZoneSet.h (probably)
    struct ZoneSetData
    {
        TZoneUUID uuid;
        ZoneSetLayoutType type; // TODO(stefan): Change this to string in JSON so user can understand it
        std::optional<int> zoneCount;

        static json::JsonObject ToJson(const ZoneSetData& zoneSet);
        static ZoneSetData FromJson(const json::JsonObject& zoneSet);
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

        static json::JsonObject ToJson(const AppZoneHistoryJSON& appZoneHistory);
        static AppZoneHistoryJSON FromJson(const json::JsonObject& zoneSet);
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

        static json::JsonObject ToJson(const DeviceInfoJSON& device);
        static DeviceInfoJSON FromJson(const json::JsonObject& device);
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

        const std::unordered_map<TAppPath, AppZoneHistoryData>& GetAppZoneHistoryMap() const
        {
            return appZoneHistoryMap;
        }

        bool GetAppLastZone(HWND window, PCWSTR appPath, _Out_ PINT iZoneIndex) const;
        bool SetAppLastZone(HWND window, PCWSTR appPath, DWORD zoneIndex); //TODO(stefan): Missing zone uuid (pass as arg)

        void SetActiveZoneSet(const std::wstring& uniqueID, const std::wstring& uuid); //TODO(stefan): deviceID missing and what about spacing fields?

        void SetDeviceInfoToTmpFile(const DeviceInfoJSON& deviceInfo, const std::wstring& tmpFilePath) const;
        void GetDeviceInfoFromTmpFile(const std::wstring& uniqueID, const std::wstring& tmpFilePath);

        void GetCustomZoneSetFromTmpFile(const std::wstring& tmpFilePath, const std::wstring& uuid);

        void ParseAppZoneHistory(const json::JsonObject& fancyZonesDataJSON);
        json::JsonArray SerializeAppZoneHistory() const;
        void ParseDeviceInfos(const json::JsonObject& fancyZonesDataJSON);
        json::JsonArray SerializeDeviceInfos() const;
        void ParseCustomZoneSets(const json::JsonObject& fancyZonesDataJSON);
        json::JsonArray SerializeCustomZoneSets() const;
        void CustomZoneSetsToJsonFile(const std::wstring& filePath) const;

        void LoadFancyZonesData();
        void SaveFancyZonesData() const;

    private:
        void TmpMigrateAppliedZoneSetsFromRegistry(TAppliedZoneSetsMap& appliedZoneSetMap);
        void MigrateAppZoneHistoryFromRegistry(); //TODO(stefan): If uuid is needed here, it needs to be resolved here some how
        void MigrateDeviceInfoFromRegistry();
        void MigrateCustomZoneSetsFromRegistry();

        std::unordered_map<TAppPath, AppZoneHistoryData> appZoneHistoryMap{};
        TDeviceInfosMap deviceInfoMap{};
        TCustomZoneSetsMap customZoneSetsMap{};

        std::wstring jsonFilePath;
    };

    FancyZonesData& FancyZonesDataInstance();
}
