#include "pch.h"
#include "json.h"

#include <fstream>

namespace
{
    // Convert an UTF8 string to a wide Unicode String
    std::wstring utf8_decode(const std::string& str)
    {
        if (str.empty())
            return std::wstring();
        int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
        std::wstring wstrTo(size_needed, 0);
        MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
        return wstrTo;
    }

    // Convert a wide Unicode string to an UTF8 string
    std::string utf8_encode(const std::wstring& wstr)
    {
        if (wstr.empty())
            return std::string();
        int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
        std::string strTo(size_needed, 0);
        WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
        return strTo;
    }
}

namespace json
{
    std::optional<JsonObject> from_file(std::wstring_view file_name)
    {
        try
        {
            std::ifstream file(file_name.data(), std::ios::binary);
            using isbi = std::istreambuf_iterator<char>;
            std::string objStr{ isbi{ file }, isbi{} };

            return JsonValue::Parse(utf8_decode(objStr)).GetObjectW();
        }
        catch (...)
        {
            return std::nullopt;
        }
    }

    void to_file(std::wstring_view file_name, const JsonObject& obj)
    {
        std::wstring objStr{ obj.Stringify().c_str() };
        std::ofstream{ file_name.data(), std::ios::binary } << utf8_encode(objStr);
    }
}
