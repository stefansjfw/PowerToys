#include "pch.h"
#include <filesystem>

#include <lib/JsonHelpers.h>

#include <CppUnitTestLogger.h>

using namespace JSONHelpers;
using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace FancyZonesUnitTests
{
    TEST_CLASS(ZoneSetDataUnitTest)
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

    TEST_CLASS(FancyZonesDataUnitTests)
    {
    private:
        const std::wstring m_defaultCustomDeviceStr = L"{\"device-id\": \"default_device_id\", \"active-zoneset\": {\"type\": \"custom\", \"uuid\": \"uuid\"}, \"editor-show-spacing\": true, \"editor-spacing\": 16, \"editor-zone-count\": 3}";
        const json::JsonValue m_defaultCustomDeviceValue = json::JsonValue::Parse(m_defaultCustomDeviceStr);
        const json::JsonObject m_defaultCustomDeviceObj = json::JsonObject::Parse(m_defaultCustomDeviceStr);

        void compareJsonObjects(const json::JsonObject& expected, const json::JsonObject& actual, bool recursive = true)
        {
            auto iter = expected.First();
            while (iter.HasCurrent())
            {
                const auto key = iter.Current().Key();
                Assert::IsTrue(actual.HasKey(key));

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
                            Assert::IsTrue(false);
                        }
                    }
                    else
                    {
                        Assert::AreEqual(expectedStringified, actualStringified);
                    }
                }
                else
                {
                    Assert::AreEqual(expectedStringified, actualStringified);
                }

                iter.MoveNext();
            }
        }

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