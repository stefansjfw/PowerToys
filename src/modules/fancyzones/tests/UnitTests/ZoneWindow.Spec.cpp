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
        const std::wstring m_deviceId = L"DeviceId";
        const std::wstring m_virtualDesktopId = L"MyVirtualDesktopId";

        HINSTANCE m_hInst{};
        HMONITOR m_monitor{};
        MONITORINFO m_monitorInfo{};
        MockZoneWindowHost m_zoneWindowHost{};

        winrt::com_ptr<IZoneWindow> m_zoneWindow;

        std::wstring guidString()
        {
            GUID guid;
            Assert::AreEqual(S_OK, CoCreateGuid(&guid));

            OLECHAR* guidString;
            Assert::AreEqual(S_OK, StringFromCLSID(guid, &guidString));

            std::wstring guisStr{ guidString };
            CoTaskMemFree(guidString);

            return guidString;
        }

        TEST_METHOD_INITIALIZE(Init)
        {
            m_hInst = (HINSTANCE)GetModuleHandleW(nullptr);

            m_monitor = MonitorFromPoint(POINT{ 0, 0 }, MONITOR_DEFAULTTOPRIMARY);
            m_monitorInfo.cbSize = sizeof(m_monitorInfo);
            Assert::AreNotEqual(0, GetMonitorInfoW(m_monitor, &m_monitorInfo));

            Assert::AreEqual(S_OK, CoCreateGuid(&m_zoneWindowHost.m_guid));
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
            Assert::IsFalse(zoneWindow->UniqueId().empty());
            Assert::AreEqual(expectedWorkArea, zoneWindow->WorkAreaKey());
            Assert::IsFalse(zoneWindow->GetActiveZoneSetTmpPath().empty());
            Assert::IsFalse(zoneWindow->GetAppliedZoneSetTmpPath().empty());
            Assert::IsFalse(zoneWindow->GetCustomZoneSetsTmpPath().empty());
        }

    public:
        TEST_METHOD(CreateZoneWindow)
        {
            auto host = m_zoneWindowHost.get_strong();
            m_zoneWindow = MakeZoneWindow(host.get(), m_hInst, m_monitor, m_deviceId.c_str(), m_virtualDesktopId.c_str(), false);
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
            auto host = m_zoneWindowHost.get_strong();
            m_zoneWindow = MakeZoneWindow(host.get(), {}, m_monitor, m_deviceId.c_str(), m_virtualDesktopId.c_str(), false);

            testZoneWindow(m_zoneWindow);
            Assert::IsNull(m_zoneWindow->ActiveZoneSet());
        }

        TEST_METHOD(CreateZoneWindowNoHinstFlashZones)
        {
            auto host = m_zoneWindowHost.get_strong();
            m_zoneWindow = MakeZoneWindow(host.get(), {}, m_monitor, m_deviceId.c_str(), m_virtualDesktopId.c_str(), true);

            testZoneWindow(m_zoneWindow);
            Assert::IsNull(m_zoneWindow->ActiveZoneSet());
        }

        TEST_METHOD(CreateZoneWindowNoMonitor)
        {
            auto host = m_zoneWindowHost.get_strong();
            m_zoneWindow = MakeZoneWindow(host.get(), m_hInst, {}, m_deviceId.c_str(), m_virtualDesktopId.c_str(), false);

            Assert::IsNull(m_zoneWindow.get());
            Assert::IsNotNull(host.get());
        }

        TEST_METHOD(CreateZoneWindowNoMonitorFlashZones)
        {
            auto host = m_zoneWindowHost.get_strong();
            m_zoneWindow = MakeZoneWindow(host.get(), m_hInst, {}, m_deviceId.c_str(), m_virtualDesktopId.c_str(), true);

            Assert::IsNull(m_zoneWindow.get());
            Assert::IsNotNull(host.get());
        }

        TEST_METHOD(CreateZoneWindowNoDeviceId)
        {
            auto host = m_zoneWindowHost.get_strong();
            m_zoneWindow = MakeZoneWindow(host.get(), m_hInst, m_monitor, nullptr, m_virtualDesktopId.c_str(), false);

            testZoneWindow(m_zoneWindow);
            Assert::IsNull(m_zoneWindow->ActiveZoneSet());
        }

        TEST_METHOD(CreateZoneWindowNoDesktopId)
        {
            auto host = m_zoneWindowHost.get_strong();
            m_zoneWindow = MakeZoneWindow(host.get(), m_hInst, m_monitor, m_deviceId.c_str(), nullptr, false);

            testZoneWindow(m_zoneWindow);
            Assert::IsNull(m_zoneWindow->ActiveZoneSet());
        }

        TEST_METHOD(CreateZoneWindowWithActiveZoneTmpFile)
        {
            using namespace JSONHelpers;

            auto host = m_zoneWindowHost.get_strong();
            m_zoneWindow = MakeZoneWindow(host.get(), m_hInst, m_monitor, m_deviceId.c_str(), m_virtualDesktopId.c_str(), false);

            const auto activeZoneSetTempPath = m_zoneWindow->GetActiveZoneSetTmpPath();

            for (int type = static_cast<int>(ZoneSetLayoutType::Focus); type < static_cast<int>(ZoneSetLayoutType::Custom); type++)
            {
                const auto expectedZoneSet = ZoneSetData{ guidString(), static_cast<ZoneSetLayoutType>(type), 5 };
                const auto data = DeviceInfoData{ expectedZoneSet, true, 16, 3 };
                const auto deviceInfo = DeviceInfoJSON{ L"default_device_id", data };
                const auto json = DeviceInfoJSON::ToJson(deviceInfo);
                json::to_file(activeZoneSetTempPath, json);

                //temp file read on initialization
                auto actual = MakeZoneWindow(host.get(), m_hInst, m_monitor, m_deviceId.c_str(), m_virtualDesktopId.c_str(), false);

                testZoneWindow(actual);

                Assert::IsNotNull(actual->ActiveZoneSet());
                const auto actualZoneSet = actual->ActiveZoneSet()->GetZones();
                Assert::AreEqual((size_t)expectedZoneSet.zoneCount.value(), actualZoneSet.size());
            }
        }

        TEST_METHOD(CreateZoneWindowWithActiveCustomZoneTmpFile)
        {
            using namespace JSONHelpers;

            auto host = m_zoneWindowHost.get_strong();
            m_zoneWindow = MakeZoneWindow(host.get(), m_hInst, m_monitor, m_deviceId.c_str(), m_virtualDesktopId.c_str(), false);

            const auto activeZoneSetTempPath = m_zoneWindow->GetActiveZoneSetTmpPath();

            const ZoneSetLayoutType type = ZoneSetLayoutType::Custom;
            const auto expectedZoneSet = ZoneSetData{ guidString(), type, 5 };
            const auto data = DeviceInfoData{ expectedZoneSet, true, 16, 3 };
            const auto deviceInfo = DeviceInfoJSON{ L"default_device_id", data };
            const auto json = DeviceInfoJSON::ToJson(deviceInfo);
            json::to_file(activeZoneSetTempPath, json);

            //temp file read on initialization
            auto actual = MakeZoneWindow(host.get(), m_hInst, m_monitor, m_deviceId.c_str(), m_virtualDesktopId.c_str(), false);

            testZoneWindow(actual);

            //custom zone needs temp file for applied zone
            Assert::IsNotNull(actual->ActiveZoneSet());
            const auto actualZoneSet = actual->ActiveZoneSet()->GetZones();
            Assert::AreEqual((size_t)0, actualZoneSet.size());
        }

        TEST_METHOD(CreateZoneWindowWithActiveCustomZoneAppliedTmpFile)
        {
            using namespace JSONHelpers;

            auto host = m_zoneWindowHost.get_strong();
            m_zoneWindow = MakeZoneWindow(host.get(), m_hInst, m_monitor, m_deviceId.c_str(), m_virtualDesktopId.c_str(), false);

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
            auto actual = MakeZoneWindow(host.get(), m_hInst, m_monitor, m_deviceId.c_str(), m_virtualDesktopId.c_str(), false);

            testZoneWindow(actual);

            //custom zone needs temp file for applied zone
            Assert::IsNotNull(actual->ActiveZoneSet());
            const auto actualZoneSet = actual->ActiveZoneSet()->GetZones();
            Assert::AreEqual((size_t)1, actualZoneSet.size());
        }

        TEST_METHOD(CreateZoneWindowWithActiveCustomZoneAppliedTmpFileWithDeletedCustomZones)
        {
            using namespace JSONHelpers;

            auto host = m_zoneWindowHost.get_strong();
            m_zoneWindow = MakeZoneWindow(host.get(), m_hInst, m_monitor, m_deviceId.c_str(), m_virtualDesktopId.c_str(), false);

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
            auto actual = MakeZoneWindow(host.get(), m_hInst, m_monitor, m_deviceId.c_str(), m_virtualDesktopId.c_str(), false);

            testZoneWindow(actual);

            Assert::IsNotNull(actual->ActiveZoneSet());
            const auto actualZoneSet = actual->ActiveZoneSet()->GetZones();
            Assert::AreEqual((size_t)0, actualZoneSet.size());
        }

        TEST_METHOD(CreateZoneWindowWithActiveCustomZoneAppliedTmpFileWithUnusedDeletedCustomZones)
        {
            using namespace JSONHelpers;

            auto host = m_zoneWindowHost.get_strong();
            m_zoneWindow = MakeZoneWindow(host.get(), m_hInst, m_monitor, m_deviceId.c_str(), m_virtualDesktopId.c_str(), false);

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
            auto actual = MakeZoneWindow(host.get(), m_hInst, m_monitor, m_deviceId.c_str(), m_virtualDesktopId.c_str(), false);

            testZoneWindow(actual);

            Assert::IsNotNull(actual->ActiveZoneSet());
            const auto actualZoneSet = actual->ActiveZoneSet()->GetZones();
            Assert::AreEqual((size_t)1, actualZoneSet.size());
        }

        TEST_METHOD(TestDeviceId)
        {
            // Window initialization requires a valid HMONITOR - just use the primary for now.
            HMONITOR pimaryMonitor = MonitorFromWindow(HWND(), MONITOR_DEFAULTTOPRIMARY);
            MockZoneWindowHost host;
            std::wstring expectedDeviceId = L"SomeRandomValue";
            winrt::com_ptr<IZoneWindow> zoneWindow = MakeZoneWindow(dynamic_cast<IZoneWindowHost*>(&host), Mocks::Instance(), pimaryMonitor, expectedDeviceId.c_str(), L"MyVirtualDesktopId", false);

            Assert::AreEqual(expectedDeviceId, zoneWindow->DeviceId());
        }

        TEST_METHOD(TestUniqueId)
        {
            // Unique id of the format "ParsedMonitorDeviceId_MonitorWidth_MonitorHeight_VirtualDesktopId
            // Example: "DELA026#5&10a58c63&0&UID16777488_1024_768_MyVirtualDesktopId"
            std::wstring deviceId(L"\\\\?\\DISPLAY#DELA026#5&10a58c63&0&UID16777488#{e6f07b5f-ee97-4a90-b076-33f57bf4eaa7}");
            // Window initialization requires a valid HMONITOR - just use the primary for now.
            HMONITOR pimaryMonitor = MonitorFromWindow(HWND(), MONITOR_DEFAULTTOPRIMARY);
            MONITORINFO info;
            info.cbSize = sizeof(info);
            Assert::IsTrue(GetMonitorInfo(pimaryMonitor, &info));

            Rect monitorRect = Rect(info.rcMonitor);
            std::wstringstream ss;
            ss << L"DELA026#5&10a58c63&0&UID16777488_" << monitorRect.width() << "_" << monitorRect.height() << "_MyVirtualDesktopId";

            MockZoneWindowHost host;
            winrt::com_ptr<IZoneWindow> zoneWindow = MakeZoneWindow(dynamic_cast<IZoneWindowHost*>(&host), Mocks::Instance(), pimaryMonitor, deviceId.c_str(), L"MyVirtualDesktopId", false);
            Assert::AreEqual(zoneWindow->UniqueId().compare(ss.str()), 0);
        }
    };
}
