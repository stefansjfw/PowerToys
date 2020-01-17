#include "pch.h"

#include <filesystem>

#include <lib/util.h>
#include <lib/ZoneSet.h>
#include <lib/ZoneWindow.h>
#include "Util.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace FancyZonesUnitTests
{
    struct MockZoneWindowHost : public winrt::implements<MockZoneWindowHost, IZoneWindowHost>
    {
        IFACEMETHODIMP_(void)
        MoveWindowsOnActiveZoneSetChange() noexcept {};
        IFACEMETHODIMP_(COLORREF)
        GetZoneHighlightColor() noexcept
        {
            return RGB(0xFF, 0xFF, 0xFF);
        }
        IFACEMETHODIMP_(GUID)
        GetCurrentMonitorZoneSetId(HMONITOR monitor) noexcept
        {
            return m_guid;
        }
        IFACEMETHODIMP_(int)
        GetZoneHighlightOpacity() noexcept
        {
            return 100;
        }

        GUID m_guid;
    };

    TEST_CLASS(ZoneWindowUnitTests)
    {
        const std::wstring m_deviceId = L"\\\\?\\DISPLAY#DELA026#5&10a58c63&0&UID16777488#{e6f07b5f-ee97-4a90-b076-33f57bf4eaa7}";
        const std::wstring m_virtualDesktopId = L"MyVirtualDesktopId";
        std::wstringstream m_uniqueId;

        HINSTANCE m_hInst{};
        HMONITOR m_monitor{};
        MONITORINFO m_monitorInfo{};
        MockZoneWindowHost m_zoneWindowHost{};
        IZoneWindowHost* m_hostPtr = m_zoneWindowHost.get_strong().get();

        winrt::com_ptr<IZoneWindow> m_zoneWindow;

        std::wstring guidString()
        {
            GUID guid;
            Assert::AreEqual(S_OK, CoCreateGuid(&guid));

            OLECHAR* guidString;
            Assert::AreEqual(S_OK, StringFromCLSID(guid, &guidString));

            std::wstring guidStr{ guidString };
            CoTaskMemFree(guidString);

            return guidStr;
        }

        TEST_METHOD_INITIALIZE(Init)
        {
            m_hInst = (HINSTANCE)GetModuleHandleW(nullptr);

            m_monitor = MonitorFromPoint(POINT{ 0, 0 }, MONITOR_DEFAULTTOPRIMARY);
            m_monitorInfo.cbSize = sizeof(m_monitorInfo);
            Assert::AreNotEqual(0, GetMonitorInfoW(m_monitor, &m_monitorInfo));

            Assert::AreEqual(S_OK, CoCreateGuid(&m_zoneWindowHost.m_guid));

            m_uniqueId << L"DELA026#5&10a58c63&0&UID16777488_" << m_monitorInfo.rcMonitor.right << "_" << m_monitorInfo.rcMonitor.bottom << "_MyVirtualDesktopId";
        }

        TEST_METHOD_CLEANUP(Cleanup)
        {
            if (m_zoneWindow)
            {
                //cleanup if temp files were created
                std::filesystem::remove(m_zoneWindow->GetActiveZoneSetTmpPath());
                std::filesystem::remove(m_zoneWindow->GetAppliedZoneSetTmpPath());
                std::filesystem::remove(m_zoneWindow->GetCustomZoneSetsTmpPath());
                m_zoneWindow = nullptr;
            }
        }

        void testZoneWindow(winrt::com_ptr<IZoneWindow> zoneWindow)
        {
            const std::wstring expectedWorkArea = std::to_wstring(m_monitorInfo.rcMonitor.right) + L"_" + std::to_wstring(m_monitorInfo.rcMonitor.bottom);

            Assert::IsNotNull(zoneWindow.get());
            Assert::IsFalse(zoneWindow->IsDragEnabled());
            Assert::AreEqual(m_deviceId, zoneWindow->DeviceId());
            Assert::AreEqual(m_uniqueId.str().c_str(), zoneWindow->UniqueId().c_str());
            Assert::AreEqual(expectedWorkArea, zoneWindow->WorkAreaKey());
            Assert::IsFalse(zoneWindow->GetActiveZoneSetTmpPath().empty());
            Assert::IsFalse(zoneWindow->GetAppliedZoneSetTmpPath().empty());
            Assert::IsFalse(zoneWindow->GetCustomZoneSetsTmpPath().empty());
        }

    public:
        TEST_METHOD(CreateZoneWindow)
        {
            m_zoneWindow = MakeZoneWindow(m_hostPtr, m_hInst, m_monitor, m_deviceId.c_str(), m_virtualDesktopId.c_str(), false);
            testZoneWindow(m_zoneWindow);
            Assert::IsNull(m_zoneWindow->ActiveZoneSet());
        }

        TEST_METHOD(CreateZoneWindowNoZoneWindowHost)
        {
            m_zoneWindow = MakeZoneWindow(nullptr, m_hInst, m_monitor, m_deviceId.c_str(), m_virtualDesktopId.c_str(), false);

            Assert::IsNull(m_zoneWindow.get());
        }

        TEST_METHOD(CreateZoneWindowNoZoneWindowHostFlashZones)
        {
            m_zoneWindow = MakeZoneWindow(nullptr, m_hInst, m_monitor, m_deviceId.c_str(), m_virtualDesktopId.c_str(), true);

            Assert::IsNull(m_zoneWindow.get());
        }

        TEST_METHOD(CreateZoneWindowNoHinst)
        {
            m_zoneWindow = MakeZoneWindow(m_hostPtr, {}, m_monitor, m_deviceId.c_str(), m_virtualDesktopId.c_str(), false);

            testZoneWindow(m_zoneWindow);
            Assert::IsNull(m_zoneWindow->ActiveZoneSet());
        }

        TEST_METHOD(CreateZoneWindowNoHinstFlashZones)
        {
            m_zoneWindow = MakeZoneWindow(m_hostPtr, {}, m_monitor, m_deviceId.c_str(), m_virtualDesktopId.c_str(), true);

            testZoneWindow(m_zoneWindow);
            Assert::IsNull(m_zoneWindow->ActiveZoneSet());
        }

        TEST_METHOD(CreateZoneWindowNoMonitor)
        {
            m_zoneWindow = MakeZoneWindow(m_hostPtr, m_hInst, {}, m_deviceId.c_str(), m_virtualDesktopId.c_str(), false);

            Assert::IsNull(m_zoneWindow.get());
            Assert::IsNotNull(m_hostPtr);
        }

        TEST_METHOD(CreateZoneWindowNoMonitorFlashZones)
        {
            m_zoneWindow = MakeZoneWindow(m_hostPtr, m_hInst, {}, m_deviceId.c_str(), m_virtualDesktopId.c_str(), true);

            Assert::IsNull(m_zoneWindow.get());
            Assert::IsNotNull(m_hostPtr);
        }

        TEST_METHOD(CreateZoneWindowNoDeviceId)
        {
            m_zoneWindow = MakeZoneWindow(m_hostPtr, m_hInst, m_monitor, nullptr, m_virtualDesktopId.c_str(), false);

            const std::wstring expectedWorkArea = std::to_wstring(m_monitorInfo.rcMonitor.right) + L"_" + std::to_wstring(m_monitorInfo.rcMonitor.bottom);
            const std::wstring expectedUniqueId = L"FallbackDevice_" + std::to_wstring(m_monitorInfo.rcMonitor.right) + L"_" + std::to_wstring(m_monitorInfo.rcMonitor.bottom) + L"_" + m_virtualDesktopId;

            Assert::IsNotNull(m_zoneWindow.get());
            Assert::IsFalse(m_zoneWindow->IsDragEnabled());
            Assert::IsTrue(m_zoneWindow->DeviceId().empty());
            Assert::AreEqual(expectedUniqueId.c_str(), m_zoneWindow->UniqueId().c_str());
            Assert::AreEqual(expectedWorkArea, m_zoneWindow->WorkAreaKey());
            Assert::IsFalse(m_zoneWindow->GetActiveZoneSetTmpPath().empty());
            Assert::IsFalse(m_zoneWindow->GetAppliedZoneSetTmpPath().empty());
            Assert::IsFalse(m_zoneWindow->GetCustomZoneSetsTmpPath().empty());
            Assert::IsNull(m_zoneWindow->ActiveZoneSet());
        }

        TEST_METHOD(CreateZoneWindowNoDesktopId)
        {
            m_zoneWindow = MakeZoneWindow(m_hostPtr, m_hInst, m_monitor, m_deviceId.c_str(), nullptr, false);

            const std::wstring expectedWorkArea = std::to_wstring(m_monitorInfo.rcMonitor.right) + L"_" + std::to_wstring(m_monitorInfo.rcMonitor.bottom);
            Assert::IsNotNull(m_zoneWindow.get());
            Assert::IsFalse(m_zoneWindow->IsDragEnabled());
            Assert::AreEqual(m_deviceId.c_str(), m_zoneWindow->DeviceId().c_str());
            Assert::IsTrue(m_zoneWindow->UniqueId().empty());
            Assert::IsFalse(m_zoneWindow->GetActiveZoneSetTmpPath().empty());
            Assert::IsFalse(m_zoneWindow->GetAppliedZoneSetTmpPath().empty());
            Assert::IsFalse(m_zoneWindow->GetCustomZoneSetsTmpPath().empty());
            Assert::IsNull(m_zoneWindow->ActiveZoneSet());
            Assert::IsNull(m_zoneWindow->ActiveZoneSet());
        }

        TEST_METHOD(CreateZoneWindowWithActiveZoneTmpFile)
        {
            using namespace JSONHelpers;

            m_zoneWindow = MakeZoneWindow(m_hostPtr, m_hInst, m_monitor, m_deviceId.c_str(), m_virtualDesktopId.c_str(), false);

            const auto activeZoneSetTempPath = m_zoneWindow->GetActiveZoneSetTmpPath();

            for (int type = static_cast<int>(ZoneSetLayoutType::Focus); type < static_cast<int>(ZoneSetLayoutType::Custom); type++)
            {
                const auto expectedZoneSet = ZoneSetData{ guidString(), static_cast<ZoneSetLayoutType>(type), 5 };
                const auto data = DeviceInfoData{ expectedZoneSet, true, 16, 3 };
                const auto deviceInfo = DeviceInfoJSON{ L"default_device_id", data };
                const auto json = DeviceInfoJSON::ToJson(deviceInfo);
                json::to_file(activeZoneSetTempPath, json);

                //temp file read on initialization
                auto actual = MakeZoneWindow(m_hostPtr, m_hInst, m_monitor, m_deviceId.c_str(), m_virtualDesktopId.c_str(), false);

                testZoneWindow(actual);

                Assert::IsNotNull(actual->ActiveZoneSet());
                const auto actualZoneSet = actual->ActiveZoneSet()->GetZones();
                Assert::AreEqual((size_t)expectedZoneSet.zoneCount.value(), actualZoneSet.size());
            }
        }

        TEST_METHOD(CreateZoneWindowWithActiveCustomZoneTmpFile)
        {
            using namespace JSONHelpers;

            m_zoneWindow = MakeZoneWindow(m_hostPtr, m_hInst, m_monitor, m_deviceId.c_str(), m_virtualDesktopId.c_str(), false);

            const auto activeZoneSetTempPath = m_zoneWindow->GetActiveZoneSetTmpPath();

            const ZoneSetLayoutType type = ZoneSetLayoutType::Custom;
            const auto expectedZoneSet = ZoneSetData{ guidString(), type, 5 };
            const auto data = DeviceInfoData{ expectedZoneSet, true, 16, 3 };
            const auto deviceInfo = DeviceInfoJSON{ L"default_device_id", data };
            const auto json = DeviceInfoJSON::ToJson(deviceInfo);
            json::to_file(activeZoneSetTempPath, json);

            //temp file read on initialization
            auto actual = MakeZoneWindow(m_hostPtr, m_hInst, m_monitor, m_deviceId.c_str(), m_virtualDesktopId.c_str(), false);

            testZoneWindow(actual);

            //custom zone needs temp file for applied zone
            Assert::IsNotNull(actual->ActiveZoneSet());
            const auto actualZoneSet = actual->ActiveZoneSet()->GetZones();
            Assert::AreEqual((size_t)0, actualZoneSet.size());
        }

        TEST_METHOD(CreateZoneWindowWithActiveCustomZoneAppliedTmpFile)
        {
            using namespace JSONHelpers;

            m_zoneWindow = MakeZoneWindow(m_hostPtr, m_hInst, m_monitor, m_deviceId.c_str(), m_virtualDesktopId.c_str(), false);

            //save required data
            const auto activeZoneSetTempPath = m_zoneWindow->GetActiveZoneSetTmpPath();
            const auto appliedZoneSetTempPath = m_zoneWindow->GetAppliedZoneSetTmpPath();

            const ZoneSetLayoutType type = ZoneSetLayoutType::Custom;
            const auto expectedZoneSet = ZoneSetData{ guidString(), type, 5 };
            const auto data = DeviceInfoData{ expectedZoneSet, true, 16, 3 };
            const auto deviceInfo = DeviceInfoJSON{ L"default_device_id", data };
            const auto json = DeviceInfoJSON::ToJson(deviceInfo);
            json::to_file(activeZoneSetTempPath, json);

            const auto info = CanvasLayoutInfo{
                100, 100, std::vector{ CanvasLayoutInfo::Rect{ 0, 0, 100, 100 } }
            };
            const auto customZoneData = CustomZoneSetData{ L"name", CustomLayoutType::Canvas, info };
            auto customZoneJson = CustomZoneSetJSON::ToJson(CustomZoneSetJSON{ guidString(), customZoneData });
            json::to_file(appliedZoneSetTempPath, customZoneJson);

            //temp file read on initialization
            auto actual = MakeZoneWindow(m_hostPtr, m_hInst, m_monitor, m_deviceId.c_str(), m_virtualDesktopId.c_str(), false);

            testZoneWindow(actual);

            //custom zone needs temp file for applied zone
            Assert::IsNotNull(actual->ActiveZoneSet());
            const auto actualZoneSet = actual->ActiveZoneSet()->GetZones();
            Assert::AreEqual((size_t)1, actualZoneSet.size());
        }

        TEST_METHOD(CreateZoneWindowWithActiveCustomZoneAppliedTmpFileWithDeletedCustomZones)
        {
            using namespace JSONHelpers;

            m_zoneWindow = MakeZoneWindow(m_hostPtr, m_hInst, m_monitor, m_deviceId.c_str(), m_virtualDesktopId.c_str(), false);

            //save required data
            const auto activeZoneSetTempPath = m_zoneWindow->GetActiveZoneSetTmpPath();
            const auto appliedZoneSetTempPath = m_zoneWindow->GetAppliedZoneSetTmpPath();
            const auto deletedZonesTempPath = m_zoneWindow->GetCustomZoneSetsTmpPath();

            const ZoneSetLayoutType type = ZoneSetLayoutType::Custom;
            const auto expectedZoneSet = ZoneSetData{ guidString(), type, 5 };
            const auto data = DeviceInfoData{ expectedZoneSet, true, 16, 3 };
            const auto deviceInfo = DeviceInfoJSON{ L"default_device_id", data };
            const auto json = DeviceInfoJSON::ToJson(deviceInfo);
            json::to_file(activeZoneSetTempPath, json);

            const auto info = CanvasLayoutInfo{
                100, 100, std::vector{ CanvasLayoutInfo::Rect{ 0, 0, 100, 100 } }
            };
            const auto customZoneData = CustomZoneSetData{ L"name", CustomLayoutType::Canvas, info };
            const auto customZoneSet = CustomZoneSetJSON{ guidString(), customZoneData };
            auto customZoneJson = CustomZoneSetJSON::ToJson(customZoneSet);
            json::to_file(appliedZoneSetTempPath, customZoneJson);

            //save same zone as deleted
            json::JsonObject deletedCustomZoneSets = {};
            json::JsonArray zonesArray{};
            zonesArray.Append(json::JsonValue::CreateStringValue(customZoneSet.uuid.substr(1, customZoneSet.uuid.size() - 2).c_str()));
            deletedCustomZoneSets.SetNamedValue(L"deleted-custom-zone-sets", zonesArray);
            json::to_file(deletedZonesTempPath, deletedCustomZoneSets);

            //temp file read on initialization
            auto actual = MakeZoneWindow(m_hostPtr, m_hInst, m_monitor, m_deviceId.c_str(), m_virtualDesktopId.c_str(), false);

            testZoneWindow(actual);

            Assert::IsNotNull(actual->ActiveZoneSet());
            const auto actualZoneSet = actual->ActiveZoneSet()->GetZones();
            Assert::AreEqual((size_t)1, actualZoneSet.size());
        }

        TEST_METHOD(CreateZoneWindowWithActiveCustomZoneAppliedTmpFileWithUnusedDeletedCustomZones)
        {
            using namespace JSONHelpers;

            m_zoneWindow = MakeZoneWindow(m_hostPtr, m_hInst, m_monitor, m_deviceId.c_str(), m_virtualDesktopId.c_str(), false);

            //save required data
            const auto activeZoneSetTempPath = m_zoneWindow->GetActiveZoneSetTmpPath();
            const auto appliedZoneSetTempPath = m_zoneWindow->GetAppliedZoneSetTmpPath();
            const auto deletedZonesTempPath = m_zoneWindow->GetCustomZoneSetsTmpPath();

            const ZoneSetLayoutType type = ZoneSetLayoutType::Custom;
            const auto expectedZoneSet = ZoneSetData{ guidString(), type, 5 };
            const auto data = DeviceInfoData{ expectedZoneSet, true, 16, 3 };
            const auto deviceInfo = DeviceInfoJSON{ L"default_device_id", data };
            const auto json = DeviceInfoJSON::ToJson(deviceInfo);
            json::to_file(activeZoneSetTempPath, json);

            const auto info = CanvasLayoutInfo{
                100, 100, std::vector{ CanvasLayoutInfo::Rect{ 0, 0, 100, 100 } }
            };
            const auto customZoneData = CustomZoneSetData{ L"name", CustomLayoutType::Canvas, info };
            const auto customZoneSet = CustomZoneSetJSON{ guidString(), customZoneData };
            auto customZoneJson = CustomZoneSetJSON::ToJson(customZoneSet);
            json::to_file(appliedZoneSetTempPath, customZoneJson);

            //save different zone as deleted
            json::JsonObject deletedCustomZoneSets = {};
            json::JsonArray zonesArray{};
            const auto uuid = guidString();
            zonesArray.Append(json::JsonValue::CreateStringValue(uuid.substr(1, uuid.size() - 2).c_str()));
            deletedCustomZoneSets.SetNamedValue(L"deleted-custom-zone-sets", zonesArray);
            json::to_file(deletedZonesTempPath, deletedCustomZoneSets);

            //temp file read on initialization
            auto actual = MakeZoneWindow(m_hostPtr, m_hInst, m_monitor, m_deviceId.c_str(), m_virtualDesktopId.c_str(), false);

            testZoneWindow(actual);

            Assert::IsNotNull(actual->ActiveZoneSet());
            const auto actualZoneSet = actual->ActiveZoneSet()->GetZones();
            Assert::AreEqual((size_t)1, actualZoneSet.size());
        }

        TEST_METHOD(MoveSizeEnter)
        {
            m_zoneWindow = MakeZoneWindow(m_hostPtr, m_hInst, m_monitor, m_deviceId.c_str(), m_virtualDesktopId.c_str(), false);

            const auto expected = S_OK;
            const auto actual = m_zoneWindow->MoveSizeEnter(Mocks::Window(), true);

            Assert::AreEqual(expected, actual);
            Assert::IsTrue(m_zoneWindow->IsDragEnabled());
        }

        TEST_METHOD(MoveSizeEnterTwice)
        {
            m_zoneWindow = MakeZoneWindow(m_hostPtr, m_hInst, m_monitor, m_deviceId.c_str(), m_virtualDesktopId.c_str(), false);

            const auto expected = E_INVALIDARG;

            m_zoneWindow->MoveSizeEnter(Mocks::Window(), true);
            const auto actual = m_zoneWindow->MoveSizeEnter(Mocks::Window(), false);

            Assert::AreEqual(expected, actual);
            Assert::IsTrue(m_zoneWindow->IsDragEnabled());
        }

        TEST_METHOD(MoveSizeUpdate)
        {
            m_zoneWindow = MakeZoneWindow(m_hostPtr, m_hInst, m_monitor, m_deviceId.c_str(), m_virtualDesktopId.c_str(), false);

            const auto expected = S_OK;
            const auto actual = m_zoneWindow->MoveSizeUpdate(POINT{ 0, 0 }, true);

            Assert::AreEqual(expected, actual);
            Assert::IsTrue(m_zoneWindow->IsDragEnabled());
        }

        TEST_METHOD(MoveSizeUpdatePointNegativeCoordinates)
        {
            m_zoneWindow = MakeZoneWindow(m_hostPtr, m_hInst, m_monitor, m_deviceId.c_str(), m_virtualDesktopId.c_str(), false);

            const auto expected = S_OK;
            const auto actual = m_zoneWindow->MoveSizeUpdate(POINT{ -10, -10 }, true);

            Assert::AreEqual(expected, actual);
            Assert::IsTrue(m_zoneWindow->IsDragEnabled());
        }

        TEST_METHOD(MoveSizeUpdatePointBigCoordinates)
        {
            m_zoneWindow = MakeZoneWindow(m_hostPtr, m_hInst, m_monitor, m_deviceId.c_str(), m_virtualDesktopId.c_str(), false);

            const auto expected = S_OK;
            const auto actual = m_zoneWindow->MoveSizeUpdate(POINT{ m_monitorInfo.rcMonitor.right + 1, m_monitorInfo.rcMonitor.bottom + 1 }, true);

            Assert::AreEqual(expected, actual);
            Assert::IsTrue(m_zoneWindow->IsDragEnabled());
        }

        TEST_METHOD(MoveSizeEnd)
        {
            //prepare data
            m_zoneWindow = MakeZoneWindow(m_hostPtr, m_hInst, m_monitor, m_deviceId.c_str(), m_virtualDesktopId.c_str(), false);
            const auto activeZoneSetTempPath = m_zoneWindow->GetActiveZoneSetTmpPath();
            const auto type = JSONHelpers::ZoneSetLayoutType::Columns;
            const auto expectedZoneSet = JSONHelpers::ZoneSetData{ guidString(), type, 5 };
            const auto data = JSONHelpers::DeviceInfoData{ expectedZoneSet, true, 16, 3 };
            const auto deviceInfo = JSONHelpers::DeviceInfoJSON{ L"default_device_id", data };
            const auto json = JSONHelpers::DeviceInfoJSON::ToJson(deviceInfo);
            json::to_file(activeZoneSetTempPath, json);

            auto zoneWindow = MakeZoneWindow(m_hostPtr, m_hInst, m_monitor, m_deviceId.c_str(), m_virtualDesktopId.c_str(), false);

            const auto window = Mocks::Window();
            zoneWindow->MoveSizeEnter(window, true);

            const auto expected = S_OK;
            const auto actual = zoneWindow->MoveSizeEnd(window, POINT{ 0, 0 });
            Assert::AreEqual(expected, actual);

            const auto zoneSet = zoneWindow->ActiveZoneSet();
            zoneSet->MoveWindowIntoZoneByIndex(window, Mocks::Window(), false);
            const auto actualZoneIndex = zoneSet->GetZoneIndexFromWindow(window);
            Assert::AreNotEqual(-1, actualZoneIndex);
        }

        TEST_METHOD(MoveSizeEndWindowNotAdded)
        {
            //prepare data
            m_zoneWindow = MakeZoneWindow(m_hostPtr, m_hInst, m_monitor, m_deviceId.c_str(), m_virtualDesktopId.c_str(), false);
            const auto activeZoneSetTempPath = m_zoneWindow->GetActiveZoneSetTmpPath();
            const auto type = JSONHelpers::ZoneSetLayoutType::Columns;
            const auto expectedZoneSet = JSONHelpers::ZoneSetData{ guidString(), type, 5 };
            const auto data = JSONHelpers::DeviceInfoData{ expectedZoneSet, true, 16, 3 };
            const auto deviceInfo = JSONHelpers::DeviceInfoJSON{ L"default_device_id", data };
            const auto json = JSONHelpers::DeviceInfoJSON::ToJson(deviceInfo);
            json::to_file(activeZoneSetTempPath, json);

            auto zoneWindow = MakeZoneWindow(m_hostPtr, m_hInst, m_monitor, m_deviceId.c_str(), m_virtualDesktopId.c_str(), false);

            const auto window = Mocks::Window();
            zoneWindow->MoveSizeEnter(window, true);

            const auto expected = S_OK;
            const auto actual = zoneWindow->MoveSizeEnd(window, POINT{ 0, 0 });
            Assert::AreEqual(expected, actual);

            const auto zoneSet = zoneWindow->ActiveZoneSet();
            const auto actualZoneIndex = zoneSet->GetZoneIndexFromWindow(window);
            Assert::AreEqual(-1, actualZoneIndex);
        }

        TEST_METHOD(MoveSizeEndDifferentWindows)
        {
            m_zoneWindow = MakeZoneWindow(m_hostPtr, m_hInst, m_monitor, m_deviceId.c_str(), m_virtualDesktopId.c_str(), false);

            const auto window = Mocks::Window();
            m_zoneWindow->MoveSizeEnter(window, true);

            const auto expected = E_INVALIDARG;
            const auto actual = m_zoneWindow->MoveSizeEnd(Mocks::Window(), POINT{ 0, 0 });

            Assert::AreEqual(expected, actual);
        }

        TEST_METHOD(MoveSizeEndWindowNotSet)
        {
            m_zoneWindow = MakeZoneWindow(m_hostPtr, m_hInst, m_monitor, m_deviceId.c_str(), m_virtualDesktopId.c_str(), false);

            const auto expected = E_INVALIDARG;
            const auto actual = m_zoneWindow->MoveSizeEnd(Mocks::Window(), POINT{0, 0});

            Assert::AreEqual(expected, actual);
        }

        TEST_METHOD(MoveSizeEndInvalidPoint)
        {
            //prepare data
            m_zoneWindow = MakeZoneWindow(m_hostPtr, m_hInst, m_monitor, m_deviceId.c_str(), m_virtualDesktopId.c_str(), false);
            const auto activeZoneSetTempPath = m_zoneWindow->GetActiveZoneSetTmpPath();
            const auto type = JSONHelpers::ZoneSetLayoutType::Columns;
            const auto expectedZoneSet = JSONHelpers::ZoneSetData{ guidString(), type, 5 };
            const auto data = JSONHelpers::DeviceInfoData{ expectedZoneSet, true, 16, 3 };
            const auto deviceInfo = JSONHelpers::DeviceInfoJSON{ L"default_device_id", data };
            const auto json = JSONHelpers::DeviceInfoJSON::ToJson(deviceInfo);
            json::to_file(activeZoneSetTempPath, json);

            auto zoneWindow = MakeZoneWindow(m_hostPtr, m_hInst, m_monitor, m_deviceId.c_str(), m_virtualDesktopId.c_str(), false);

            const auto window = Mocks::Window();
            zoneWindow->MoveSizeEnter(window, true);

            const auto expected = S_OK;
            const auto actual = zoneWindow->MoveSizeEnd(window, POINT{ -1, -1 });
            Assert::AreEqual(expected, actual);

            const auto zoneSet = zoneWindow->ActiveZoneSet();
            zoneSet->MoveWindowIntoZoneByIndex(window, Mocks::Window(), false);
            const auto actualZoneIndex = zoneSet->GetZoneIndexFromWindow(window);
            Assert::AreNotEqual(-1, actualZoneIndex); //with invalid point zone remains the same
        }

        TEST_METHOD(MoveSizeCancel)
        {
            m_zoneWindow = MakeZoneWindow(m_hostPtr, m_hInst, m_monitor, m_deviceId.c_str(), m_virtualDesktopId.c_str(), false);

            const auto expected = S_OK;
            const auto actual = m_zoneWindow->MoveSizeCancel();

            Assert::AreEqual(expected, actual);
        }
    };
}
