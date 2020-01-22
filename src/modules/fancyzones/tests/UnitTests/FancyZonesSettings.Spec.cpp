#include "pch.h"

#include <lib/Settings.h>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace FancyZonesUnitTests
{
    TEST_CLASS(FancyZonesSettingsUnitTest)
    {
        HINSTANCE m_hInst;

        TEST_METHOD_INITIALIZE(Init)
        {
            m_hInst = (HINSTANCE)GetModuleHandleW(nullptr);
        }
    };
}