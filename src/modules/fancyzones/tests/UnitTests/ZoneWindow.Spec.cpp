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

        TEST_METHOD_INITIALIZE(Init)
        {
            m_hInst = (HINSTANCE)GetModuleHandleW(nullptr);

            m_monitor = MonitorFromPoint(POINT{ 0, 0 }, MONITOR_DEFAULTTOPRIMARY);
            m_monitorInfo.cbSize = sizeof(m_monitorInfo);
            Assert::AreNotEqual(0, GetMonitorInfoW(m_monitor, &m_monitorInfo));
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

    public:
        TEST_METHOD(CreateZoneWindow)
        {
            const std::wstring expectedWorkArea = std::to_wstring(m_monitorInfo.rcMonitor.right) + L"_" + std::to_wstring(m_monitorInfo.rcMonitor.bottom);
            auto host = m_zoneWindowHost.get_strong();
            m_zoneWindow = MakeZoneWindow(host.get(), m_hInst, m_monitor, m_deviceId.c_str(), m_virtualDesktopId.c_str(), false);

            Assert::IsNotNull(m_zoneWindow.get());
            Assert::IsNotNull(host.get());
            Assert::IsFalse(m_zoneWindow->IsDragEnabled());
            Assert::AreEqual(m_deviceId, m_zoneWindow->DeviceId());
            Assert::IsFalse(m_zoneWindow->UniqueId().empty());
            Assert::AreEqual(expectedWorkArea, m_zoneWindow->WorkAreaKey());
            Assert::IsNull(m_zoneWindow->ActiveZoneSet());
            Assert::IsFalse(m_zoneWindow->GetActiveZoneSetTmpPath().empty());
            Assert::IsFalse(m_zoneWindow->GetAppliedZoneSetTmpPath().empty());
            Assert::IsFalse(m_zoneWindow->GetCustomZoneSetsTmpPath().empty());
        }

        TEST_METHOD(CreateZoneWindowFlashZones)
        {
            const std::wstring expectedWorkArea = std::to_wstring(m_monitorInfo.rcMonitor.right) + L"_" + std::to_wstring(m_monitorInfo.rcMonitor.bottom);
            auto host = m_zoneWindowHost.get_strong();
            m_zoneWindow = MakeZoneWindow(host.get(), m_hInst, m_monitor, m_deviceId.c_str(), m_virtualDesktopId.c_str(), false);

            Assert::IsNotNull(m_zoneWindow.get());
            Assert::IsNotNull(host.get());
            Assert::IsFalse(m_zoneWindow->IsDragEnabled());
            Assert::AreEqual(m_deviceId, m_zoneWindow->DeviceId());
            Assert::IsFalse(m_zoneWindow->UniqueId().empty());
            Assert::AreEqual(expectedWorkArea, m_zoneWindow->WorkAreaKey());
            Assert::IsNull(m_zoneWindow->ActiveZoneSet());
            Assert::IsFalse(m_zoneWindow->GetActiveZoneSetTmpPath().empty());
            Assert::IsFalse(m_zoneWindow->GetAppliedZoneSetTmpPath().empty());
            Assert::IsFalse(m_zoneWindow->GetCustomZoneSetsTmpPath().empty());
        }

        TEST_METHOD(CreateZoneWindowNoZoneWindowHost)
        {
            const std::wstring expectedWorkArea = std::to_wstring(m_monitorInfo.rcMonitor.right) + L"_" + std::to_wstring(m_monitorInfo.rcMonitor.bottom);
            m_zoneWindow = MakeZoneWindow(nullptr, m_hInst, m_monitor, m_deviceId.c_str(), m_virtualDesktopId.c_str(), false);

            Assert::IsNull(m_zoneWindow.get());
        }

        TEST_METHOD(CreateZoneWindowNoZoneWindowHostFlashZones)
        {
            const std::wstring expectedWorkArea = std::to_wstring(m_monitorInfo.rcMonitor.right) + L"_" + std::to_wstring(m_monitorInfo.rcMonitor.bottom);
            m_zoneWindow = MakeZoneWindow(nullptr, m_hInst, m_monitor, m_deviceId.c_str(), m_virtualDesktopId.c_str(), true);

            Assert::IsNull(m_zoneWindow.get());
        }

        TEST_METHOD(CreateZoneWindowNoHinst)
        {
            const std::wstring expectedWorkArea = std::to_wstring(m_monitorInfo.rcMonitor.right) + L"_" + std::to_wstring(m_monitorInfo.rcMonitor.bottom);
            auto host = m_zoneWindowHost.get_strong();
            m_zoneWindow = MakeZoneWindow(host.get(), {}, m_monitor, m_deviceId.c_str(), m_virtualDesktopId.c_str(), false);

            Assert::IsNotNull(m_zoneWindow.get());
            Assert::IsNotNull(host.get());
            Assert::IsFalse(m_zoneWindow->IsDragEnabled());
            Assert::AreEqual(m_deviceId, m_zoneWindow->DeviceId());
            Assert::IsFalse(m_zoneWindow->UniqueId().empty());
            Assert::AreEqual(expectedWorkArea, m_zoneWindow->WorkAreaKey());
            Assert::IsNull(m_zoneWindow->ActiveZoneSet());
            Assert::IsFalse(m_zoneWindow->GetActiveZoneSetTmpPath().empty());
            Assert::IsFalse(m_zoneWindow->GetAppliedZoneSetTmpPath().empty());
            Assert::IsFalse(m_zoneWindow->GetCustomZoneSetsTmpPath().empty());
        }

        TEST_METHOD(CreateZoneWindowNoHinstFlashZones)
        {
            const std::wstring expectedWorkArea = std::to_wstring(m_monitorInfo.rcMonitor.right) + L"_" + std::to_wstring(m_monitorInfo.rcMonitor.bottom);
            auto host = m_zoneWindowHost.get_strong();
            m_zoneWindow = MakeZoneWindow(host.get(), {}, m_monitor, m_deviceId.c_str(), m_virtualDesktopId.c_str(), true);

            Assert::IsNotNull(m_zoneWindow.get());
            Assert::IsNotNull(host.get());
            Assert::IsFalse(m_zoneWindow->IsDragEnabled());
            Assert::AreEqual(m_deviceId, m_zoneWindow->DeviceId());
            Assert::IsFalse(m_zoneWindow->UniqueId().empty());
            Assert::AreEqual(expectedWorkArea, m_zoneWindow->WorkAreaKey());
            Assert::IsNull(m_zoneWindow->ActiveZoneSet());
            Assert::IsFalse(m_zoneWindow->GetActiveZoneSetTmpPath().empty());
            Assert::IsFalse(m_zoneWindow->GetAppliedZoneSetTmpPath().empty());
            Assert::IsFalse(m_zoneWindow->GetCustomZoneSetsTmpPath().empty());
        }

        TEST_METHOD(CreateZoneWindowNoMonitor)
        {
            const std::wstring expectedWorkArea = std::to_wstring(m_monitorInfo.rcMonitor.right) + L"_" + std::to_wstring(m_monitorInfo.rcMonitor.bottom);
            auto host = m_zoneWindowHost.get_strong();
            m_zoneWindow = MakeZoneWindow(host.get(), m_hInst, {}, m_deviceId.c_str(), m_virtualDesktopId.c_str(), false);

            Assert::IsNull(m_zoneWindow.get());
            Assert::IsNotNull(host.get());
        }

        TEST_METHOD(CreateZoneWindowNoMonitorFlashZones)
        {
            const std::wstring expectedWorkArea = std::to_wstring(m_monitorInfo.rcMonitor.right) + L"_" + std::to_wstring(m_monitorInfo.rcMonitor.bottom);
            auto host = m_zoneWindowHost.get_strong();
            m_zoneWindow = MakeZoneWindow(host.get(), m_hInst, {}, m_deviceId.c_str(), m_virtualDesktopId.c_str(), true);

            Assert::IsNull(m_zoneWindow.get());
            Assert::IsNotNull(host.get());
        }

        TEST_METHOD(CreateZoneWindowNoDeviceId)
        {
            const std::wstring expectedWorkArea = std::to_wstring(m_monitorInfo.rcMonitor.right) + L"_" + std::to_wstring(m_monitorInfo.rcMonitor.bottom);
            auto host = m_zoneWindowHost.get_strong();
            m_zoneWindow = MakeZoneWindow(host.get(), m_hInst, m_monitor, nullptr, m_virtualDesktopId.c_str(), false);

            Assert::IsNotNull(m_zoneWindow.get());
            Assert::IsNotNull(host.get());
            Assert::IsFalse(m_zoneWindow->IsDragEnabled());
            Assert::AreEqual(std::wstring(), m_zoneWindow->DeviceId());
            Assert::IsFalse(m_zoneWindow->UniqueId().empty());
            Assert::AreEqual(expectedWorkArea, m_zoneWindow->WorkAreaKey());
            Assert::IsNull(m_zoneWindow->ActiveZoneSet());
            Assert::IsFalse(m_zoneWindow->GetActiveZoneSetTmpPath().empty());
            Assert::IsFalse(m_zoneWindow->GetAppliedZoneSetTmpPath().empty());
            Assert::IsFalse(m_zoneWindow->GetCustomZoneSetsTmpPath().empty());
        }

        TEST_METHOD(CreateZoneWindowNoDeviceIdFlashZones)
        {
            const std::wstring expectedWorkArea = std::to_wstring(m_monitorInfo.rcMonitor.right) + L"_" + std::to_wstring(m_monitorInfo.rcMonitor.bottom);
            auto host = m_zoneWindowHost.get_strong();
            m_zoneWindow = MakeZoneWindow(host.get(), m_hInst, m_monitor, nullptr, m_virtualDesktopId.c_str(), true);

            Assert::IsNotNull(m_zoneWindow.get());
            Assert::IsNotNull(host.get());
            Assert::IsFalse(m_zoneWindow->IsDragEnabled());
            Assert::AreEqual(std::wstring(), m_zoneWindow->DeviceId());
            Assert::IsFalse(m_zoneWindow->UniqueId().empty());
            Assert::AreEqual(expectedWorkArea, m_zoneWindow->WorkAreaKey());
            Assert::IsNull(m_zoneWindow->ActiveZoneSet());
            Assert::IsFalse(m_zoneWindow->GetActiveZoneSetTmpPath().empty());
            Assert::IsFalse(m_zoneWindow->GetAppliedZoneSetTmpPath().empty());
            Assert::IsFalse(m_zoneWindow->GetCustomZoneSetsTmpPath().empty());
        }

        TEST_METHOD(CreateZoneWindowNoDesktopId)
        {
            const std::wstring expectedWorkArea = std::to_wstring(m_monitorInfo.rcMonitor.right) + L"_" + std::to_wstring(m_monitorInfo.rcMonitor.bottom);
            auto host = m_zoneWindowHost.get_strong();
            m_zoneWindow = MakeZoneWindow(host.get(), m_hInst, m_monitor, m_deviceId.c_str(), nullptr, false);

            Assert::IsNotNull(m_zoneWindow.get());
            Assert::IsNotNull(host.get());
            Assert::IsFalse(m_zoneWindow->IsDragEnabled());
            Assert::AreEqual(m_deviceId, m_zoneWindow->DeviceId());
            Assert::IsFalse(m_zoneWindow->UniqueId().empty());
            Assert::AreEqual(expectedWorkArea, m_zoneWindow->WorkAreaKey());
            Assert::IsNull(m_zoneWindow->ActiveZoneSet());
            Assert::IsFalse(m_zoneWindow->GetActiveZoneSetTmpPath().empty());
            Assert::IsFalse(m_zoneWindow->GetAppliedZoneSetTmpPath().empty());
            Assert::IsFalse(m_zoneWindow->GetCustomZoneSetsTmpPath().empty());
        }

        TEST_METHOD(CreateZoneWindowNoDesktopIdFlashZones)
        {
            const std::wstring expectedWorkArea = std::to_wstring(m_monitorInfo.rcMonitor.right) + L"_" + std::to_wstring(m_monitorInfo.rcMonitor.bottom);
            auto host = m_zoneWindowHost.get_strong();
            m_zoneWindow = MakeZoneWindow(host.get(), m_hInst, m_monitor, m_deviceId.c_str(), nullptr, true);

            Assert::IsNotNull(m_zoneWindow.get());
            Assert::IsNotNull(host.get());
            Assert::IsFalse(m_zoneWindow->IsDragEnabled());
            Assert::AreEqual(m_deviceId, m_zoneWindow->DeviceId());
            Assert::IsFalse(m_zoneWindow->UniqueId().empty());
            Assert::AreEqual(expectedWorkArea, m_zoneWindow->WorkAreaKey());
            Assert::IsNull(m_zoneWindow->ActiveZoneSet());
            Assert::IsFalse(m_zoneWindow->GetActiveZoneSetTmpPath().empty());
            Assert::IsFalse(m_zoneWindow->GetAppliedZoneSetTmpPath().empty());
            Assert::IsFalse(m_zoneWindow->GetCustomZoneSetsTmpPath().empty());
        }

        TEST_METHOD(CreateZoneWindowWithActiveZoneTmpFile)
        {
            const std::wstring expectedWorkArea = std::to_wstring(m_monitorInfo.rcMonitor.right) + L"_" + std::to_wstring(m_monitorInfo.rcMonitor.bottom);
            auto host = m_zoneWindowHost.get_strong();
            m_zoneWindow = MakeZoneWindow(host.get(), m_hInst, m_monitor, m_deviceId.c_str(), m_virtualDesktopId.c_str(), false);

            Assert::IsNotNull(m_zoneWindow.get());
            Assert::IsNotNull(host.get());
            Assert::IsFalse(m_zoneWindow->IsDragEnabled());
            Assert::AreEqual(m_deviceId, m_zoneWindow->DeviceId());
            Assert::IsFalse(m_zoneWindow->UniqueId().empty());
            Assert::AreEqual(expectedWorkArea, m_zoneWindow->WorkAreaKey());
            Assert::IsNull(m_zoneWindow->ActiveZoneSet());
            Assert::IsFalse(m_zoneWindow->GetActiveZoneSetTmpPath().empty());
            Assert::IsFalse(m_zoneWindow->GetAppliedZoneSetTmpPath().empty());
            Assert::IsFalse(m_zoneWindow->GetCustomZoneSetsTmpPath().empty());
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
