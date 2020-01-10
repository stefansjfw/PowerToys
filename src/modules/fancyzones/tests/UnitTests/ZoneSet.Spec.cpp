#include "pch.h"
#include "lib\ZoneSet.h"

#include "Util.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace FancyZonesUnitTests
{
    TEST_CLASS(ZoneSetUnitTests)
    {
        GUID m_id;
        const WORD m_layoutId = 0xFFFF;
        const PCWSTR m_resolutionKey = L"WorkAreaIn";

        ZoneSetConfig* m_config = nullptr;

        HINSTANCE m_hInst{};

        TEST_METHOD_INITIALIZE(Init)
        {
            auto hres = CoCreateGuid(&m_id);
            Assert::AreEqual(S_OK, hres);
            m_config = new ZoneSetConfig(m_id, m_layoutId, Mocks::Monitor(), m_resolutionKey);

            m_hInst = (HINSTANCE)GetModuleHandleW(nullptr);
        }

        TEST_METHOD_CLEANUP(Clear)
        {
            delete m_config;
        }

        void compareZones(const winrt::com_ptr<IZone>& expected, const winrt::com_ptr<IZone>& actual)
        {
            Assert::AreEqual(expected->Id(), actual->Id());
            Assert::AreEqual(expected->GetZoneRect().left, actual->GetZoneRect().left);
            Assert::AreEqual(expected->GetZoneRect().right, actual->GetZoneRect().right);
            Assert::AreEqual(expected->GetZoneRect().top, actual->GetZoneRect().top);
            Assert::AreEqual(expected->GetZoneRect().bottom, actual->GetZoneRect().bottom);
        }

    public:
        TEST_METHOD(TestCreateZoneSet)
        {
            winrt::com_ptr<IZoneSet> set = MakeZoneSet(*m_config);
            Assert::IsNotNull(&set);
            CustomAssert::AreEqual(set->Id(), m_id);
            CustomAssert::AreEqual(set->LayoutId(), m_layoutId);
        }

        TEST_METHOD(TestCreateZoneSetGuidEmpty)
        {
            GUID zoneSetId{};
            ZoneSetConfig config(zoneSetId, m_layoutId, Mocks::Monitor(), m_resolutionKey);
            winrt::com_ptr<IZoneSet> set = MakeZoneSet(config);

            Assert::IsNotNull(&set);
            CustomAssert::AreEqual(set->Id(), zoneSetId);
            CustomAssert::AreEqual(set->LayoutId(), m_layoutId);
        }

        TEST_METHOD(TestCreateZoneSetMonitorEmpty)
        {
            ZoneSetConfig config(m_id, m_layoutId, nullptr, m_resolutionKey);
            winrt::com_ptr<IZoneSet> set = MakeZoneSet(config);
            Assert::IsNotNull(&set);
            CustomAssert::AreEqual(set->Id(), m_id);
            CustomAssert::AreEqual(set->LayoutId(), m_layoutId);
        }

        TEST_METHOD(TestCreateZoneSetKeyEmpty)
        {
            ZoneSetConfig config(m_id, m_layoutId, Mocks::Monitor(), nullptr);
            winrt::com_ptr<IZoneSet> set = MakeZoneSet(config);
            Assert::IsNotNull(&set);
            CustomAssert::AreEqual(set->Id(), m_id);
            CustomAssert::AreEqual(set->LayoutId(), m_layoutId);
        }

        TEST_METHOD(EmptyZones)
        {
            winrt::com_ptr<IZoneSet> set = MakeZoneSet(*m_config);
            auto zones = set->GetZones();
            Assert::AreEqual((size_t)0, zones.size());
        }

        TEST_METHOD(AddOne)
        {
            winrt::com_ptr<IZoneSet> set = MakeZoneSet(*m_config);
            winrt::com_ptr<IZone> zone = MakeZone({ 0, 0, 100, 100 });
            set->AddZone(zone);
            auto zones = set->GetZones();
            Assert::AreEqual((size_t)1, zones.size());
            compareZones(zone, zones[0]);
            Assert::AreEqual((size_t)1, zones[0]->Id());
        }

        TEST_METHOD(AddManySame)
        {
            winrt::com_ptr<IZoneSet> set = MakeZoneSet(*m_config);

            winrt::com_ptr<IZone> zone = MakeZone({ 0, 0, 100, 100 });
            for (size_t i = 0; i < 1024; i++)
            {
                set->AddZone(zone);
                auto zones = set->GetZones();
                Assert::AreEqual(i + 1, zones.size());
                compareZones(zone, zones[i]);
                Assert::AreEqual(i + 1, zones[i]->Id());
            }
        }

        TEST_METHOD(AddManyEqual)
        {
            winrt::com_ptr<IZoneSet> set = MakeZoneSet(*m_config);

            for (size_t i = 0; i < 1024; i++)
            {
                winrt::com_ptr<IZone> zone = MakeZone({ 0, 0, 100, 100 });
                set->AddZone(zone);
                auto zones = set->GetZones();
                Assert::AreEqual(i + 1, zones.size());
                compareZones(zone, zones[i]);
                Assert::AreEqual(i + 1, zones[i]->Id());
            }
        }

        TEST_METHOD(AddManyDifferent)
        {
            winrt::com_ptr<IZoneSet> set = MakeZoneSet(*m_config);

            for (size_t i = 0; i < 1024; i++)
            {
                winrt::com_ptr<IZone> zone = MakeZone({ rand() % 10, rand() % 10, rand() % 100, rand() % 100 });
                set->AddZone(zone);
                auto zones = set->GetZones();
                Assert::AreEqual(i + 1, zones.size());
                compareZones(zone, zones[i]);
                Assert::AreEqual(i + 1, zones[i]->Id());
            }
        }

        TEST_METHOD(ZoneFromPointEmpty)
        {
            winrt::com_ptr<IZoneSet> set = MakeZoneSet(*m_config);
            auto actual = set->ZoneFromPoint(POINT{ 0, 0 });
            Assert::IsTrue(nullptr == actual);
        }

        TEST_METHOD(ZoneFromPointInner)
        {
            winrt::com_ptr<IZoneSet> set = MakeZoneSet(*m_config);
            const int left = 0, top = 0, right = 100, bottom = 100;
            winrt::com_ptr<IZone> expected = MakeZone({ left, top, right, bottom });
            set->AddZone(expected);

            for (int i = left + 1; i < right; i++)
            {
                for (int j = top + 1; j < bottom; j++)
                {
                    auto actual = set->ZoneFromPoint(POINT{ i, j });
                    Assert::IsTrue(actual != nullptr);
                    compareZones(expected, actual);
                }
            }
        }

        TEST_METHOD(ZoneFromPointBorder)
        {
            winrt::com_ptr<IZoneSet> set = MakeZoneSet(*m_config);
            const int left = 0, top = 0, right = 100, bottom = 100;
            winrt::com_ptr<IZone> expected = MakeZone({ left, top, right, bottom });
            set->AddZone(expected);

            for (int i = left; i < right; i++)
            {
                auto actual = set->ZoneFromPoint(POINT{ i, top });
                Assert::IsTrue(actual != nullptr);
                compareZones(expected, actual);
            }

            for (int i = top; i < bottom; i++)
            {
                auto actual = set->ZoneFromPoint(POINT{ left, i });
                Assert::IsTrue(actual != nullptr);
                compareZones(expected, actual);
            }

            //bottom and right borders considered to be outside
            for (int i = left; i < right; i++)
            {
                auto actual = set->ZoneFromPoint(POINT{ i, bottom });
                Assert::IsTrue(nullptr == actual);
            }

            for (int i = top; i < bottom; i++)
            {
                auto actual = set->ZoneFromPoint(POINT{ right, i });
                Assert::IsTrue(nullptr == actual);
            }
        }

        TEST_METHOD(ZoneFromPointOuter)
        {
            winrt::com_ptr<IZoneSet> set = MakeZoneSet(*m_config);
            const int left = 0, top = 0, right = 100, bottom = 100;
            winrt::com_ptr<IZone> zone = MakeZone({ left, top, right, bottom });
            set->AddZone(zone);

            auto actual = set->ZoneFromPoint(POINT{ 101, 101 });
            Assert::IsTrue(actual == nullptr);
        }

        TEST_METHOD(ZoneFromPointOverlapping)
        {
            winrt::com_ptr<IZoneSet> set = MakeZoneSet(*m_config);

            winrt::com_ptr<IZone> zone1 = MakeZone({ 0, 0, 100, 100 });
            set->AddZone(zone1);
            winrt::com_ptr<IZone> zone2 = MakeZone({ 10, 10, 90, 90 });
            set->AddZone(zone2);
            winrt::com_ptr<IZone> zone3 = MakeZone({ 10, 10, 150, 150 });
            set->AddZone(zone3);
            winrt::com_ptr<IZone> zone4 = MakeZone({ 10, 10, 50, 50 });
            set->AddZone(zone4);

            auto actual = set->ZoneFromPoint(POINT{ 50, 50 });
            Assert::IsTrue(actual != nullptr);
            compareZones(zone2, actual);
        }

        TEST_METHOD(ZoneFromPointWithNotNormalizedRect)
        {
            winrt::com_ptr<IZoneSet> set = MakeZoneSet(*m_config);
            winrt::com_ptr<IZone> zone = MakeZone({ 100, 100, 0, 0 });
            set->AddZone(zone);

            auto actual = set->ZoneFromPoint(POINT{ 50, 50 });
            Assert::IsTrue(actual == nullptr);
        }

        TEST_METHOD(ZoneIndexFromWindow)
        {
            winrt::com_ptr<IZoneSet> set = MakeZoneSet(*m_config);

            HWND window = Mocks::WindowCreate(m_hInst);
            HWND zoneWindow = Mocks::WindowCreate(m_hInst);

            winrt::com_ptr<IZone> zone1 = MakeZone({ 0, 0, 100, 100 });
            winrt::com_ptr<IZone> zone2 = MakeZone({ 20, 20, 200, 200 });
            winrt::com_ptr<IZone> zone3 = MakeZone({ 0, 0, 100, 100 });
            winrt::com_ptr<IZone> zone4 = MakeZone({ 10, 10, 100, 100 });
            winrt::com_ptr<IZone> zone5 = MakeZone({ 20, 20, 100, 100 });
            
            zone3->AddWindowToZone(window, zoneWindow, true);

            set->AddZone(zone1);
            set->AddZone(zone2);
            set->AddZone(zone3);
            set->AddZone(zone4);
            set->AddZone(zone5);

            const int expected = 2;
            auto actual = set->GetZoneIndexFromWindow(window);
            Assert::AreEqual(expected, actual);
        }

        TEST_METHOD(ZoneIndexFromWindowWithEqualWindows)
        {
            winrt::com_ptr<IZoneSet> set = MakeZoneSet(*m_config);

            HWND window = Mocks::WindowCreate(m_hInst);
            HWND zoneWindow = Mocks::WindowCreate(m_hInst);

            winrt::com_ptr<IZone> zone1 = MakeZone({ 0, 0, 100, 100 });
            winrt::com_ptr<IZone> zone2 = MakeZone({ 20, 20, 200, 200 });
            winrt::com_ptr<IZone> zone3 = MakeZone({ 0, 0, 100, 100 });
            winrt::com_ptr<IZone> zone4 = MakeZone({ 10, 10, 100, 100 });
            winrt::com_ptr<IZone> zone5 = MakeZone({ 20, 20, 100, 100 });

            zone3->AddWindowToZone(window, zoneWindow, true);
            zone4->AddWindowToZone(window, zoneWindow, true);

            set->AddZone(zone1);
            set->AddZone(zone2);
            set->AddZone(zone3);
            set->AddZone(zone4);
            set->AddZone(zone5);

            const int expected = 2;
            auto actual = set->GetZoneIndexFromWindow(window);
            Assert::AreEqual(expected, actual);
        }

        TEST_METHOD(ZoneIndexFromWindowUnknown)
        {
            winrt::com_ptr<IZoneSet> set = MakeZoneSet(*m_config);
            
            winrt::com_ptr<IZone> zone = MakeZone({ 0, 0, 100, 100 });
            HWND window = Mocks::WindowCreate(m_hInst);
            HWND zoneWindow = Mocks::WindowCreate(m_hInst);
            zone->AddWindowToZone(window, zoneWindow, true);
            set->AddZone(zone);

            const int expected = -1;
            auto actual = set->GetZoneIndexFromWindow(Mocks::WindowCreate(m_hInst));
            Assert::AreEqual(expected, actual);
        }

        TEST_METHOD(ZoneIndexFromWindowNull)
        {
            winrt::com_ptr<IZoneSet> set = MakeZoneSet(*m_config);

            winrt::com_ptr<IZone> zone = MakeZone({ 0, 0, 100, 100 });
            HWND window = Mocks::WindowCreate(m_hInst);
            HWND zoneWindow = Mocks::WindowCreate(m_hInst);
            zone->AddWindowToZone(window, zoneWindow, true);
            set->AddZone(zone);

            const int expected = -1;
            auto actual = set->GetZoneIndexFromWindow(nullptr);
            Assert::AreEqual(expected, actual);
        }
        
        TEST_METHOD(TestMoveWindowIntoZoneByIndex)
        {
            ZoneSetConfig config({}, 0xFFFF, Mocks::Monitor(), L"WorkAreaIn");
            winrt::com_ptr<IZoneSet> set = MakeZoneSet(config);

            // Add a couple of zones.
            winrt::com_ptr<IZone> zone1 = MakeZone({ 0, 0, 100, 100 });
            winrt::com_ptr<IZone> zone2 = MakeZone({ 0, 0, 100, 100 });
            winrt::com_ptr<IZone> zone3 = MakeZone({ 0, 0, 100, 100 });
            set->AddZone(zone1);
            set->AddZone(zone2);
            set->AddZone(zone3);

            HWND window = Mocks::Window();
            set->MoveWindowIntoZoneByIndex(window, Mocks::Window(), 1);
            Assert::IsFalse(zone1->ContainsWindow(window));
            Assert::IsTrue(zone2->ContainsWindow(window));
            Assert::IsFalse(zone3->ContainsWindow(window));
        }

        TEST_METHOD(TestMoveWindowIntoZoneByIndexWithNoZones)
        {
            ZoneSetConfig config({}, 0xFFFF, Mocks::Monitor(), L"WorkAreaIn");
            winrt::com_ptr<IZoneSet> set = MakeZoneSet(config);

            // Add a couple of zones.
            HWND window = Mocks::Window();
            set->MoveWindowIntoZoneByIndex(window, Mocks::Window(), 0);
        }

        TEST_METHOD(TestMoveWindowIntoZoneByIndexWithInvalidIndex)
        {
            ZoneSetConfig config({}, 0xFFFF, Mocks::Monitor(), L"WorkAreaIn");
            winrt::com_ptr<IZoneSet> set = MakeZoneSet(config);

            // Add a couple of zones.
            winrt::com_ptr<IZone> zone1 = MakeZone({ 0, 0, 100, 100 });
            winrt::com_ptr<IZone> zone2 = MakeZone({ 0, 0, 100, 100 });
            winrt::com_ptr<IZone> zone3 = MakeZone({ 0, 0, 100, 100 });
            set->AddZone(zone1);
            set->AddZone(zone2);
            set->AddZone(zone3);

            HWND window = Mocks::Window();
            set->MoveWindowIntoZoneByIndex(window, Mocks::Window(), 100);
            Assert::IsTrue(zone1->ContainsWindow(window));
            Assert::IsFalse(zone2->ContainsWindow(window));
            Assert::IsFalse(zone3->ContainsWindow(window));
        }
    };

    // MoveWindowIntoZoneByDirection is complicated enough to warrant it's own test class
    TEST_CLASS(MoveWindowIntoZoneByDirectionUnitTests)
    {
        winrt::com_ptr<IZoneSet> set;
        winrt::com_ptr<IZone> zone1;
        winrt::com_ptr<IZone> zone2;
        winrt::com_ptr<IZone> zone3;

        TEST_METHOD_INITIALIZE(Initialize)
        {
            ZoneSetConfig config({}, 0xFFFF, Mocks::Monitor(), L"WorkAreaIn");
            set = MakeZoneSet(config);

            // Add a couple of zones.
            zone1 = MakeZone({ 0, 0, 100, 100 });
            zone2 = MakeZone({ 0, 0, 100, 100 });
            zone3 = MakeZone({ 0, 0, 100, 100 });
            set->AddZone(zone1);
            set->AddZone(zone2);
            set->AddZone(zone3);
        }

        TEST_METHOD(MoveWindowIntoZoneByDirectionRightNoZones)
        {
            HWND window = Mocks::Window();
            set->MoveWindowIntoZoneByDirection(window, Mocks::Window(), VK_RIGHT);
            Assert::IsTrue(zone1->ContainsWindow(window));
            Assert::IsFalse(zone2->ContainsWindow(window));
            Assert::IsFalse(zone3->ContainsWindow(window));
        }

        TEST_METHOD(MoveWindowIntoZoneByDirectionLeftNoZones)
        {
            HWND window = Mocks::Window();
            set->MoveWindowIntoZoneByDirection(window, Mocks::Window(), VK_LEFT);
            Assert::IsFalse(zone1->ContainsWindow(window));
            Assert::IsFalse(zone2->ContainsWindow(window));
            Assert::IsTrue(zone3->ContainsWindow(window));
        }

        TEST_METHOD(MoveWindowIntoZoneByDirectionRight)
        {
            HWND window = Mocks::Window();
            zone1->AddWindowToZone(window, Mocks::Window(), false /*stampZone*/);
            set->MoveWindowIntoZoneByDirection(window, Mocks::Window(), VK_RIGHT);
            Assert::IsFalse(zone1->ContainsWindow(window));
            Assert::IsTrue(zone2->ContainsWindow(window));
            Assert::IsFalse(zone3->ContainsWindow(window));

            set->MoveWindowIntoZoneByDirection(window, Mocks::Window(), VK_RIGHT);
            Assert::IsFalse(zone1->ContainsWindow(window));
            Assert::IsFalse(zone2->ContainsWindow(window));
            Assert::IsTrue(zone3->ContainsWindow(window));
        }

        TEST_METHOD(MoveWindowIntoZoneByDirectionLeft)
        {
            HWND window = Mocks::Window();
            zone3->AddWindowToZone(window, Mocks::Window(), false /*stampZone*/);
            set->MoveWindowIntoZoneByDirection(window, Mocks::Window(), VK_LEFT);
            Assert::IsFalse(zone1->ContainsWindow(window));
            Assert::IsTrue(zone2->ContainsWindow(window));
            Assert::IsFalse(zone3->ContainsWindow(window));

            set->MoveWindowIntoZoneByDirection(window, Mocks::Window(), VK_LEFT);
            Assert::IsTrue(zone1->ContainsWindow(window));
            Assert::IsFalse(zone2->ContainsWindow(window));
            Assert::IsFalse(zone3->ContainsWindow(window));
        }

        TEST_METHOD(MoveWindowIntoZoneByDirectionWrapAroundRight)
        {
            HWND window = Mocks::Window();
            zone3->AddWindowToZone(window, Mocks::Window(), false /*stampZone*/);
            set->MoveWindowIntoZoneByDirection(window, Mocks::Window(), VK_RIGHT);
            Assert::IsTrue(zone1->ContainsWindow(window));
            Assert::IsFalse(zone2->ContainsWindow(window));
            Assert::IsFalse(zone3->ContainsWindow(window));
        }

        TEST_METHOD(MoveWindowIntoZoneByDirectionWrapAroundLeft)
        {
            HWND window = Mocks::Window();
            zone1->AddWindowToZone(window, Mocks::Window(), false /*stampZone*/);
            set->MoveWindowIntoZoneByDirection(window, Mocks::Window(), VK_LEFT);
            Assert::IsFalse(zone1->ContainsWindow(window));
            Assert::IsFalse(zone2->ContainsWindow(window));
            Assert::IsTrue(zone3->ContainsWindow(window));
        }
    };
}
