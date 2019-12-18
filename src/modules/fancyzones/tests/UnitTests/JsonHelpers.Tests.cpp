#include "pch.h"
#include <filesystem>

#include <lib/JsonHelpers.h>

#include <CppUnitTestLogger.h>

using namespace JSONHelpers;
using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace FancyZonesUnitTests
{
    void compareJsonObjects(const json::JsonObject& expected, const json::JsonObject& actual, bool recursive = true)
    {
        auto iter = expected.First();
        while (iter.HasCurrent())
        {
            const auto key = iter.Current().Key();
            Assert::IsTrue(actual.HasKey(key), key.c_str());
            
            const std::wstring expectedStringified = iter.Current().Value().Stringify().c_str();
            const std::wstring actualStringified = actual.GetNamedValue(key).Stringify().c_str();

            if (recursive)
            {
                json::JsonObject expectedJson;
                if (json::JsonObject::TryParse(expectedStringified, expectedJson))
                {
                    json::JsonObject actualJson;
                    if (json::JsonObject::TryParse(actualStringified, actualJson))
                    {
                        compareJsonObjects(expectedJson, actualJson, true);
                    }
                    else
                    {
                        Assert::IsTrue(false, key.c_str());
                    }
                }
                else
                {
                    Assert::AreEqual(expectedStringified, actualStringified, key.c_str());
                }
            }
            else
            {
                Assert::AreEqual(expectedStringified, actualStringified, key.c_str());
            }

            iter.MoveNext();
        }
    }

    TEST_CLASS(ZoneSetLayoutTypeUnitTest)
    {

        TEST_METHOD(ZoneSetLayoutTypeToString)
        {
            std::map<int, std::wstring> expectedMap = {
                std::make_pair(-1, L"TypeToString_ERROR"),
                std::make_pair(0, L"focus"),
                std::make_pair(1, L"columns"),
                std::make_pair(2, L"rows"),
                std::make_pair(3, L"grid"),
                std::make_pair(4, L"priority-grid"),
                std::make_pair(5, L"custom"),
                std::make_pair(6, L"TypeToString_ERROR"),
            };

            for (const auto& expected : expectedMap)
            {
                auto actual = JSONHelpers::TypeToString(static_cast<ZoneSetLayoutType>(expected.first));
                Assert::AreEqual(expected.second, actual);
            }
        }

        TEST_METHOD(ZoneSetLayoutTypeFromString)
        {
            std::map<ZoneSetLayoutType, std::wstring> expectedMap = {
                std::make_pair(ZoneSetLayoutType::Focus, L"focus"),
                std::make_pair(ZoneSetLayoutType::Columns, L"columns"),
                std::make_pair(ZoneSetLayoutType::Rows, L"rows"),
                std::make_pair(ZoneSetLayoutType::Grid, L"grid"),
                std::make_pair(ZoneSetLayoutType::PriorityGrid, L"priority-grid"),
                std::make_pair(ZoneSetLayoutType::Custom, L"custom"),
            };

            for (const auto& expected : expectedMap)
            {
                auto actual = JSONHelpers::TypeFromString(expected.second);
                Assert::AreEqual(static_cast<int>(expected.first), static_cast<int>(actual));
            }
        }

        TEST_METHOD(ZoneSetLayoutTypeToLayoutId)
        {
            std::map<int, int> expectedMap = {
                std::make_pair(-1, -1),
                std::make_pair(0, 0xFFFF),
                std::make_pair(1, 0xFFFD),
                std::make_pair(2, 0xFFFE),
                std::make_pair(3, 0xFFFC),
                std::make_pair(4, 0xFFFB),
                std::make_pair(5, 0),
                std::make_pair(6, -1),
            };

            for (const auto& expected : expectedMap)
            {
                auto actual = JSONHelpers::TypeToLayoutId(static_cast<ZoneSetLayoutType>(expected.first));
                Assert::AreEqual(expected.second, actual);
            }
        }

        TEST_METHOD(ZoneSetLayoutTypeFromLayoutId)
        {
            std::map<ZoneSetLayoutType, int> expectedMap = {
                std::make_pair(ZoneSetLayoutType::Focus, 0xFFFF),
                std::make_pair(ZoneSetLayoutType::Columns, 0xFFFD),
                std::make_pair(ZoneSetLayoutType::Rows, 0xFFFE),
                std::make_pair(ZoneSetLayoutType::Grid, 0xFFFC),
                std::make_pair(ZoneSetLayoutType::PriorityGrid, 0xFFFB),
                std::make_pair(ZoneSetLayoutType::Custom, 0xFFFA),
                std::make_pair(ZoneSetLayoutType::Custom, 0),
                std::make_pair(ZoneSetLayoutType::Custom, -1),
            };

            for (const auto& expected : expectedMap)
            {
                auto actual = JSONHelpers::TypeFromLayoutId(expected.second);
                Assert::AreEqual(static_cast<int>(expected.first), static_cast<int>(actual));
            }
        }
    };

    TEST_CLASS(CanvasLayoutInfoUnitTests)
    {
        json::JsonObject m_json = json::JsonObject::Parse(L"{\"ref-width\": 123, \"ref-height\": 321, \"zones\": [{\"X\": 11, \"Y\": 22, \"width\": 33, \"height\": 44}, {\"X\": 55, \"Y\": 66, \"width\": 77, \"height\": 88}]}");

        TEST_METHOD(ToJson)
        {
            CanvasLayoutInfo info;
            info.referenceWidth = 123;
            info.referenceHeight = 321;
            info.zones = { CanvasLayoutInfo::Rect{ 11, 22, 33, 44 }, CanvasLayoutInfo::Rect{ 55, 66, 77, 88 } };

            auto actual = CanvasLayoutInfo::ToJson(info);
            compareJsonObjects(m_json, actual);
        }

        TEST_METHOD(FromJson)
        {
            CanvasLayoutInfo expected;
            expected.referenceWidth = 123;
            expected.referenceHeight = 321;
            expected.zones = { CanvasLayoutInfo::Rect{ 11, 22, 33, 44 }, CanvasLayoutInfo::Rect{ 55, 66, 77, 88 } };

            auto actual = CanvasLayoutInfo::FromJson(m_json);
            Assert::AreEqual(expected.referenceHeight, actual.referenceHeight);
            Assert::AreEqual(expected.referenceWidth, actual.referenceWidth);
            Assert::AreEqual(expected.zones.size(), actual.zones.size());
            for (int i = 0; i < expected.zones.size(); i++)
            {
                Assert::AreEqual(expected.zones[i].x, actual.zones[i].x);
                Assert::AreEqual(expected.zones[i].y, actual.zones[i].y);
                Assert::AreEqual(expected.zones[i].width, actual.zones[i].width);
                Assert::AreEqual(expected.zones[i].height, actual.zones[i].height);
            }
        }
    };

    TEST_CLASS(GridLayoutInfoUnitTests)
    {
        TEST_METHOD(ToJson)
        {
            json::JsonObject expected = json::JsonObject::Parse(L"{\"rows\": 1, \"columns\": 2}");
            GridLayoutInfo info;
            info.rows = 1;
            info.columns = 2;

            json::JsonArray rowsArray, columnsArray, cells;
            for (int i = 0; i < MAX_ZONE_COUNT; i++)
            {
                int row = rand() % 100;
                rowsArray.Append(json::JsonValue::CreateNumberValue(row));
                info.rowsPercents[i] = row;
                
                int column = rand() % 100;
                columnsArray.Append(json::JsonValue::CreateNumberValue(column));
                info.columnsPercents[i] = column;

                json::JsonArray cellsArray;
                for (int j = 0; j < MAX_ZONE_COUNT; j++)
                {
                    int cell = rand() % 100;
                    info.cellChildMap[i][j] = cell;
                    cellsArray.Append(json::JsonValue::CreateNumberValue(cell));
                }
                cells.Append(cellsArray);
            }

            expected.SetNamedValue(L"rows-percentage", rowsArray);
            expected.SetNamedValue(L"columns-percentage", columnsArray);
            expected.SetNamedValue(L"cell-child-map", cells);
            
            auto actual = GridLayoutInfo::ToJson(info);
            compareJsonObjects(expected, actual);
        }

        TEST_METHOD(FromJson)
        {
            json::JsonObject json = json::JsonObject::Parse(L"{\"rows\": 1, \"columns\": 2}");
            GridLayoutInfo expected;
            expected.rows = 1;
            expected.columns = 2;

            json::JsonArray rowsArray, columnsArray, cells;
            for (int i = 0; i < MAX_ZONE_COUNT; i++)
            {
                int row = rand() % 100;
                rowsArray.Append(json::JsonValue::CreateNumberValue(row));
                expected.rowsPercents[i] = row;

                int column = rand() % 100;
                columnsArray.Append(json::JsonValue::CreateNumberValue(column));
                expected.columnsPercents[i] = column;

                json::JsonArray cellsArray;
                for (int j = 0; j < MAX_ZONE_COUNT; j++)
                {
                    int cell = rand() % 100;
                    expected.cellChildMap[i][j] = cell;
                    cellsArray.Append(json::JsonValue::CreateNumberValue(cell));
                }
                cells.Append(cellsArray);
            }

            json.SetNamedValue(L"rows-percentage", rowsArray);
            json.SetNamedValue(L"columns-percentage", columnsArray);
            json.SetNamedValue(L"cell-child-map", cells);
            
            auto actual = GridLayoutInfo::FromJson(json);
            Assert::AreEqual(expected.rows, actual.rows);
            Assert::AreEqual(expected.columns, actual.columns);
            for (int i = 0; i < MAX_ZONE_COUNT; i++)
            {
                Assert::AreEqual(expected.rowsPercents[i], actual.rowsPercents[i]);
                Assert::AreEqual(expected.columnsPercents[i], actual.columnsPercents[i]);
                for (int j = 0; j < MAX_ZONE_COUNT; j++)
                {
                    Assert::AreEqual(expected.cellChildMap[i][j], actual.cellChildMap[i][j]);
                }
            }
        }

        TEST_METHOD(FromJsonEmptyArray)
        {
            json::JsonObject json = json::JsonObject::Parse(L"{\"rows\": 1, \"columns\": 2}");
            GridLayoutInfo expected;
            expected.rows = 1;
            expected.columns = 2;

            json::JsonArray rowsArray, columnsArray, cells;

            json.SetNamedValue(L"rows-percentage", rowsArray);
            json.SetNamedValue(L"columns-percentage", columnsArray);
            json.SetNamedValue(L"cell-child-map", cells);

            auto actual = GridLayoutInfo::FromJson(json);
            Assert::AreEqual(expected.rows, actual.rows);
            Assert::AreEqual(expected.columns, actual.columns);
            for (int i = 0; i < MAX_ZONE_COUNT; i++)
            {
                Assert::AreEqual(expected.rowsPercents[i], actual.rowsPercents[i]);
                Assert::AreEqual(expected.columnsPercents[i], actual.columnsPercents[i]);
                for (int j = 0; j < MAX_ZONE_COUNT; j++)
                {
                    Assert::AreEqual(expected.cellChildMap[i][j], actual.cellChildMap[i][j]);
                }
            }
        }

        TEST_METHOD(FromJsonSmallerArray)
        {
            json::JsonObject json = json::JsonObject::Parse(L"{\"rows\": 1, \"columns\": 2}");
            GridLayoutInfo expected;
            expected.rows = 1;
            expected.columns = 2;

            json::JsonArray rowsArray, columnsArray, cells;
            for (int i = 0; i < MAX_ZONE_COUNT - 5; i++)
            {
                int row = rand() % 100;
                rowsArray.Append(json::JsonValue::CreateNumberValue(row));
                expected.rowsPercents[i] = row;

                int column = rand() % 100;
                columnsArray.Append(json::JsonValue::CreateNumberValue(column));
                expected.columnsPercents[i] = column;

                json::JsonArray cellsArray;
                for (int j = 0; j < MAX_ZONE_COUNT; j++)
                {
                    int cell = rand() % 100;
                    expected.cellChildMap[i][j] = cell;
                    cellsArray.Append(json::JsonValue::CreateNumberValue(cell));
                }
                cells.Append(cellsArray);
            }

            json.SetNamedValue(L"rows-percentage", rowsArray);
            json.SetNamedValue(L"columns-percentage", columnsArray);
            json.SetNamedValue(L"cell-child-map", cells);

            auto actual = GridLayoutInfo::FromJson(json);
            Assert::AreEqual(expected.rows, actual.rows);
            Assert::AreEqual(expected.columns, actual.columns);
            for (int i = 0; i < MAX_ZONE_COUNT; i++)
            {
                Assert::AreEqual(expected.rowsPercents[i], actual.rowsPercents[i]);
                Assert::AreEqual(expected.columnsPercents[i], actual.columnsPercents[i]);
                for (int j = 0; j < MAX_ZONE_COUNT; j++)
                {
                    Assert::AreEqual(expected.cellChildMap[i][j], actual.cellChildMap[i][j]);
                }
            }
        }

        TEST_METHOD(FromJsonBiggerArray)
        {
            json::JsonObject json = json::JsonObject::Parse(L"{\"rows\": 1, \"columns\": 2}");
            GridLayoutInfo expected;
            expected.rows = 1;
            expected.columns = 2;

            json::JsonArray rowsArray, columnsArray, cells;
            for (int i = 0; i < MAX_ZONE_COUNT; i++)
            {
                rowsArray.Append(json::JsonValue::CreateNumberValue(1));
                expected.rowsPercents[i] = 1;

                columnsArray.Append(json::JsonValue::CreateNumberValue(1));
                expected.columnsPercents[i] = 1;

                json::JsonArray cellsArray;
                for (int j = 0; j < MAX_ZONE_COUNT; j++)
                {
                    expected.cellChildMap[i][j] = 1;
                    cellsArray.Append(json::JsonValue::CreateNumberValue(1));
                }
                cells.Append(cellsArray);
            }

            //extra
            for (int i = 0; i < 5; i++)
            {
                rowsArray.Append(json::JsonValue::CreateNumberValue(2));
                columnsArray.Append(json::JsonValue::CreateNumberValue(2));
                
                json::JsonArray cellsArray;
                for (int j = 0; j < 5; j++)
                {
                    cellsArray.Append(json::JsonValue::CreateNumberValue(2));
                }
                cells.Append(cellsArray);
            }

            json.SetNamedValue(L"rows-percentage", rowsArray);
            json.SetNamedValue(L"columns-percentage", columnsArray);
            json.SetNamedValue(L"cell-child-map", cells);

            auto actual = GridLayoutInfo::FromJson(json);
            Assert::AreEqual(expected.rows, actual.rows);
            Assert::AreEqual(expected.columns, actual.columns);
            for (int i = 0; i < MAX_ZONE_COUNT; i++)
            {
                Assert::AreEqual(expected.rowsPercents[i], actual.rowsPercents[i]);
                Assert::AreEqual(expected.columnsPercents[i], actual.columnsPercents[i]);
                for (int j = 0; j < MAX_ZONE_COUNT; j++)
                {
                    Assert::AreEqual(expected.cellChildMap[i][j], actual.cellChildMap[i][j]);
                }
            }
        }
    };

    TEST_CLASS(FancyZonesDataUnitTests)
    {
    private:
        const std::wstring m_defaultCustomDeviceStr = L"{\"device-id\": \"default_device_id\", \"active-zoneset\": {\"type\": \"custom\", \"uuid\": \"uuid\"}, \"editor-show-spacing\": true, \"editor-spacing\": 16, \"editor-zone-count\": 3}";
        const json::JsonValue m_defaultCustomDeviceValue = json::JsonValue::Parse(m_defaultCustomDeviceStr);
        const json::JsonObject m_defaultCustomDeviceObj = json::JsonObject::Parse(m_defaultCustomDeviceStr);

        void compareJsonArrays(const json::JsonArray& expected, const json::JsonArray& actual)
        {
            Assert::AreEqual(expected.Size(), actual.Size());
            for (int i = 0; i < expected.Size(); i++)
            {
                compareJsonObjects(expected.GetObjectAt(i), actual.GetObjectAt(i));
            }
        }

    public:
        TEST_METHOD(FancyZonesDataPath)
        {
            FancyZonesData data;
            Assert::IsFalse(data.GetPersistFancyZonesJSONPath().empty());
        }

        TEST_METHOD(FancyZonesDataJsonEmpty)
        {
            FancyZonesData data;
            const auto jsonPath = data.GetPersistFancyZonesJSONPath();
            auto savedJson = json::from_file(jsonPath);

            if (std::filesystem::exists(jsonPath))
            {
                std::filesystem::remove(jsonPath);
            }

            json::JsonObject expected;
            auto actual = data.GetPersistFancyZonesJSON();

            Assert::AreEqual(expected.Stringify().c_str(), actual.Stringify().c_str());

            if (savedJson)
            {
                json::to_file(jsonPath, *savedJson);
            }
        }

        TEST_METHOD(FancyZonesDataJson)
        {
            FancyZonesData data;
            const auto jsonPath = data.GetPersistFancyZonesJSONPath();
            auto savedJson = json::from_file(jsonPath);

            if (std::filesystem::exists(jsonPath))
            {
                std::filesystem::remove(jsonPath);
            }

            json::JsonObject expected = json::JsonObject::Parse(L"{\"fancy-zones\":{\"custom-zonesets \":[{\"uuid\":\"uuid1\",\"name\":\"Custom1\",\"type\":\"custom\" }] } }");
            json::to_file(jsonPath, expected);

            auto actual = data.GetPersistFancyZonesJSON();
            Assert::AreEqual(expected.Stringify().c_str(), actual.Stringify().c_str());

            if (savedJson)
            {
                json::to_file(jsonPath, *savedJson);
            }
            else
            {
                std::filesystem::remove(jsonPath);
            }
        }

        TEST_METHOD(FancyZonesDataDeviceInfoMap)
        {
            FancyZonesData data;
            const auto actual = data.GetDeviceInfoMap();
            Assert::IsTrue(actual.empty());
        }

        TEST_METHOD(FancyZonesDataDeviceInfoMapParseEmpty)
        {
            FancyZonesData data;

            json::JsonObject json;
            data.ParseDeviceInfos(json);

            const auto actual = data.GetDeviceInfoMap();
            Assert::IsTrue(actual.empty());
        }

        TEST_METHOD(FancyZonesDataDeviceInfoMapParseValidEmpty)
        {
            FancyZonesData data;

            json::JsonObject expected;
            json::JsonArray zoneSets;
            expected.SetNamedValue(L"devices", zoneSets);

            data.ParseDeviceInfos(expected);

            const auto actual = data.GetDeviceInfoMap();
            Assert::IsTrue(actual.empty());
        }

        TEST_METHOD(FancyZonesDataDeviceInfoMapParseInvalid)
        {
            auto iter = m_defaultCustomDeviceObj.First();
            while (iter.HasCurrent())
            {
                json::JsonArray devices;
                json::JsonObject obj = json::JsonObject::Parse(m_defaultCustomDeviceStr);
                obj.Remove(iter.Current().Key());
                devices.Append(obj);
                json::JsonObject expected;
                expected.SetNamedValue(L"devices", devices);

                auto func = [&expected] {
                    FancyZonesData data;
                    data.ParseDeviceInfos(expected);
                };
                Assert::ExpectException<winrt::hresult_error>(func);

                iter.MoveNext();
            }
        }

        TEST_METHOD(FancyZonesDataDeviceInfoMapParseSingle)
        {
            json::JsonArray devices;
            devices.Append(m_defaultCustomDeviceValue);
            json::JsonObject expected;
            expected.SetNamedValue(L"devices", devices);

            FancyZonesData data;
            data.ParseDeviceInfos(expected);

            const auto actualMap = data.GetDeviceInfoMap();
            Assert::AreEqual((size_t)1, actualMap.size());
        }

        TEST_METHOD(FancyZonesDataDeviceInfoMapParseMany)
        {
            json::JsonArray devices;
            for (int i = 0; i < 10; i++)
            {
                json::JsonObject obj = json::JsonObject::Parse(m_defaultCustomDeviceStr);
                obj.SetNamedValue(L"device-id", json::JsonValue::CreateStringValue(std::to_wstring(i)));

                Logger::WriteMessage(obj.Stringify().c_str());
                Logger::WriteMessage("\n");

                devices.Append(obj);
            }

            json::JsonObject expected;
            expected.SetNamedValue(L"devices", devices);
            Logger::WriteMessage(expected.Stringify().c_str());
            Logger::WriteMessage("\n");

            FancyZonesData data;
            data.ParseDeviceInfos(expected);

            const auto actualMap = data.GetDeviceInfoMap();
            Assert::AreEqual((size_t)10, actualMap.size());
        }

        TEST_METHOD(FancyZonesDataDeviceInfoMapParseSpacingTrue)
        {
            const auto expectedValue = true;
            json::JsonArray devices;
            json::JsonObject obj = m_defaultCustomDeviceObj;
            obj.SetNamedValue(L"editor-show-spacing", json::JsonValue::CreateBooleanValue(expectedValue));
            devices.Append(obj);

            json::JsonObject expected;
            expected.SetNamedValue(L"devices", devices);

            FancyZonesData data;
            data.ParseDeviceInfos(expected);

            const auto actual = data.GetDeviceInfoMap().begin()->second;
            Assert::AreEqual(expectedValue, actual.showSpacing);
        }

        TEST_METHOD(FancyZonesDataDeviceInfoMapParseSpacingFalse)
        {
            const auto expectedValue = false;
            json::JsonArray devices;
            json::JsonObject obj = m_defaultCustomDeviceObj;
            obj.SetNamedValue(L"editor-show-spacing", json::JsonValue::CreateBooleanValue(expectedValue));
            devices.Append(obj);

            json::JsonObject expected;
            expected.SetNamedValue(L"devices", devices);

            FancyZonesData data;
            data.ParseDeviceInfos(expected);

            const auto actual = data.GetDeviceInfoMap().begin()->second;
            Assert::AreEqual(expectedValue, actual.showSpacing);
        }

        TEST_METHOD(FancyZonesDataDeviceInfoMapParseSpacingPositive)
        {
            const auto expectedValue = 12345;
            json::JsonArray devices;
            json::JsonObject obj = m_defaultCustomDeviceObj;
            obj.SetNamedValue(L"editor-spacing", json::JsonValue::CreateNumberValue(expectedValue));
            devices.Append(obj);

            json::JsonObject expected;
            expected.SetNamedValue(L"devices", devices);

            FancyZonesData data;
            data.ParseDeviceInfos(expected);

            const auto actual = data.GetDeviceInfoMap().begin()->second;
            Assert::AreEqual(expectedValue, actual.spacing);
        }

        TEST_METHOD(FancyZonesDataDeviceInfoMapParseSpacingNegative)
        {
            const auto expectedValue = -12345;
            json::JsonArray devices;
            json::JsonObject obj = m_defaultCustomDeviceObj;
            obj.SetNamedValue(L"editor-spacing", json::JsonValue::CreateNumberValue(expectedValue));
            devices.Append(obj);

            json::JsonObject expected;
            expected.SetNamedValue(L"devices", devices);

            FancyZonesData data;
            data.ParseDeviceInfos(expected);

            const auto actual = data.GetDeviceInfoMap().begin()->second;
            Assert::AreEqual(expectedValue, actual.spacing);
        }

        TEST_METHOD(FancyZonesDataDeviceInfoMapParseZoneCountPositive)
        {
            const auto expectedValue = 12345;
            json::JsonArray devices;
            json::JsonObject obj = m_defaultCustomDeviceObj;
            obj.SetNamedValue(L"editor-zone-count", json::JsonValue::CreateNumberValue(expectedValue));
            devices.Append(obj);

            json::JsonObject expected;
            expected.SetNamedValue(L"devices", devices);

            FancyZonesData data;
            data.ParseDeviceInfos(expected);

            const auto actual = data.GetDeviceInfoMap().begin()->second;
            Assert::AreEqual(expectedValue, actual.zoneCount);
        }

        TEST_METHOD(FancyZonesDataDeviceInfoMapParseZoneCountNegative)
        {
            const auto expectedValue = -12345;
            json::JsonArray devices;
            json::JsonObject obj = m_defaultCustomDeviceObj;
            obj.SetNamedValue(L"editor-zone-count", json::JsonValue::CreateNumberValue(expectedValue));
            devices.Append(obj);

            json::JsonObject expected;
            expected.SetNamedValue(L"devices", devices);

            FancyZonesData data;
            data.ParseDeviceInfos(expected);

            const auto actual = data.GetDeviceInfoMap().begin()->second;
            Assert::AreEqual(expectedValue, actual.zoneCount);
        }

        TEST_METHOD(FancyZonesDataDeviceInfoMapParseZoneCustom)
        {
            const auto expectedValue = json::JsonValue::Parse(L"{\"type\": \"custom\", \"uuid\": \"uuid\"}");
            json::JsonArray devices;
            json::JsonObject obj = m_defaultCustomDeviceObj;
            obj.SetNamedValue(L"active-zoneset", expectedValue);
            devices.Append(obj);

            json::JsonObject expected;
            expected.SetNamedValue(L"devices", devices);

            FancyZonesData data;
            data.ParseDeviceInfos(expected);

            const auto actual = data.GetDeviceInfoMap().begin()->second.activeZoneSet;
            Assert::AreEqual(static_cast<int>(ZoneSetLayoutType::Custom), static_cast<int>(actual.type));
            Assert::AreEqual(L"uuid", actual.uuid.c_str());
            Assert::IsFalse(actual.zoneCount.has_value());
        }

        TEST_METHOD(FancyZonesDataDeviceInfoMapParseZoneGeneral)
        {
            const auto expectedValue = json::JsonValue::Parse(L"{\"type\": \"focus\", \"uuid\": \"uuid\", \"zone-count\": 10}");
            json::JsonArray devices;
            json::JsonObject obj = m_defaultCustomDeviceObj;
            obj.SetNamedValue(L"active-zoneset", expectedValue);
            devices.Append(obj);

            json::JsonObject expected;
            expected.SetNamedValue(L"devices", devices);

            FancyZonesData data;
            data.ParseDeviceInfos(expected);

            const auto actual = data.GetDeviceInfoMap().begin()->second.activeZoneSet;
            Assert::AreEqual(static_cast<int>(ZoneSetLayoutType::Focus), static_cast<int>(actual.type));
            Assert::AreEqual(L"uuid", actual.uuid.c_str());
            Assert::IsTrue(actual.zoneCount.has_value());
            Assert::AreEqual(10, *actual.zoneCount);
        }

        TEST_METHOD(FancyZonesDataSerialize)
        {
            json::JsonArray expectedDevices;
            expectedDevices.Append(m_defaultCustomDeviceObj);
            json::JsonObject expected;
            expected.SetNamedValue(L"devices", expectedDevices);

            FancyZonesData data;
            data.ParseDeviceInfos(expected);

            auto actual = data.SerializeDeviceInfos();
            compareJsonArrays(expectedDevices, actual);
        }
    };
}