#include "pch.h"
#include <filesystem>

#include <lib/Settings.h>
#include <common/settings_helpers.h>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace FancyZonesUnitTests
{
    TEST_CLASS(FancyZonesSettingsCreationUnitTest)
    {
        HINSTANCE m_hInst;
        PCWSTR m_moduleName = L"FancyZonesTest";
        std::wstring m_tmpName;

        const PowerToysSettings::HotkeyObject m_defaultHotkeyObject = PowerToysSettings::HotkeyObject::from_settings(true, false, false, false, VK_OEM_3);
        const Settings m_defaultSettings = Settings{
            .shiftDrag = true,
            .displayChange_moveWindows = false,
            .virtualDesktopChange_moveWindows = false,
            .zoneSetChange_flashZones = false,
            .zoneSetChange_moveWindows = false,
            .overrideSnapHotkeys = false,
            .appLastZone_moveWindows = false,
            .use_cursorpos_editor_startupscreen = true,
            .zoneHightlightColor = L"#0078D7",
            .zoneHighlightOpacity = 90,
            .editorHotkey = m_defaultHotkeyObject,
            .excludedApps = L"",
            .excludedAppsArray = {},
        };

        void compareHotkeyObjects(const PowerToysSettings::HotkeyObject& expected, const PowerToysSettings::HotkeyObject& actual)
        {
            Assert::AreEqual(expected.alt_pressed(), actual.alt_pressed());
            Assert::AreEqual(expected.ctrl_pressed(), actual.ctrl_pressed());
            Assert::AreEqual(expected.shift_pressed(), actual.shift_pressed());
            Assert::AreEqual(expected.win_pressed(), actual.win_pressed());

            //NOTE: key_from_code may create different values
            //Assert::AreEqual(expected.get_key(), actual.get_key());
            Assert::AreEqual(expected.get_code(), actual.get_code());
            Assert::AreEqual(expected.get_modifiers(), actual.get_modifiers());
            Assert::AreEqual(expected.get_modifiers_repeat(), actual.get_modifiers_repeat());
        }

        void compareSettings(const Settings& expected, const Settings& actual)
        {
            Assert::AreEqual(expected.shiftDrag, actual.shiftDrag);
            Assert::AreEqual(expected.displayChange_moveWindows, actual.displayChange_moveWindows);
            Assert::AreEqual(expected.virtualDesktopChange_moveWindows, actual.virtualDesktopChange_moveWindows);
            Assert::AreEqual(expected.zoneSetChange_flashZones, actual.zoneSetChange_flashZones);
            Assert::AreEqual(expected.zoneSetChange_moveWindows, actual.zoneSetChange_moveWindows);
            Assert::AreEqual(expected.overrideSnapHotkeys, actual.overrideSnapHotkeys);
            Assert::AreEqual(expected.appLastZone_moveWindows, actual.appLastZone_moveWindows);
            Assert::AreEqual(expected.use_cursorpos_editor_startupscreen, actual.use_cursorpos_editor_startupscreen);
            Assert::AreEqual(expected.zoneHightlightColor.c_str(), actual.zoneHightlightColor.c_str());
            Assert::AreEqual(expected.zoneHighlightOpacity, actual.zoneHighlightOpacity);
            Assert::AreEqual(expected.excludedApps.c_str(), actual.excludedApps.c_str());
            Assert::AreEqual(expected.excludedAppsArray.size(), actual.excludedAppsArray.size());
            for (int i = 0; i < expected.excludedAppsArray.size(); i++)
            {
                Assert::AreEqual(expected.excludedAppsArray[i].c_str(), actual.excludedAppsArray[i].c_str());
            }

            compareHotkeyObjects(expected.editorHotkey, actual.editorHotkey);
        }

        TEST_METHOD_INITIALIZE(Init)
        {
            m_hInst = (HINSTANCE)GetModuleHandleW(nullptr);
            m_tmpName = PTSettingsHelper::get_module_save_folder_location(m_moduleName) + L"\\settings.json";
        }

        TEST_METHOD_CLEANUP(Cleanup)
        {
            std::filesystem::remove(m_tmpName);
        }

        TEST_METHOD(CreateWithHinstanceDefault)
        {
            auto actual = MakeFancyZonesSettings({}, m_moduleName);
            Assert::IsTrue(actual != nullptr);

            auto actualSettings = actual->GetSettings();
            compareSettings(m_defaultSettings, actualSettings);
        }

        TEST_METHOD(CreateWithHinstanceNullptr)
        {
            auto actual = MakeFancyZonesSettings(nullptr, m_moduleName);
            Assert::IsTrue(actual != nullptr);

            auto actualSettings = actual->GetSettings();
            compareSettings(m_defaultSettings, actualSettings);
        }

        TEST_METHOD(CreateWithNameEmpty)
        {
            auto actual = MakeFancyZonesSettings(m_hInst, L"");
            Assert::IsTrue(actual != nullptr);

            auto actualSettings = actual->GetSettings();
            compareSettings(m_defaultSettings, actualSettings);
        }

        TEST_METHOD(CreateWithNameNullptr)
        {
            auto actual = MakeFancyZonesSettings(m_hInst, nullptr);
            Assert::IsTrue(actual == nullptr);
        }

        TEST_METHOD(Create)
        {
            //prepare data
            const Settings expected {
                .shiftDrag = false,
                .displayChange_moveWindows = true,
                .virtualDesktopChange_moveWindows = true,
                .zoneSetChange_flashZones = true,
                .zoneSetChange_moveWindows = true,
                .overrideSnapHotkeys = false,
                .appLastZone_moveWindows = false,
                .use_cursorpos_editor_startupscreen = true,
                .zoneHightlightColor = L"#00FFD7",
                .zoneHighlightOpacity = 45,
                .editorHotkey = PowerToysSettings::HotkeyObject::from_settings(false, true, true, false, VK_OEM_3),
                .excludedApps = L"app",
                .excludedAppsArray = { L"APP" },
            };

            PowerToysSettings::PowerToyValues values(m_moduleName);
            values.add_property(L"fancyzones_shiftDrag", expected.shiftDrag);
            values.add_property(L"fancyzones_displayChange_moveWindows", expected.displayChange_moveWindows);
            values.add_property(L"fancyzones_virtualDesktopChange_moveWindows", expected.virtualDesktopChange_moveWindows);
            values.add_property(L"fancyzones_zoneSetChange_flashZones", expected.zoneSetChange_flashZones);
            values.add_property(L"fancyzones_zoneSetChange_moveWindows", expected.zoneSetChange_moveWindows);
            values.add_property(L"fancyzones_overrideSnapHotkeys", expected.overrideSnapHotkeys);
            values.add_property(L"fancyzones_appLastZone_moveWindows", expected.appLastZone_moveWindows);
            values.add_property(L"use_cursorpos_editor_startupscreen", expected.use_cursorpos_editor_startupscreen);
            values.add_property(L"fancyzones_zoneHighlightColor", expected.zoneHightlightColor);
            values.add_property(L"fancyzones_highlight_opacity", expected.zoneHighlightOpacity);
            values.add_property(L"fancyzones_editor_hotkey", expected.editorHotkey.get_json());
            values.add_property(L"fancyzones_excluded_apps", expected.excludedApps);

            values.save_to_settings_file();

            auto actual = MakeFancyZonesSettings(m_hInst, m_moduleName);
            Assert::IsTrue(actual != nullptr);

            auto actualSettings = actual->GetSettings();
            compareSettings(expected, actualSettings);
        }

        TEST_METHOD(CreateWithMultipleApps)
        {
            //prepare data
            const Settings expected {
                .shiftDrag = false,
                .displayChange_moveWindows = true,
                .virtualDesktopChange_moveWindows = true,
                .zoneSetChange_flashZones = true,
                .zoneSetChange_moveWindows = true,
                .overrideSnapHotkeys = false,
                .appLastZone_moveWindows = false,
                .use_cursorpos_editor_startupscreen = true,
                .zoneHightlightColor = L"#00FFD7",
                .zoneHighlightOpacity = 45,
                .editorHotkey = PowerToysSettings::HotkeyObject::from_settings(false, true, true, false, VK_OEM_3),
                .excludedApps = L"app\r\napp1\r\napp2\r\nanother app",
                .excludedAppsArray = { L"APP", L"APP1", L"APP2", L"ANOTHER APP" },
            };

            PowerToysSettings::PowerToyValues values(m_moduleName);
            values.add_property(L"fancyzones_shiftDrag", expected.shiftDrag);
            values.add_property(L"fancyzones_displayChange_moveWindows", expected.displayChange_moveWindows);
            values.add_property(L"fancyzones_virtualDesktopChange_moveWindows", expected.virtualDesktopChange_moveWindows);
            values.add_property(L"fancyzones_zoneSetChange_flashZones", expected.zoneSetChange_flashZones);
            values.add_property(L"fancyzones_zoneSetChange_moveWindows", expected.zoneSetChange_moveWindows);
            values.add_property(L"fancyzones_overrideSnapHotkeys", expected.overrideSnapHotkeys);
            values.add_property(L"fancyzones_appLastZone_moveWindows", expected.appLastZone_moveWindows);
            values.add_property(L"use_cursorpos_editor_startupscreen", expected.use_cursorpos_editor_startupscreen);
            values.add_property(L"fancyzones_zoneHighlightColor", expected.zoneHightlightColor);
            values.add_property(L"fancyzones_highlight_opacity", expected.zoneHighlightOpacity);
            values.add_property(L"fancyzones_editor_hotkey", expected.editorHotkey.get_json());
            values.add_property(L"fancyzones_excluded_apps", expected.excludedApps);

            values.save_to_settings_file();

            auto actual = MakeFancyZonesSettings(m_hInst, m_moduleName);
            Assert::IsTrue(actual != nullptr);

            auto actualSettings = actual->GetSettings();
            compareSettings(expected, actualSettings);
        }

        TEST_METHOD(CreateWithBoolValuesMissed)
        {
            const Settings expected {
                .shiftDrag = m_defaultSettings.shiftDrag,
                .displayChange_moveWindows = m_defaultSettings.displayChange_moveWindows,
                .virtualDesktopChange_moveWindows = m_defaultSettings.virtualDesktopChange_moveWindows,
                .zoneSetChange_flashZones = m_defaultSettings.zoneSetChange_flashZones,
                .zoneSetChange_moveWindows = m_defaultSettings.zoneSetChange_moveWindows,
                .overrideSnapHotkeys = m_defaultSettings.overrideSnapHotkeys,
                .appLastZone_moveWindows = m_defaultSettings.appLastZone_moveWindows,
                .use_cursorpos_editor_startupscreen = m_defaultSettings.use_cursorpos_editor_startupscreen,
                .zoneHightlightColor = L"#00FFD7",
                .zoneHighlightOpacity = 45,
                .editorHotkey = PowerToysSettings::HotkeyObject::from_settings(false, true, true, false, VK_OEM_3),
                .excludedApps = L"app",
                .excludedAppsArray = { L"APP" },
            };

            PowerToysSettings::PowerToyValues values(m_moduleName);
            values.add_property(L"fancyzones_zoneHighlightColor", expected.zoneHightlightColor);
            values.add_property(L"fancyzones_highlight_opacity", expected.zoneHighlightOpacity);
            values.add_property(L"fancyzones_editor_hotkey", expected.editorHotkey.get_json());
            values.add_property(L"fancyzones_excluded_apps", expected.excludedApps);

            values.save_to_settings_file();

            auto actual = MakeFancyZonesSettings(m_hInst, m_moduleName);
            Assert::IsTrue(actual != nullptr);

            auto actualSettings = actual->GetSettings();
            compareSettings(expected, actualSettings);
        }

        TEST_METHOD(CreateColorMissed)
        {
            //prepare data
            const Settings expected {
                .shiftDrag = false,
                .displayChange_moveWindows = true,
                .virtualDesktopChange_moveWindows = true,
                .zoneSetChange_flashZones = true,
                .zoneSetChange_moveWindows = true,
                .overrideSnapHotkeys = false,
                .appLastZone_moveWindows = false,
                .use_cursorpos_editor_startupscreen = true,
                .zoneHightlightColor = m_defaultSettings.zoneHightlightColor,
                .zoneHighlightOpacity = 45,
                .editorHotkey = PowerToysSettings::HotkeyObject::from_settings(false, true, true, false, VK_OEM_3),
                .excludedApps = L"app",
                .excludedAppsArray = { L"APP" },
            };

            PowerToysSettings::PowerToyValues values(m_moduleName);
            values.add_property(L"fancyzones_shiftDrag", expected.shiftDrag);
            values.add_property(L"fancyzones_displayChange_moveWindows", expected.displayChange_moveWindows);
            values.add_property(L"fancyzones_virtualDesktopChange_moveWindows", expected.virtualDesktopChange_moveWindows);
            values.add_property(L"fancyzones_zoneSetChange_flashZones", expected.zoneSetChange_flashZones);
            values.add_property(L"fancyzones_zoneSetChange_moveWindows", expected.zoneSetChange_moveWindows);
            values.add_property(L"fancyzones_overrideSnapHotkeys", expected.overrideSnapHotkeys);
            values.add_property(L"fancyzones_appLastZone_moveWindows", expected.appLastZone_moveWindows);
            values.add_property(L"use_cursorpos_editor_startupscreen", expected.use_cursorpos_editor_startupscreen);
            values.add_property(L"fancyzones_highlight_opacity", expected.zoneHighlightOpacity);
            values.add_property(L"fancyzones_editor_hotkey", expected.editorHotkey.get_json());
            values.add_property(L"fancyzones_excluded_apps", expected.excludedApps);

            values.save_to_settings_file();

            auto actual = MakeFancyZonesSettings(m_hInst, m_moduleName);
            Assert::IsTrue(actual != nullptr);

            auto actualSettings = actual->GetSettings();
            compareSettings(expected, actualSettings);
        }

        TEST_METHOD(CreateOpacityMissed)
        {
            //prepare data
            const Settings expected {
                .shiftDrag = false,
                .displayChange_moveWindows = true,
                .virtualDesktopChange_moveWindows = true,
                .zoneSetChange_flashZones = true,
                .zoneSetChange_moveWindows = true,
                .overrideSnapHotkeys = false,
                .appLastZone_moveWindows = false,
                .use_cursorpos_editor_startupscreen = true,
                .zoneHightlightColor = L"#00FFD7",
                .zoneHighlightOpacity = m_defaultSettings.zoneHighlightOpacity,
                .editorHotkey = PowerToysSettings::HotkeyObject::from_settings(false, true, true, false, VK_OEM_3),
                .excludedApps = L"app",
                .excludedAppsArray = { L"APP" },
            };

            PowerToysSettings::PowerToyValues values(m_moduleName);
            values.add_property(L"fancyzones_shiftDrag", expected.shiftDrag);
            values.add_property(L"fancyzones_displayChange_moveWindows", expected.displayChange_moveWindows);
            values.add_property(L"fancyzones_virtualDesktopChange_moveWindows", expected.virtualDesktopChange_moveWindows);
            values.add_property(L"fancyzones_zoneSetChange_flashZones", expected.zoneSetChange_flashZones);
            values.add_property(L"fancyzones_zoneSetChange_moveWindows", expected.zoneSetChange_moveWindows);
            values.add_property(L"fancyzones_overrideSnapHotkeys", expected.overrideSnapHotkeys);
            values.add_property(L"fancyzones_appLastZone_moveWindows", expected.appLastZone_moveWindows);
            values.add_property(L"use_cursorpos_editor_startupscreen", expected.use_cursorpos_editor_startupscreen);
            values.add_property(L"fancyzones_zoneHighlightColor", expected.zoneHightlightColor);
            values.add_property(L"fancyzones_editor_hotkey", expected.editorHotkey.get_json());
            values.add_property(L"fancyzones_excluded_apps", expected.excludedApps);

            values.save_to_settings_file();

            auto actual = MakeFancyZonesSettings(m_hInst, m_moduleName);
            Assert::IsTrue(actual != nullptr);

            auto actualSettings = actual->GetSettings();
            compareSettings(expected, actualSettings);
        }

        TEST_METHOD(CreateHotkeyMissed)
        {
            //prepare data
            const Settings expected = Settings{
                .shiftDrag = false,
                .displayChange_moveWindows = true,
                .virtualDesktopChange_moveWindows = true,
                .zoneSetChange_flashZones = true,
                .zoneSetChange_moveWindows = true,
                .overrideSnapHotkeys = false,
                .appLastZone_moveWindows = false,
                .use_cursorpos_editor_startupscreen = true,
                .zoneHightlightColor = L"#00FFD7",
                .zoneHighlightOpacity = 45,
                .editorHotkey = m_defaultSettings.editorHotkey,
                .excludedApps = L"app",
                .excludedAppsArray = { L"APP" },
            };

            PowerToysSettings::PowerToyValues values(m_moduleName);
            values.add_property(L"fancyzones_shiftDrag", expected.shiftDrag);
            values.add_property(L"fancyzones_displayChange_moveWindows", expected.displayChange_moveWindows);
            values.add_property(L"fancyzones_virtualDesktopChange_moveWindows", expected.virtualDesktopChange_moveWindows);
            values.add_property(L"fancyzones_zoneSetChange_flashZones", expected.zoneSetChange_flashZones);
            values.add_property(L"fancyzones_zoneSetChange_moveWindows", expected.zoneSetChange_moveWindows);
            values.add_property(L"fancyzones_overrideSnapHotkeys", expected.overrideSnapHotkeys);
            values.add_property(L"fancyzones_appLastZone_moveWindows", expected.appLastZone_moveWindows);
            values.add_property(L"use_cursorpos_editor_startupscreen", expected.use_cursorpos_editor_startupscreen);
            values.add_property(L"fancyzones_zoneHighlightColor", expected.zoneHightlightColor);
            values.add_property(L"fancyzones_highlight_opacity", expected.zoneHighlightOpacity);
            values.add_property(L"fancyzones_excluded_apps", expected.excludedApps);

            values.save_to_settings_file();

            auto actual = MakeFancyZonesSettings(m_hInst, m_moduleName);
            Assert::IsTrue(actual != nullptr);

            auto actualSettings = actual->GetSettings();
            compareSettings(expected, actualSettings);
        }

        TEST_METHOD(CreateAppsMissed)
        {
            //prepare data
            const Settings expected = Settings{
                .shiftDrag = false,
                .displayChange_moveWindows = true,
                .virtualDesktopChange_moveWindows = true,
                .zoneSetChange_flashZones = true,
                .zoneSetChange_moveWindows = true,
                .overrideSnapHotkeys = false,
                .appLastZone_moveWindows = false,
                .use_cursorpos_editor_startupscreen = true,
                .zoneHightlightColor = L"#00FFD7",
                .zoneHighlightOpacity = 45,
                .editorHotkey = PowerToysSettings::HotkeyObject::from_settings(false, true, true, false, VK_OEM_3),
                .excludedApps = m_defaultSettings.excludedApps,
                .excludedAppsArray = m_defaultSettings.excludedAppsArray,
            };

            PowerToysSettings::PowerToyValues values(m_moduleName);
            values.add_property(L"fancyzones_shiftDrag", expected.shiftDrag);
            values.add_property(L"fancyzones_displayChange_moveWindows", expected.displayChange_moveWindows);
            values.add_property(L"fancyzones_virtualDesktopChange_moveWindows", expected.virtualDesktopChange_moveWindows);
            values.add_property(L"fancyzones_zoneSetChange_flashZones", expected.zoneSetChange_flashZones);
            values.add_property(L"fancyzones_zoneSetChange_moveWindows", expected.zoneSetChange_moveWindows);
            values.add_property(L"fancyzones_overrideSnapHotkeys", expected.overrideSnapHotkeys);
            values.add_property(L"fancyzones_appLastZone_moveWindows", expected.appLastZone_moveWindows);
            values.add_property(L"use_cursorpos_editor_startupscreen", expected.use_cursorpos_editor_startupscreen);
            values.add_property(L"fancyzones_zoneHighlightColor", expected.zoneHightlightColor);
            values.add_property(L"fancyzones_highlight_opacity", expected.zoneHighlightOpacity);
            values.add_property(L"fancyzones_editor_hotkey", expected.editorHotkey.get_json());

            values.save_to_settings_file();

            auto actual = MakeFancyZonesSettings(m_hInst, m_moduleName);
            Assert::IsTrue(actual != nullptr);

            auto actualSettings = actual->GetSettings();
            compareSettings(expected, actualSettings);
        }

        TEST_METHOD(CreateWithEmptyJson)
        {
            json::to_file(m_tmpName, json::JsonObject());
            auto actual = MakeFancyZonesSettings(m_hInst, m_moduleName);
            Assert::IsTrue(actual != nullptr);

            auto actualSettings = actual->GetSettings();
            compareSettings(m_defaultSettings, actualSettings);
        }
    };
}