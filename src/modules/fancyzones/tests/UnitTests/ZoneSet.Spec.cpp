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

        winrt::com_ptr<IZoneSet> m_set;

        TEST_METHOD_INITIALIZE(Init)
        {
            auto hres = CoCreateGuid(&m_id);
            Assert::AreEqual(S_OK, hres);

            ZoneSetConfig m_config = ZoneSetConfig(m_id, m_layoutId, Mocks::Monitor(), m_resolutionKey);
            m_set = MakeZoneSet(m_config);
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
            Assert::IsNotNull(&m_set);
            CustomAssert::AreEqual(m_set->Id(), m_id);
            CustomAssert::AreEqual(m_set->LayoutId(), m_layoutId);
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
            auto zones = m_set->GetZones();
            Assert::AreEqual((size_t)0, zones.size());
        }

        TEST_METHOD(AddOne)
        {
            winrt::com_ptr<IZone> zone = MakeZone({ 0, 0, 100, 100 });
            m_set->AddZone(zone);
            auto zones = m_set->GetZones();
            Assert::AreEqual((size_t)1, zones.size());
            compareZones(zone, zones[0]);
            Assert::AreEqual((size_t)1, zones[0]->Id());
        }

        TEST_METHOD(AddManySame)
        {
            winrt::com_ptr<IZone> zone = MakeZone({ 0, 0, 100, 100 });
            for (size_t i = 0; i < 1024; i++)
            {
                m_set->AddZone(zone);
                auto zones = m_set->GetZones();
                Assert::AreEqual(i + 1, zones.size());
                compareZones(zone, zones[i]);
                Assert::AreEqual(i + 1, zones[i]->Id());
            }
        }

        TEST_METHOD(AddManyEqual)
        {
            for (size_t i = 0; i < 1024; i++)
            {
                winrt::com_ptr<IZone> zone = MakeZone({ 0, 0, 100, 100 });
                m_set->AddZone(zone);
                auto zones = m_set->GetZones();
                Assert::AreEqual(i + 1, zones.size());
                compareZones(zone, zones[i]);
                Assert::AreEqual(i + 1, zones[i]->Id());
            }
        }

        TEST_METHOD(AddManyDifferent)
        {
            for (size_t i = 0; i < 1024; i++)
            {
                winrt::com_ptr<IZone> zone = MakeZone({ rand() % 10, rand() % 10, rand() % 100, rand() % 100 });
                m_set->AddZone(zone);
                auto zones = m_set->GetZones();
                Assert::AreEqual(i + 1, zones.size());
                compareZones(zone, zones[i]);
                Assert::AreEqual(i + 1, zones[i]->Id());
            }
        }

        TEST_METHOD(ZoneFromPointEmpty)
        {
            auto actual = m_set->ZoneFromPoint(POINT{ 0, 0 });
            Assert::IsTrue(nullptr == actual);
        }

        TEST_METHOD(ZoneFromPointInner)
        {
            const int left = 0, top = 0, right = 100, bottom = 100;
            winrt::com_ptr<IZone> expected = MakeZone({ left, top, right, bottom });
            m_set->AddZone(expected);

            for (int i = left + 1; i < right; i++)
            {
                for (int j = top + 1; j < bottom; j++)
                {
                    auto actual = m_set->ZoneFromPoint(POINT{ i, j });
                    Assert::IsTrue(actual != nullptr);
                    compareZones(expected, actual);
                }
            }
        }

        TEST_METHOD(ZoneFromPointBorder)
        {
            const int left = 0, top = 0, right = 100, bottom = 100;
            winrt::com_ptr<IZone> expected = MakeZone({ left, top, right, bottom });
            m_set->AddZone(expected);

            for (int i = left; i < right; i++)
            {
                auto actual = m_set->ZoneFromPoint(POINT{ i, top });
                Assert::IsTrue(actual != nullptr);
                compareZones(expected, actual);
            }

            for (int i = top; i < bottom; i++)
            {
                auto actual = m_set->ZoneFromPoint(POINT{ left, i });
                Assert::IsTrue(actual != nullptr);
                compareZones(expected, actual);
            }

            //bottom and right borders considered to be outside
            for (int i = left; i < right; i++)
            {
                auto actual = m_set->ZoneFromPoint(POINT{ i, bottom });
                Assert::IsTrue(nullptr == actual);
            }

            for (int i = top; i < bottom; i++)
            {
                auto actual = m_set->ZoneFromPoint(POINT{ right, i });
                Assert::IsTrue(nullptr == actual);
            }
        }

        TEST_METHOD(ZoneFromPointOuter)
        {
            const int left = 0, top = 0, right = 100, bottom = 100;
            winrt::com_ptr<IZone> zone = MakeZone({ left, top, right, bottom });
            m_set->AddZone(zone);

            auto actual = m_set->ZoneFromPoint(POINT{ 101, 101 });
            Assert::IsTrue(actual == nullptr);
        }

        TEST_METHOD(ZoneFromPointOverlapping)
        {
            winrt::com_ptr<IZone> zone1 = MakeZone({ 0, 0, 100, 100 });
            m_set->AddZone(zone1);
            winrt::com_ptr<IZone> zone2 = MakeZone({ 10, 10, 90, 90 });
            m_set->AddZone(zone2);
            winrt::com_ptr<IZone> zone3 = MakeZone({ 10, 10, 150, 150 });
            m_set->AddZone(zone3);
            winrt::com_ptr<IZone> zone4 = MakeZone({ 10, 10, 50, 50 });
            m_set->AddZone(zone4);

            auto actual = m_set->ZoneFromPoint(POINT{ 50, 50 });
            Assert::IsTrue(actual != nullptr);
            compareZones(zone2, actual);
        }

        TEST_METHOD(ZoneFromPointWithNotNormalizedRect)
        {
            winrt::com_ptr<IZone> zone = MakeZone({ 100, 100, 0, 0 });
            m_set->AddZone(zone);

            auto actual = m_set->ZoneFromPoint(POINT{ 50, 50 });
            Assert::IsTrue(actual == nullptr);
        }

        TEST_METHOD(ZoneFromPointWithZeroRect)
        {
            winrt::com_ptr<IZone> zone = MakeZone({ 0, 0, 0, 0 });
            m_set->AddZone(zone);

            auto actual = m_set->ZoneFromPoint(POINT{ 0, 0 });
            Assert::IsTrue(actual == nullptr);
        }

        TEST_METHOD(ZoneIndexFromWindow)
        {
            HWND window = Mocks::Window();
            HWND zoneWindow = Mocks::Window();

            winrt::com_ptr<IZone> zone1 = MakeZone({ 0, 0, 100, 100 });
            winrt::com_ptr<IZone> zone2 = MakeZone({ 20, 20, 200, 200 });
            winrt::com_ptr<IZone> zone3 = MakeZone({ 0, 0, 100, 100 });
            winrt::com_ptr<IZone> zone4 = MakeZone({ 10, 10, 100, 100 });
            winrt::com_ptr<IZone> zone5 = MakeZone({ 20, 20, 100, 100 });

            zone3->AddWindowToZone(window, zoneWindow, true);

            m_set->AddZone(zone1);
            m_set->AddZone(zone2);
            m_set->AddZone(zone3);
            m_set->AddZone(zone4);
            m_set->AddZone(zone5);

            const int expected = 2;
            auto actual = m_set->GetZoneIndexFromWindow(window);
            Assert::AreEqual(expected, actual);
        }

        TEST_METHOD(ZoneIndexFromWindowWithEqualWindows)
        {
            HWND window = Mocks::Window();
            HWND zoneWindow = Mocks::Window();

            winrt::com_ptr<IZone> zone1 = MakeZone({ 0, 0, 100, 100 });
            winrt::com_ptr<IZone> zone2 = MakeZone({ 20, 20, 200, 200 });
            winrt::com_ptr<IZone> zone3 = MakeZone({ 0, 0, 100, 100 });
            winrt::com_ptr<IZone> zone4 = MakeZone({ 10, 10, 100, 100 });
            winrt::com_ptr<IZone> zone5 = MakeZone({ 20, 20, 100, 100 });

            zone3->AddWindowToZone(window, zoneWindow, true);
            zone4->AddWindowToZone(window, zoneWindow, true);

            m_set->AddZone(zone1);
            m_set->AddZone(zone2);
            m_set->AddZone(zone3);
            m_set->AddZone(zone4);
            m_set->AddZone(zone5);

            const int expected = 2;
            auto actual = m_set->GetZoneIndexFromWindow(window);
            Assert::AreEqual(expected, actual);
        }

        TEST_METHOD(ZoneIndexFromWindowUnknown)
        {
            winrt::com_ptr<IZone> zone = MakeZone({ 0, 0, 100, 100 });
            HWND window = Mocks::Window();
            HWND zoneWindow = Mocks::Window();
            zone->AddWindowToZone(window, zoneWindow, true);
            m_set->AddZone(zone);

            const int expected = -1;
            auto actual = m_set->GetZoneIndexFromWindow(Mocks::Window());
            Assert::AreEqual(expected, actual);
        }

        TEST_METHOD(ZoneIndexFromWindowNull)
        {
            winrt::com_ptr<IZone> zone = MakeZone({ 0, 0, 100, 100 });
            HWND window = Mocks::Window();
            HWND zoneWindow = Mocks::Window();
            zone->AddWindowToZone(window, zoneWindow, true);
            m_set->AddZone(zone);

            const int expected = -1;
            auto actual = m_set->GetZoneIndexFromWindow(nullptr);
            Assert::AreEqual(expected, actual);
        }

        TEST_METHOD(MoveWindowIntoZoneByIndex)
        {
            // Add a couple of zones.
            winrt::com_ptr<IZone> zone1 = MakeZone({ 0, 0, 100, 100 });
            winrt::com_ptr<IZone> zone2 = MakeZone({ 0, 0, 100, 100 });
            winrt::com_ptr<IZone> zone3 = MakeZone({ 0, 0, 100, 100 });
            m_set->AddZone(zone1);
            m_set->AddZone(zone2);
            m_set->AddZone(zone3);

            HWND window = Mocks::Window();
            m_set->MoveWindowIntoZoneByIndex(window, Mocks::Window(), 1);
            Assert::IsFalse(zone1->ContainsWindow(window));
            Assert::IsTrue(zone2->ContainsWindow(window));
            Assert::IsFalse(zone3->ContainsWindow(window));
        }

        TEST_METHOD(MoveWindowIntoZoneByIndexWithNoZones)
        {
            // Add a couple of zones.
            HWND window = Mocks::Window();
            m_set->MoveWindowIntoZoneByIndex(window, Mocks::Window(), 0);
        }

        TEST_METHOD(MoveWindowIntoZoneByIndexWithInvalidIndex)
        {
            // Add a couple of zones.
            winrt::com_ptr<IZone> zone1 = MakeZone({ 0, 0, 100, 100 });
            winrt::com_ptr<IZone> zone2 = MakeZone({ 0, 0, 100, 100 });
            winrt::com_ptr<IZone> zone3 = MakeZone({ 0, 0, 100, 100 });
            m_set->AddZone(zone1);
            m_set->AddZone(zone2);
            m_set->AddZone(zone3);

            HWND window = Mocks::Window();
            m_set->MoveWindowIntoZoneByIndex(window, Mocks::Window(), 100);
            Assert::IsTrue(zone1->ContainsWindow(window));
            Assert::IsFalse(zone2->ContainsWindow(window));
            Assert::IsFalse(zone3->ContainsWindow(window));
        }

        TEST_METHOD(MoveWindowIntoZoneByIndexSeveralTimesSameWindow)
        {
            // Add a couple of zones.
            winrt::com_ptr<IZone> zone1 = MakeZone({ 0, 0, 100, 100 });
            winrt::com_ptr<IZone> zone2 = MakeZone({ 1, 1, 101, 101 });
            winrt::com_ptr<IZone> zone3 = MakeZone({ 2, 2, 102, 102 });
            m_set->AddZone(zone1);
            m_set->AddZone(zone2);
            m_set->AddZone(zone3);

            HWND window = Mocks::Window();
            m_set->MoveWindowIntoZoneByIndex(window, Mocks::Window(), 0);
            m_set->MoveWindowIntoZoneByIndex(window, Mocks::Window(), 1);
            m_set->MoveWindowIntoZoneByIndex(window, Mocks::Window(), 2);
            Assert::IsTrue(zone1->ContainsWindow(window));
            Assert::IsTrue(zone2->ContainsWindow(window));
            Assert::IsTrue(zone3->ContainsWindow(window));
        }

        TEST_METHOD(MoveWindowIntoZoneByIndexSeveralTimesSameIndex)
        {
            // Add a couple of zones.
            winrt::com_ptr<IZone> zone1 = MakeZone({ 0, 0, 100, 100 });
            winrt::com_ptr<IZone> zone2 = MakeZone({ 1, 1, 101, 101 });
            winrt::com_ptr<IZone> zone3 = MakeZone({ 2, 2, 102, 102 });
            m_set->AddZone(zone1);
            m_set->AddZone(zone2);
            m_set->AddZone(zone3);

            HWND window = Mocks::Window();
            m_set->MoveWindowIntoZoneByIndex(window, Mocks::Window(), 0);
            m_set->MoveWindowIntoZoneByIndex(window, Mocks::Window(), 0);
            m_set->MoveWindowIntoZoneByIndex(window, Mocks::Window(), 0);
            Assert::IsTrue(zone1->ContainsWindow(window));
            Assert::IsFalse(zone2->ContainsWindow(window));
            Assert::IsFalse(zone3->ContainsWindow(window));
        }
    };

    // MoveWindowIntoZoneByDirection is complicated enough to warrant it's own test class
    TEST_CLASS(ZoneSetsMoveWindowIntoZoneByDirectionUnitTests)
    {
        winrt::com_ptr<IZoneSet> m_set;
        winrt::com_ptr<IZone> m_zone1;
        winrt::com_ptr<IZone> m_zone2;
        winrt::com_ptr<IZone> m_zone3;

        TEST_METHOD_INITIALIZE(Initialize)
        {
            ZoneSetConfig config({}, 0xFFFF, Mocks::Monitor(), L"WorkAreaIn");
            m_set = MakeZoneSet(config);

            // Add a couple of zones.
            m_zone1 = MakeZone({ 0, 0, 100, 100 });
            m_zone2 = MakeZone({ 0, 0, 100, 100 });
            m_zone3 = MakeZone({ 0, 0, 100, 100 });
            m_set->AddZone(m_zone1);
            m_set->AddZone(m_zone2);
            m_set->AddZone(m_zone3);
        }

        TEST_METHOD(EmptyZonesLeft)
        {
            ZoneSetConfig config({}, 0xFFFF, Mocks::Monitor(), L"WorkAreaIn");
            auto set = MakeZoneSet(config);

            set->MoveWindowIntoZoneByDirection(Mocks::Window(), Mocks::Window(), VK_LEFT);
        }

        TEST_METHOD(EmptyZonesRight)
        {
            ZoneSetConfig config({}, 0xFFFF, Mocks::Monitor(), L"WorkAreaIn");
            auto set = MakeZoneSet(config);

            set->MoveWindowIntoZoneByDirection(Mocks::Window(), Mocks::Window(), VK_RIGHT);
        }

        TEST_METHOD(MoveRightNoZones)
        {
            HWND window = Mocks::Window();
            m_set->MoveWindowIntoZoneByDirection(window, Mocks::Window(), VK_RIGHT);
            Assert::IsTrue(m_zone1->ContainsWindow(window));
            Assert::IsFalse(m_zone2->ContainsWindow(window));
            Assert::IsFalse(m_zone3->ContainsWindow(window));
        }

        TEST_METHOD(MoveLeftNoZones)
        {
            HWND window = Mocks::Window();
            m_set->MoveWindowIntoZoneByDirection(window, Mocks::Window(), VK_LEFT);
            Assert::IsFalse(m_zone1->ContainsWindow(window));
            Assert::IsFalse(m_zone2->ContainsWindow(window));
            Assert::IsTrue(m_zone3->ContainsWindow(window));
        }

        TEST_METHOD(MoveRightTwice)
        {
            HWND window = Mocks::Window();
            m_set->MoveWindowIntoZoneByDirection(window, Mocks::Window(), VK_RIGHT);
            m_set->MoveWindowIntoZoneByDirection(window, Mocks::Window(), VK_RIGHT);
            Assert::IsFalse(m_zone1->ContainsWindow(window));
            Assert::IsTrue(m_zone2->ContainsWindow(window));
            Assert::IsFalse(m_zone3->ContainsWindow(window));
        }

        TEST_METHOD(MoveLeftTwice)
        {
            HWND window = Mocks::Window();
            m_set->MoveWindowIntoZoneByDirection(window, Mocks::Window(), VK_LEFT);
            m_set->MoveWindowIntoZoneByDirection(window, Mocks::Window(), VK_LEFT);
            Assert::IsFalse(m_zone1->ContainsWindow(window));
            Assert::IsTrue(m_zone2->ContainsWindow(window));
            Assert::IsFalse(m_zone3->ContainsWindow(window));
        }

        TEST_METHOD(MoveRightMoreThanZonesCount)
        {
            HWND window = Mocks::Window();
            for (int i = 0; i <= m_set->GetZones().size(); i++)
            {
                m_set->MoveWindowIntoZoneByDirection(window, Mocks::Window(), VK_RIGHT);
            }

            Assert::IsTrue(m_zone1->ContainsWindow(window));
            Assert::IsFalse(m_zone2->ContainsWindow(window));
            Assert::IsFalse(m_zone3->ContainsWindow(window));
        }

        TEST_METHOD(MoveLeftMoreThanZonesCount)
        {
            HWND window = Mocks::Window();
            for (int i = 0; i <= m_set->GetZones().size(); i++)
            {
                m_set->MoveWindowIntoZoneByDirection(window, Mocks::Window(), VK_LEFT);
            }

            Assert::IsFalse(m_zone1->ContainsWindow(window));
            Assert::IsFalse(m_zone2->ContainsWindow(window));
            Assert::IsTrue(m_zone3->ContainsWindow(window));
        }

        TEST_METHOD(MoveWindowIntoZoneByDirectionRight)
        {
            HWND window = Mocks::Window();
            m_zone1->AddWindowToZone(window, Mocks::Window(), false /*stampZone*/);
            m_set->MoveWindowIntoZoneByDirection(window, Mocks::Window(), VK_RIGHT);
            Assert::IsFalse(m_zone1->ContainsWindow(window));
            Assert::IsTrue(m_zone2->ContainsWindow(window));
            Assert::IsFalse(m_zone3->ContainsWindow(window));

            m_set->MoveWindowIntoZoneByDirection(window, Mocks::Window(), VK_RIGHT);
            Assert::IsFalse(m_zone1->ContainsWindow(window));
            Assert::IsFalse(m_zone2->ContainsWindow(window));
            Assert::IsTrue(m_zone3->ContainsWindow(window));
        }

        TEST_METHOD(MoveWindowIntoZoneByDirectionLeft)
        {
            HWND window = Mocks::Window();
            m_zone3->AddWindowToZone(window, Mocks::Window(), false /*stampZone*/);
            m_set->MoveWindowIntoZoneByDirection(window, Mocks::Window(), VK_LEFT);
            Assert::IsFalse(m_zone1->ContainsWindow(window));
            Assert::IsTrue(m_zone2->ContainsWindow(window));
            Assert::IsFalse(m_zone3->ContainsWindow(window));

            m_set->MoveWindowIntoZoneByDirection(window, Mocks::Window(), VK_LEFT);
            Assert::IsTrue(m_zone1->ContainsWindow(window));
            Assert::IsFalse(m_zone2->ContainsWindow(window));
            Assert::IsFalse(m_zone3->ContainsWindow(window));
        }

        TEST_METHOD(MoveWindowIntoZoneByDirectionWrapAroundRight)
        {
            HWND window = Mocks::Window();
            m_zone3->AddWindowToZone(window, Mocks::Window(), false /*stampZone*/);
            m_set->MoveWindowIntoZoneByDirection(window, Mocks::Window(), VK_RIGHT);
            Assert::IsTrue(m_zone1->ContainsWindow(window));
            Assert::IsFalse(m_zone2->ContainsWindow(window));
            Assert::IsFalse(m_zone3->ContainsWindow(window));
        }

        TEST_METHOD(MoveWindowIntoZoneByDirectionWrapAroundLeft)
        {
            HWND window = Mocks::Window();
            m_zone1->AddWindowToZone(window, Mocks::Window(), false /*stampZone*/);
            m_set->MoveWindowIntoZoneByDirection(window, Mocks::Window(), VK_LEFT);
            Assert::IsFalse(m_zone1->ContainsWindow(window));
            Assert::IsFalse(m_zone2->ContainsWindow(window));
            Assert::IsTrue(m_zone3->ContainsWindow(window));
        }

        TEST_METHOD(MoveSecondWindowIntoSameZone)
        {
            HWND window1 = Mocks::Window();
            m_zone1->AddWindowToZone(window1, Mocks::Window(), false /*stampZone*/);

            HWND window2 = Mocks::Window();
            m_set->MoveWindowIntoZoneByDirection(window2, Mocks::Window(), VK_RIGHT);

            Assert::IsTrue(m_zone1->ContainsWindow(window1));
            Assert::IsFalse(m_zone2->ContainsWindow(window1));
            Assert::IsFalse(m_zone3->ContainsWindow(window1));

            Assert::IsTrue(m_zone1->ContainsWindow(window2));
            Assert::IsFalse(m_zone2->ContainsWindow(window2));
            Assert::IsFalse(m_zone3->ContainsWindow(window2));
        }
    };
}
