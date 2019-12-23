#include "pch.h"
#include "lib\Zone.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace FancyZonesUnitTests
{
    TEST_CLASS(ZoneUnitTests)
    {
    private:
        RECT m_zoneRect{ 10, 10, 200, 200 };
        HINSTANCE m_hInst{};

        TEST_METHOD_INITIALIZE(Init)
        {
            m_hInst = (HINSTANCE)GetModuleHandleW(nullptr);
        }

    public:
        TEST_METHOD(TestCreateZone)
        {
            winrt::com_ptr<IZone> zone = MakeZone(m_zoneRect);
            Assert::IsNotNull(&zone);
            CustomAssert::AreEqual(m_zoneRect, zone->GetZoneRect());
        }

        TEST_METHOD(TestCreateZoneZeroRect)
        {
            RECT zoneRect{ 0, 0, 0, 0 };
            winrt::com_ptr<IZone> zone = MakeZone(zoneRect);
            Assert::IsNotNull(&zone);
            CustomAssert::AreEqual(zoneRect, zone->GetZoneRect());
        }

        TEST_METHOD(GetSetId)
        {
            winrt::com_ptr<IZone> zone = MakeZone(m_zoneRect);

            constexpr size_t id = 10;
            zone->SetId(id);
            Assert::AreEqual(zone->Id(), id);
        }

        TEST_METHOD(IsEmpty)
        {
            winrt::com_ptr<IZone> zone = MakeZone(m_zoneRect);
            Assert::IsTrue(zone->IsEmpty());
        }

        TEST_METHOD(IsNonEmptyStampTrue)
        {
            winrt::com_ptr<IZone> zone = MakeZone(m_zoneRect);

            HWND window = Mocks::HwndCreator()(m_hInst);
            HWND zoneWindow = Mocks::HwndCreator()(m_hInst);
            zone->AddWindowToZone(window, zoneWindow, true);

            Assert::IsFalse(zone->IsEmpty());
        }

        TEST_METHOD(IsNonEmptyStampFalse)
        {
            winrt::com_ptr<IZone> zone = MakeZone(m_zoneRect);

            HWND window = Mocks::HwndCreator()(m_hInst);
            HWND zoneWindow = Mocks::HwndCreator()(m_hInst);
            zone->AddWindowToZone(window, zoneWindow, false);

            Assert::IsFalse(zone->IsEmpty());
        }

        TEST_METHOD(IsNonEmptyManyWindows)
        {
            winrt::com_ptr<IZone> zone = MakeZone(m_zoneRect);

            HWND zoneWindow = Mocks::HwndCreator()(m_hInst);
            for (int i = 0; i < 10; i++)
            {
                HWND window = Mocks::HwndCreator()(m_hInst);
                zone->AddWindowToZone(window, zoneWindow, i % 2 == 0);
            }

            Assert::IsFalse(zone->IsEmpty());
        }

        TEST_METHOD(IsNonEmptyManyZoneWindows)
        {
            winrt::com_ptr<IZone> zone = MakeZone(m_zoneRect);

            HWND window = Mocks::HwndCreator()(m_hInst);
            for (int i = 0; i < 10; i++)
            {
                HWND zoneWindow = Mocks::HwndCreator()(m_hInst);
                zone->AddWindowToZone(window, zoneWindow, i % 2 == 0);
            }

            Assert::IsFalse(zone->IsEmpty());
        }

        TEST_METHOD(IsNonEmptyMany)
        {
            winrt::com_ptr<IZone> zone = MakeZone(m_zoneRect);

            for (int i = 0; i < 10; i++)
            {
                HWND zoneWindow = Mocks::HwndCreator()(m_hInst);
                HWND window = Mocks::HwndCreator()(m_hInst);
                zone->AddWindowToZone(window, zoneWindow, i % 2 == 0);
            }

            Assert::IsFalse(zone->IsEmpty());
        }

        TEST_METHOD(ContainsWindowEmpty)
        {
            winrt::com_ptr<IZone> zone = MakeZone(m_zoneRect);
            HWND newWindow = Mocks::HwndCreator()(m_hInst);
            Assert::IsFalse(zone->ContainsWindow(newWindow));
        }
        TEST_METHOD(ContainsWindowNot)
        {
            winrt::com_ptr<IZone> zone = MakeZone(m_zoneRect);

            for (int i = 0; i < 10; i++)
            {
                HWND zoneWindow = Mocks::HwndCreator()(m_hInst);
                HWND window = Mocks::HwndCreator()(m_hInst);
                zone->AddWindowToZone(window, zoneWindow, i % 2 == 0);
            }

            HWND newWindow = Mocks::HwndCreator()(m_hInst);
            Assert::IsFalse(zone->ContainsWindow(newWindow));
        }

        TEST_METHOD(ContainsWindowStampTrue)
        {
            winrt::com_ptr<IZone> zone = MakeZone(m_zoneRect);

            HWND window = Mocks::HwndCreator()(m_hInst);
            HWND zoneWindow = Mocks::HwndCreator()(m_hInst);
            zone->AddWindowToZone(window, zoneWindow, true);

            Assert::IsTrue(zone->ContainsWindow(window));
        }

        TEST_METHOD(ContainsWindowStampFalse)
        {
            winrt::com_ptr<IZone> zone = MakeZone(m_zoneRect);

            HWND window = Mocks::HwndCreator()(m_hInst);
            HWND zoneWindow = Mocks::HwndCreator()(m_hInst);
            zone->AddWindowToZone(window, zoneWindow, false);

            Assert::IsTrue(zone->ContainsWindow(window));
        }

        TEST_METHOD(ContainsWindowManyWindows)
        {
            winrt::com_ptr<IZone> zone = MakeZone(m_zoneRect);

            HWND zoneWindow = Mocks::HwndCreator()(m_hInst);
            std::vector<HWND> windowVec{};
            for (int i = 0; i < 10; i++)
            {
                HWND window = Mocks::HwndCreator()(m_hInst);
                windowVec.push_back(window);
                zone->AddWindowToZone(window, zoneWindow, i % 2 == 0);
            }

            for (auto wnd : windowVec)
            {
                Assert::IsTrue(zone->ContainsWindow(wnd));
            }
        }

        TEST_METHOD(ContainsWindowManyZoneWindows)
        {
            winrt::com_ptr<IZone> zone = MakeZone(m_zoneRect);

            HWND window = Mocks::HwndCreator()(m_hInst);
            std::vector<HWND> windowVec{};
            for (int i = 0; i < 10; i++)
            {
                HWND zoneWindow = Mocks::HwndCreator()(m_hInst);
                windowVec.push_back(window);
                zone->AddWindowToZone(window, zoneWindow, i % 2 == 0);
            }

            for (auto wnd : windowVec)
            {
                Assert::IsTrue(zone->ContainsWindow(wnd));
            }
        }

        TEST_METHOD(ContainsWindowMany)
        {
            winrt::com_ptr<IZone> zone = MakeZone(m_zoneRect);

            std::vector<HWND> windowVec{};
            for (int i = 0; i < 10; i++)
            {
                HWND zoneWindow = Mocks::HwndCreator()(m_hInst);
                HWND window = Mocks::HwndCreator()(m_hInst);
                windowVec.push_back(window);
                zone->AddWindowToZone(window, zoneWindow, i % 2 == 0);
            }

            for (auto wnd : windowVec)
            {
                Assert::IsTrue(zone->ContainsWindow(wnd));
            }
        }

        TEST_METHOD(AddWindowNullptr)
        {
            winrt::com_ptr<IZone> zone = MakeZone(m_zoneRect);

            HWND window = nullptr;
            HWND zoneWindow = Mocks::HwndCreator()(m_hInst);
            zone->AddWindowToZone(window, zoneWindow, true);

            Assert::IsFalse(zone->IsEmpty());
            Assert::IsTrue(zone->ContainsWindow(window));
        }

        TEST_METHOD(AddWindowZoneNullptr)
        {
            winrt::com_ptr<IZone> zone = MakeZone(m_zoneRect);

            HWND window = Mocks::HwndCreator()(m_hInst);
            HWND zoneWindow = nullptr;
            zone->AddWindowToZone(window, zoneWindow, true);

            Assert::IsFalse(zone->IsEmpty());
            Assert::IsTrue(zone->ContainsWindow(window));
        }

        TEST_METHOD(AddManySame)
        {
            winrt::com_ptr<IZone> zone = MakeZone(m_zoneRect);

            HWND zoneWindow = Mocks::HwndCreator()(m_hInst);
            HWND window = Mocks::HwndCreator()(m_hInst);
            std::vector<HWND> windowVec{};
            for (int i = 0; i < 10; i++)
            {
                zone->AddWindowToZone(window, zoneWindow, i % 2 == 0);
            }

            Assert::IsFalse(zone->IsEmpty());
            for (auto wnd : windowVec)
            {
                Assert::IsTrue(zone->ContainsWindow(wnd));
            }
        }

        TEST_METHOD(AddManySameNullptr)
        {
            winrt::com_ptr<IZone> zone = MakeZone(m_zoneRect);

            HWND zoneWindow = nullptr;
            HWND window = nullptr;
            std::vector<HWND> windowVec{};
            for (int i = 0; i < 10; i++)
            {
                zone->AddWindowToZone(window, zoneWindow, i % 2 == 0);
            }

            Assert::IsFalse(zone->IsEmpty());
            for (auto wnd : windowVec)
            {
                Assert::IsTrue(zone->ContainsWindow(wnd));
            }
        }

        TEST_METHOD(RemoveWindowRestoreSizeTrue)
        {
            winrt::com_ptr<IZone> zone = MakeZone(m_zoneRect);
            HWND newWindow = Mocks::HwndCreator()(m_hInst);

            zone->AddWindowToZone(newWindow, Mocks::HwndCreator()(m_hInst), true);
            Assert::IsFalse(zone->IsEmpty());
            Assert::IsTrue(zone->ContainsWindow(newWindow));

            zone->RemoveWindowFromZone(newWindow, true);
            Assert::IsTrue(zone->IsEmpty());
            Assert::IsFalse(zone->ContainsWindow(newWindow));
        }

        TEST_METHOD(RemoveWindowRestoreSizeFalse)
        {
            winrt::com_ptr<IZone> zone = MakeZone(m_zoneRect);
            HWND newWindow = Mocks::HwndCreator()(m_hInst);

            zone->AddWindowToZone(newWindow, Mocks::HwndCreator()(m_hInst), true);
            Assert::IsFalse(zone->IsEmpty());
            Assert::IsTrue(zone->ContainsWindow(newWindow));

            zone->RemoveWindowFromZone(newWindow, false);
            Assert::IsTrue(zone->IsEmpty());
            Assert::IsFalse(zone->ContainsWindow(newWindow));
        }

        TEST_METHOD(RemoveInvalidWindowRestoreSizeTrue)
        {
            winrt::com_ptr<IZone> zone = MakeZone(m_zoneRect);
            HWND newWindow = Mocks::HwndCreator()(m_hInst);
            zone->RemoveWindowFromZone(newWindow, true);

            Assert::IsTrue(zone->IsEmpty());
            Assert::IsFalse(zone->ContainsWindow(newWindow));
        }
        TEST_METHOD(RemoveInvalidWindowRestoreSizeFalse)
        {
            winrt::com_ptr<IZone> zone = MakeZone(m_zoneRect);
            HWND newWindow = Mocks::HwndCreator()(m_hInst);
            zone->RemoveWindowFromZone(newWindow, false);

            Assert::IsTrue(zone->IsEmpty());
            Assert::IsFalse(zone->ContainsWindow(newWindow));
        }

        TEST_METHOD(RemoveNullptrWindowRestoreSizeTrue)
        {
            winrt::com_ptr<IZone> zone = MakeZone(m_zoneRect);
            HWND newWindow = nullptr;

            zone->AddWindowToZone(newWindow, Mocks::HwndCreator()(m_hInst), true);
            Assert::IsFalse(zone->IsEmpty());
            Assert::IsTrue(zone->ContainsWindow(newWindow));

            zone->RemoveWindowFromZone(newWindow, true);
            Assert::IsTrue(zone->IsEmpty());
            Assert::IsFalse(zone->ContainsWindow(newWindow));
        }

        TEST_METHOD(RemoveNullptrWindowRestoreSizeFalse)
        {
            winrt::com_ptr<IZone> zone = MakeZone(m_zoneRect);
            HWND newWindow = nullptr;

            zone->AddWindowToZone(newWindow, Mocks::HwndCreator()(m_hInst), true);
            Assert::IsFalse(zone->IsEmpty());
            Assert::IsTrue(zone->ContainsWindow(newWindow));

            zone->RemoveWindowFromZone(newWindow, false);
            Assert::IsTrue(zone->IsEmpty());
            Assert::IsFalse(zone->ContainsWindow(newWindow));
        }

        TEST_METHOD(RemoveMany)
        {
            winrt::com_ptr<IZone> zone = MakeZone(m_zoneRect);

            std::vector<HWND> windowVec{};
            for (int i = 0; i < 10; i++)
            {
                HWND zoneWindow = Mocks::HwndCreator()(m_hInst);
                HWND window = Mocks::HwndCreator()(m_hInst);
                windowVec.push_back(window);
                zone->AddWindowToZone(window, zoneWindow, i % 2 == 0);
            }

            for (auto wnd : windowVec)
            {
                zone->RemoveWindowFromZone(wnd, true);
            }

            Assert::IsTrue(zone->IsEmpty());
        }

        TEST_METHOD(RemoveManySame)
        {
            winrt::com_ptr<IZone> zone = MakeZone(m_zoneRect);

            HWND zoneWindow = Mocks::HwndCreator()(m_hInst);
            HWND window = Mocks::HwndCreator()(m_hInst);
            for (int i = 0; i < 10; i++)
            {
                zone->AddWindowToZone(window, zoneWindow, i % 2 == 0);
            }

            zone->RemoveWindowFromZone(window, true);

            Assert::IsTrue(zone->IsEmpty());
            Assert::IsFalse(zone->ContainsWindow(window));
        }

        TEST_METHOD(RemoveDouble)
        {
            winrt::com_ptr<IZone> zone = MakeZone(m_zoneRect);

            HWND zoneWindow = Mocks::HwndCreator()(m_hInst);
            HWND window = Mocks::HwndCreator()(m_hInst);
            for (int i = 0; i < 10; i++)
            {
                zone->AddWindowToZone(window, zoneWindow, i % 2 == 0);
            }

            zone->RemoveWindowFromZone(window, true);
            zone->RemoveWindowFromZone(window, true);

            Assert::IsTrue(zone->IsEmpty());
        }
    };
}
