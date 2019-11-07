#pragma once

#include <string>

class PowertoyModuleIface;

class PowertoySystemMenuIface
{
public:
    virtual void SetConfiguration(PowertoyModuleIface* module, const wchar_t* config) = 0;
    virtual void RegisterAction  (PowertoyModuleIface* module, HWND window, const wchar_t* name) = 0;
};