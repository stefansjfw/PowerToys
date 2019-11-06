#pragma once

#include <string>

class PowertoyModuleIface;

class PowertoySystemMenuIface
{
public:
    virtual void SetConfiguration(PowertoyModuleIface* module, const wchar_t* config) = 0;
};