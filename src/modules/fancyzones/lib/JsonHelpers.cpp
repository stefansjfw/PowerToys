#include "pch.h"
#include "JsonHelpers.h"

#include <shlwapi.h>
#include <filesystem>
#include <fstream>
#include <regex>

namespace
{
    struct Monitors
    {
        HMONITOR* data;
        int count;
    };

    BOOL CALLBACK MyInfoEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData)
    {
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

}

namespace JSONHelpers
{
    int TypeToLayoutId(JSONHelpers::ZoneSetLayoutType layoutType)
    {
        switch (layoutType)
        {
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
            return 0;
        }
        return -1; // Error
    }

    ZoneSetLayoutType TypeFromLayoutId(int layoutID)
    {
        switch (layoutID)
        {
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

    std::wstring TypeToString(ZoneSetLayoutType type)
    {
        switch (type)
        {
        case ZoneSetLayoutType::Focus:
            return L"focus";
        case ZoneSetLayoutType::Columns:
            return L"columns";
        case ZoneSetLayoutType::Rows:
            return L"rows";
        case ZoneSetLayoutType::Grid:
            return L"grid";
        case ZoneSetLayoutType::PriorityGrid:
            return L"priority-grid";
        case ZoneSetLayoutType::Custom:
            return L"custom";
        default:
            return L"TypeToString_ERROR";
        }
    }

    ZoneSetLayoutType TypeFromString(const std::wstring& typeStr)
    {
        if (typeStr.compare(L"focus") == 0)
        {
            return JSONHelpers::ZoneSetLayoutType::Focus;
        }
        else if (typeStr.compare(L"columns") == 0)
        {
            return JSONHelpers::ZoneSetLayoutType::Columns;
        }
        else if (typeStr.compare(L"rows") == 0)
        {
            return JSONHelpers::ZoneSetLayoutType::Rows;
        }
        else if (typeStr.compare(L"grid") == 0)
        {
            return JSONHelpers::ZoneSetLayoutType::Grid;
        }
        else if (typeStr.compare(L"priority-grid") == 0)
        {
            return JSONHelpers::ZoneSetLayoutType::PriorityGrid;
        }
        else
        { //Custom
            return JSONHelpers::ZoneSetLayoutType::Custom;
        }
    }

    FancyZonesData& FancyZonesDataInstance()
    {
        static FancyZonesData instance;
        return instance;
    }

    FancyZonesData::FancyZonesData()
    {
        std::wstring result = PTSettingsHelper::get_module_save_folder_location(L"FancyZones");
        jsonFilePath = result + L"\\" + std::wstring(FANCY_ZONES_DATA_FILE);
    }

    const std::wstring& FancyZonesData::GetPersistFancyZonesJSONPath() const
    {
        return jsonFilePath;
    }

    json::JsonObject FancyZonesData::GetPersistFancyZonesJSON()
    {
        std::wstring save_file_path = GetPersistFancyZonesJSONPath();

        auto result = json::from_file(save_file_path);
        if (result)
        {
            return *result;
        }
        else
        {
            return json::JsonObject();
        }
    }

    bool FancyZonesData::GetAppLastZone(HWND window, PCWSTR appPath, _Out_ PINT iZoneIndex) const
    {
        *iZoneIndex = -1;

        if (auto monitor = MonitorFromWindow(window, MONITOR_DEFAULTTONULL))
        {
            TAppPath path{ appPath };
            if (appZoneHistoryMap.contains(path))
            {
                *iZoneIndex = appZoneHistoryMap.at(path).zoneIndex;
                return true;
            }
        }
        return false;
    }

    // Pass -1 for the zoneIndex to delete the entry from the map
    bool FancyZonesData::SetAppLastZone(HWND window, PCWSTR appPath, DWORD zoneIndex)
    {
        if (auto monitor = MonitorFromWindow(window, MONITOR_DEFAULTTONULL))
        {
            if (zoneIndex == -1)
            {
                appZoneHistoryMap.erase(TAppPath{ appPath });
            }
            else
            {
                appZoneHistoryMap[TAppPath{ appPath }] = AppZoneHistoryData{ L"NESTO_NESTO", static_cast<int>(zoneIndex) }; //TODO(stefan)
            }
            return true;
        }
        return false;
    }

    void FancyZonesData::SetActiveZoneSet(const std::wstring& uniqueID, const std::wstring& uuid /*TODO(stefan): deviceId instead of uniqueId in the future*/)
    {
        if (!uuid.empty() && deviceInfoMap.find(uniqueID) != deviceInfoMap.end())
        {
            deviceInfoMap[uniqueID].activeZoneSet.uuid = uuid;
        }
    }

    void FancyZonesData::SetDeviceInfoToTmpFile(const DeviceInfoJSON& deviceInfo, const std::wstring& tmpFilePath) const
    {
        json::JsonObject deviceInfoJson = DeviceInfoJSON::ToJson(deviceInfo);
        json::to_file(tmpFilePath, deviceInfoJson);
    }

    void FancyZonesData::GetDeviceInfoFromTmpFile(const std::wstring& uniqueID, const std::wstring& tmpFilePath)
    {
        if (!deviceInfoMap.contains(uniqueID))
        {
            deviceInfoMap[uniqueID] = DeviceInfoData{ ZoneSetData{ L"", ZoneSetLayoutType::Grid, 0 }, true, 16, 3 }; // Creates entry in map when ZoneWindow is created
        }

        if (std::filesystem::exists(tmpFilePath))
        {
            auto zoneSetJson = json::from_file(tmpFilePath);
            if (zoneSetJson.has_value())
            {
                auto deviceInfo = DeviceInfoJSON::FromJson(*zoneSetJson);
                if (deviceInfo.has_value())
                {
                    deviceInfoMap[uniqueID] = deviceInfo->data;
                }
            }

            DeleteFileW(tmpFilePath.c_str());
        }
    }

    void FancyZonesData::GetCustomZoneSetFromTmpFile(const std::wstring& tmpFilePath, const std::wstring& uuid)
    {
        if (std::filesystem::exists(tmpFilePath))
        {
            auto customZoneSetJson = json::from_file(tmpFilePath);
            if (customZoneSetJson.has_value())
            {
                auto customZoneSet = CustomZoneSetJSON::FromJson(*customZoneSetJson);
                if (customZoneSet.has_value())
                {
                    customZoneSetsMap[uuid] = customZoneSet->data;
                }
            }

            DeleteFileW(tmpFilePath.c_str());
        }
    }

    bool FancyZonesData::ParseAppZoneHistory(const json::JsonObject& fancyZonesDataJSON)
    {
        try
        {
            auto appLastZones = fancyZonesDataJSON.GetNamedArray(L"app-zone-history");

            for (int i = 0; i < appLastZones.Size(); ++i)
            {
                json::JsonObject appLastZone = appLastZones.GetObjectAt(i);
                auto appZoneHistory = AppZoneHistoryJSON::FromJson(appLastZone);
                if (appZoneHistory.has_value())
                {
                    appZoneHistoryMap[appZoneHistory->appPath] = appZoneHistory->data;
                }
                else
                {
                    return false;
                }
            }

            return true;
        }
        catch (const winrt::hresult_error&)
        {
            return false;
        }
    }

    json::JsonArray FancyZonesData::SerializeAppZoneHistory() const
    {
        json::JsonArray appHistoryArray;

        int i = 0;
        for (const auto& [appPath, appZoneHistoryData] : appZoneHistoryMap)
        {
            appHistoryArray.Append(AppZoneHistoryJSON::ToJson(AppZoneHistoryJSON{ appPath, appZoneHistoryData }));
        }

        return appHistoryArray;
    }

    bool FancyZonesData::ParseDeviceInfos(const json::JsonObject& fancyZonesDataJSON)
    {
        try
        {
            auto devices = fancyZonesDataJSON.GetNamedArray(L"devices");

            for (int i = 0; i < devices.Size(); ++i)
            {
                auto device = DeviceInfoJSON::DeviceInfoJSON::FromJson(devices.GetObjectAt(i));
                if (device.has_value())
                {
                    deviceInfoMap[device->deviceId] = device->data;
                }
                else
                {
                    return false;
                }
            }

            return true;
        }
        catch (const winrt::hresult_error&)
        {
            return false;
        }
    }

    json::JsonArray FancyZonesData::SerializeDeviceInfos() const
    {
        json::JsonArray DeviceInfosJSON{};

        int i = 0;
        for (const auto& [deviceID, deviceData] : deviceInfoMap)
        {
            DeviceInfosJSON.Append(DeviceInfoJSON::DeviceInfoJSON::ToJson(DeviceInfoJSON{ deviceID, deviceData }));
        }

        return DeviceInfosJSON;
    }

    bool FancyZonesData::ParseCustomZoneSets(const json::JsonObject& fancyZonesDataJSON)
    {
        try
        {
            auto customZoneSets = fancyZonesDataJSON.GetNamedArray(L"custom-zone-sets");

            for (int i = 0; i < customZoneSets.Size(); ++i)
            {
                auto zoneSet = CustomZoneSetJSON::FromJson(customZoneSets.GetObjectAt(i));
                if (zoneSet.has_value())
                {
                    customZoneSetsMap[zoneSet->uuid] = zoneSet->data;
                }
            }

            return true;
        }
        catch (const winrt::hresult_error& e)
        {
            return false;
        }
    }

    json::JsonArray FancyZonesData::SerializeCustomZoneSets() const
    {
        json::JsonArray CustomZoneSetsJSON{};

        int i = 0;
        for (const auto& [zoneSetId, zoneSetData] : customZoneSetsMap)
        {
            CustomZoneSetsJSON.Append(CustomZoneSetJSON::ToJson(CustomZoneSetJSON{ zoneSetId, zoneSetData }));
        }

        return CustomZoneSetsJSON;
    }

    void FancyZonesData::LoadFancyZonesData()
    {
        std::wstring jsonFilePath = GetPersistFancyZonesJSONPath();

        if (!std::filesystem::exists(jsonFilePath))
        {
            MigrateCustomZoneSetsFromRegistry(); // Custom zone sets have to be migrated before applied zone sets!
            MigrateAppZoneHistoryFromRegistry();
            MigrateDeviceInfoFromRegistry();

            SaveFancyZonesData();
        }
        else
        {
            json::JsonObject fancyZonesDataJSON = GetPersistFancyZonesJSON();

            ParseAppZoneHistory(fancyZonesDataJSON);
            ParseDeviceInfos(fancyZonesDataJSON);
            ParseCustomZoneSets(fancyZonesDataJSON);
        }
    }

    void FancyZonesData::SaveFancyZonesData() const
    {
        json::JsonObject root{};

        root.SetNamedValue(L"app-zone-history", SerializeAppZoneHistory());
        root.SetNamedValue(L"devices", SerializeDeviceInfos());
        root.SetNamedValue(L"custom-zone-sets", SerializeCustomZoneSets());

        json::to_file(jsonFilePath, root);
    }

    void FancyZonesData::TmpMigrateAppliedZoneSetsFromRegistry(TAppliedZoneSetsMap& appliedZoneSetMap)
    {
        std::wregex ex(L"^[0-9]{3,4}_[0-9]{3,4}$");

        wchar_t key[256];
        StringCchPrintf(key, ARRAYSIZE(key), L"%s", RegistryHelpers::REG_SETTINGS);
        HKEY hkey;
        if (RegOpenKeyExW(HKEY_CURRENT_USER, key, 0, KEY_ALL_ACCESS, &hkey) == ERROR_SUCCESS)
        {
            wchar_t resolutionKey[256]{};
            DWORD resolutionKeyLength = ARRAYSIZE(resolutionKey);
            DWORD i = 0;
            while (RegEnumKeyW(hkey, i++, resolutionKey, resolutionKeyLength) == ERROR_SUCCESS)
            {
                std::wstring resolution{ resolutionKey };
                wchar_t appliedZoneSetskey[256];
                StringCchPrintf(appliedZoneSetskey, ARRAYSIZE(appliedZoneSetskey), L"%s\\%s", RegistryHelpers::REG_SETTINGS, resolutionKey);
                HKEY appliedZoneSetsHkey;
                if (std::regex_match(resolution, ex) && RegOpenKeyExW(HKEY_CURRENT_USER, appliedZoneSetskey, 0, KEY_ALL_ACCESS, &appliedZoneSetsHkey) == ERROR_SUCCESS)
                {
                    ZoneSetPersistedData data;
                    DWORD dataSize = sizeof(data);
                    wchar_t value[256]{};
                    DWORD valueLength = ARRAYSIZE(value);
                    DWORD i = 0;
                    while (RegEnumValueW(appliedZoneSetsHkey, i++, value, &valueLength, nullptr, nullptr, reinterpret_cast<BYTE*>(&data), &dataSize) == ERROR_SUCCESS)
                    {
                        ZoneSetData appliedZoneSetData;
                        appliedZoneSetData.type = TypeFromLayoutId(data.LayoutId);
                        if (appliedZoneSetData.type != ZoneSetLayoutType::Custom)
                        {
                            appliedZoneSetData.zoneCount = data.ZoneCount;
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

    void FancyZonesData::MigrateAppZoneHistoryFromRegistry()
    {
        HMONITOR buffer[10];
        Monitors monitors;
        monitors.data = buffer;
        monitors.count = 0;
        EnumDisplayMonitors(NULL, NULL, &MyInfoEnumProc, reinterpret_cast<LPARAM>(&monitors));

        for (int i = 0; i < monitors.count; i++)
        {
            HMONITOR monitor = monitors.data[i];
            wchar_t key[256];
            StringCchPrintf(key, ARRAYSIZE(key), L"%s\\%s\\%x", RegistryHelpers::REG_SETTINGS, RegistryHelpers::APP_ZONE_HISTORY_SUBKEY, monitor);
            HKEY hkey;
            if (RegOpenKeyExW(HKEY_CURRENT_USER, key, 0, KEY_ALL_ACCESS, &hkey) == ERROR_SUCCESS)
            {
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

    void FancyZonesData::MigrateDeviceInfoFromRegistry()
    {
        TAppliedZoneSetsMap appliedZoneSets;
        TmpMigrateAppliedZoneSetsFromRegistry(appliedZoneSets);

        wchar_t key[256];
        StringCchPrintf(key, ARRAYSIZE(key), L"%s", RegistryHelpers::REG_SETTINGS);
        HKEY hkey;
        if (RegOpenKeyExW(HKEY_CURRENT_USER, key, 0, KEY_ALL_ACCESS, &hkey) == ERROR_SUCCESS)
        {
            wchar_t value[256]{};
            DWORD valueLength = ARRAYSIZE(value);
            DWORD i = 0;
            while (RegEnumKeyW(hkey, i++, value, valueLength) == ERROR_SUCCESS)
            {
                std::wstring uniqueID{ value };
                if (uniqueID.find(L"0000-0000-0000") != std::wstring::npos)
                { //TODO(stefan): This is ugly!! Hope Andrey will resolve this with deviceID
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

                    deviceInfoMap[uniqueID] = DeviceInfoData{ appliedZoneSets[std::wstring{ activeZoneSetId }], static_cast<bool>(showSpacing), static_cast<int>(spacing), static_cast<int>(zoneCount) };

                    valueLength = ARRAYSIZE(value);
                }
            }
        }
    }

    void FancyZonesData::MigrateCustomZoneSetsFromRegistry()
    {
        wchar_t key[256];
        StringCchPrintf(key, ARRAYSIZE(key), L"%s\\%s", RegistryHelpers::REG_SETTINGS, L"Layouts");
        HKEY hkey;
        if (RegOpenKeyExW(HKEY_CURRENT_USER, key, 0, KEY_ALL_ACCESS, &hkey) == ERROR_SUCCESS)
        {
            BYTE data[256];
            DWORD dataSize = ARRAYSIZE(data);
            wchar_t value[256]{};
            DWORD valueLength = ARRAYSIZE(value);
            DWORD i = 0;
            while (RegEnumValueW(hkey, i++, value, &valueLength, nullptr, nullptr, reinterpret_cast<BYTE*>(&data), &dataSize) == ERROR_SUCCESS)
            {
                CustomZoneSetData zoneSetData;
                zoneSetData.name = std::wstring{ value };
                zoneSetData.type = static_cast<CustomLayoutType>(data[2]);
                // int version =  data[0] * 256 + data[1]; - Not used anymore

                //TODO(stefan): We need this layoutID for migration. It's unique for custom zone sets.
                //Should we use this for id or just for migration (won't be present in .json file!) and generate UUID
                std::wstring uuid = std::to_wstring(data[3] * 256 + data[4]);
                switch (zoneSetData.type)
                {
                case CustomLayoutType::Grid: {
                    GridLayoutInfo zoneSetInfo;
                    int j = 5;

                    zoneSetInfo.rows = data[j++];
                    zoneSetInfo.columns = data[j++];

                    for (int row = 0; row < zoneSetInfo.rows; row++)
                    {
                        zoneSetInfo.rowsPercents[row] = data[j++] * 256 + data[j++];
                    }

                    for (int col = 0; col < zoneSetInfo.columns; col++)
                    {
                        zoneSetInfo.columnsPercents[col] = data[j++] * 256 + data[j++];
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
                case CustomLayoutType::Canvas: {
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

    json::JsonObject ZoneSetData::ToJson(const ZoneSetData& zoneSet)
    {
        json::JsonObject result{};

        result.SetNamedValue(L"uuid", json::value(zoneSet.uuid));
        result.SetNamedValue(L"type", json::value(TypeToString(zoneSet.type)));
        if (zoneSet.type != ZoneSetLayoutType::Custom)
        {
            result.SetNamedValue(L"zone-count", json::value(*zoneSet.zoneCount));
        }

        return result;
    }

    std::optional<ZoneSetData> ZoneSetData::FromJson(const json::JsonObject& zoneSet)
    {
        try
        {
            ZoneSetData zoneSetData;

            zoneSetData.uuid = zoneSet.GetNamedString(L"uuid");
            zoneSetData.type = TypeFromString(std::wstring{ zoneSet.GetNamedString(L"type") });
            if (zoneSetData.type != ZoneSetLayoutType::Custom)
            {
                zoneSetData.zoneCount = zoneSet.GetNamedNumber(L"zone-count");
            }

            return zoneSetData;
        }
        catch (const winrt::hresult_error& e)
        {
            return std::nullopt;
        }
    }

    json::JsonObject AppZoneHistoryJSON::ToJson(const AppZoneHistoryJSON& appZoneHistory)
    {
        json::JsonObject result{};

        result.SetNamedValue(L"app-path", json::value(appZoneHistory.appPath));
        result.SetNamedValue(L"zoneset-uuid", json::value(appZoneHistory.data.zoneSetUuid));
        result.SetNamedValue(L"zone-index", json::value(appZoneHistory.data.zoneIndex));

        return result;
    }

    std::optional<AppZoneHistoryJSON> AppZoneHistoryJSON::FromJson(const json::JsonObject& zoneSet)
    {
        try
        {
            AppZoneHistoryJSON result;

            result.appPath = zoneSet.GetNamedString(L"app-path");
            result.data.zoneSetUuid = zoneSet.GetNamedString(L"zoneset-uuid");
            result.data.zoneIndex = zoneSet.GetNamedNumber(L"zone-index");

            return result;
        }
        catch (const winrt::hresult_error& e)
        {
            return std::nullopt;
        }
    }

    json::JsonObject DeviceInfoJSON::ToJson(const DeviceInfoJSON& device)
    {
        json::JsonObject result{};

        result.SetNamedValue(L"device-id", json::value(device.deviceId));
        result.SetNamedValue(L"active-zoneset", ZoneSetData::ToJson(device.data.activeZoneSet));
        result.SetNamedValue(L"editor-show-spacing", json::value(device.data.showSpacing));
        result.SetNamedValue(L"editor-spacing", json::value(device.data.spacing));
        result.SetNamedValue(L"editor-zone-count", json::value(device.data.zoneCount));

        return result;
    }

    std::optional<DeviceInfoJSON> DeviceInfoJSON::FromJson(const json::JsonObject& device)
    {
        try
        {
            DeviceInfoJSON result;

            result.deviceId = device.GetNamedString(L"device-id");

            auto zoneSet = ZoneSetData::FromJson(device.GetNamedObject(L"active-zoneset"));
            if (zoneSet.has_value())
            {
                result.data.activeZoneSet = *zoneSet;
            }
            else
            {
                return std::nullopt;
            }

            result.data.showSpacing = device.GetNamedBoolean(L"editor-show-spacing");
            result.data.spacing = device.GetNamedNumber(L"editor-spacing");
            result.data.zoneCount = device.GetNamedNumber(L"editor-zone-count");

            return result;
        }
        catch (const winrt::hresult_error& e)
        {
            return std::nullopt;
        }
    }

    json::JsonObject CanvasLayoutInfo::ToJson(const CanvasLayoutInfo& canvasInfo)
    {
        json::JsonObject infoJson{};
        infoJson.SetNamedValue(L"ref-width", json::value(canvasInfo.referenceWidth));
        infoJson.SetNamedValue(L"ref-height", json::value(canvasInfo.referenceHeight));
        json::JsonArray zonesJson;
        int i = 0;
        for (const auto& [x, y, width, height] : canvasInfo.zones)
        {
            json::JsonObject zoneJson;
            zoneJson.SetNamedValue(L"X", json::value(x));
            zoneJson.SetNamedValue(L"Y", json::value(y));
            zoneJson.SetNamedValue(L"width", json::value(width));
            zoneJson.SetNamedValue(L"height", json::value(height));
            zonesJson.Append(zoneJson);
        }
        infoJson.SetNamedValue(L"zones", zonesJson);
        return infoJson;
    }

    std::optional<CanvasLayoutInfo> CanvasLayoutInfo::FromJson(const json::JsonObject& infoJson)
    {
        try
        {
            CanvasLayoutInfo info;
            info.referenceWidth = infoJson.GetNamedNumber(L"ref-width");
            info.referenceHeight = infoJson.GetNamedNumber(L"ref-height");
            json::JsonArray zonesJson = infoJson.GetNamedArray(L"zones");
            for (int i = 0; i < zonesJson.Size(); ++i)
            {
                json::JsonObject zoneJson = zonesJson.GetObjectAt(i);
                CanvasLayoutInfo::Rect zone{ zoneJson.GetNamedNumber(L"X"), zoneJson.GetNamedNumber(L"Y"), zoneJson.GetNamedNumber(L"width"), zoneJson.GetNamedNumber(L"height") };
                info.zones.push_back(zone);
            }
            return info;
        }
        catch (const winrt::hresult_error& e)
        {
            return std::nullopt;
        }
    }

    json::JsonObject GridLayoutInfo::ToJson(const GridLayoutInfo& gridInfo)
    {
        json::JsonObject infoJson;
        infoJson.SetNamedValue(L"rows", json::value(gridInfo.rows));
        infoJson.SetNamedValue(L"columns", json::value(gridInfo.columns));

        json::JsonArray rowsPercentageJson;
        for (const auto& rowsPercentsElem : gridInfo.rowsPercents)
        {
            rowsPercentageJson.Append(json::value(rowsPercentsElem));
        }
        infoJson.SetNamedValue(L"rows-percentage", rowsPercentageJson);

        json::JsonArray columnPercentageJson;
        for (const auto& columnsPercentsElem : gridInfo.columnsPercents)
        {
            columnPercentageJson.Append(json::value(columnsPercentsElem));
        }
        infoJson.SetNamedValue(L"columns-percentage", columnPercentageJson);

        json::JsonArray cellChildMapJson;
        for (const auto& cellChildMapRow : gridInfo.cellChildMap)
        {
            json::JsonArray cellChildMapRowJson;
            for (const auto& cellChildMapRowElem : cellChildMapRow)
            {
                cellChildMapRowJson.Append(json::value(cellChildMapRowElem));
            }
            cellChildMapJson.Append(cellChildMapRowJson);
        }
        infoJson.SetNamedValue(L"cell-child-map", cellChildMapJson);

        return infoJson;
    }

    std::optional<GridLayoutInfo> GridLayoutInfo::FromJson(const json::JsonObject& infoJson)
    {
        try
        {
            GridLayoutInfo info{};
            
            info.rows = infoJson.GetNamedNumber(L"rows");
            info.columns = infoJson.GetNamedNumber(L"columns");

            json::JsonArray rowsPercentage = infoJson.GetNamedArray(L"rows-percentage");
            json::JsonArray columnsPercentage = infoJson.GetNamedArray(L"columns-percentage");
            json::JsonArray cellChildMap = infoJson.GetNamedArray(L"cell-child-map");

            const uint32_t rowsSize = rowsPercentage.Size();
            for (int i = 0; i < rowsSize && i < MAX_ZONE_COUNT; i++)
            {
                info.rowsPercents[i] = rowsPercentage.GetAt(i).GetNumber();
            }
            
            const uint32_t columnsSize = columnsPercentage.Size();
            for (int i = 0; i < columnsSize && i < MAX_ZONE_COUNT; i++)
            {
                info.columnsPercents[i] = columnsPercentage.GetAt(i).GetNumber();
            }
            
            const uint32_t cellsRowsSize = cellChildMap.Size();
            for (int i = 0; i < cellsRowsSize && i < MAX_ZONE_COUNT; i++)
            {
                auto mapRow = cellChildMap.GetAt(i).GetArray();
                const uint32_t cellsColumnsSize = mapRow.Size();
                for (int j = 0; j < cellsColumnsSize && j < MAX_ZONE_COUNT; j++)
                {
                    info.cellChildMap[i][j] = mapRow.GetAt(j).GetNumber();
                }
            }

            return info;
        }
        catch (const winrt::hresult_error& e)
        {
            return std::nullopt;
        }
    }

    json::JsonObject CustomZoneSetJSON::ToJson(const CustomZoneSetJSON& customZoneSet)
    {
        json::JsonObject result{};

        result.SetNamedValue(L"uuid", json::value(customZoneSet.uuid));
        result.SetNamedValue(L"name", json::value(customZoneSet.data.name));
        switch (customZoneSet.data.type)
        {
        case CustomLayoutType::Canvas: {
            result.SetNamedValue(L"type", json::value(L"canvas"));

            CanvasLayoutInfo info = std::get<CanvasLayoutInfo>(customZoneSet.data.info);
            result.SetNamedValue(L"info", CanvasLayoutInfo::ToJson(info));

            break;
        }
        case CustomLayoutType::Grid: {
            result.SetNamedValue(L"type", json::value(L"grid"));

            GridLayoutInfo gridInfo = std::get<GridLayoutInfo>(customZoneSet.data.info);
            result.SetNamedValue(L"info", GridLayoutInfo::ToJson(gridInfo));

            break;
        }
        }

        return result;
    }

    std::optional<CustomZoneSetJSON> CustomZoneSetJSON::FromJson(const json::JsonObject& customZoneSet)
    {
        try
        {
            CustomZoneSetJSON result;

            result.uuid = customZoneSet.GetNamedString(L"uuid");
            result.data.name = customZoneSet.GetNamedString(L"name");

            json::JsonObject infoJson = customZoneSet.GetNamedObject(L"info");
            std::wstring zoneSetType = std::wstring{ customZoneSet.GetNamedString(L"type") };
            if (zoneSetType.compare(L"canvas") == 0)
            {
                auto info = CanvasLayoutInfo::FromJson(infoJson);
                if (info.has_value())
                {
                    result.data.type = CustomLayoutType::Canvas;
                    result.data.info = *info;
                }
                else
                {
                    return std::nullopt;
                }
            }
            else if (zoneSetType.compare(L"grid") == 0)
            {
                auto info = GridLayoutInfo::FromJson(infoJson);
                if (info.has_value())
                {
                    result.data.type = CustomLayoutType::Grid;
                    result.data.info = *info;
                }
                else
                {
                    return std::nullopt;
                }
            }
            else
            {
                return std::nullopt;
            }

            return result;
        }
        catch (const winrt::hresult_error& e)
        {
            return std::nullopt;
        }
    }
}