#include "pch.h"
#include <lib/JsonHelpers.h>

#include <CppUnitTestLogger.h>

using namespace JSONHelpers;
using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace FancyZonesUnitTests
{
    TEST_CLASS(ZoneSetDataUnitTest){

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
}