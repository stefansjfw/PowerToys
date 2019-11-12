#pragma once
#include <string>
#include <Shlobj.h>
#include <cpprest/json.h>

namespace PTSettingsHelper {

  std::wstring get_module_save_folder_location(const std::wstring& powertoy_name);

  void save_module_settings(const std::wstring& powertoy_name, web::json::value& settings);
  web::json::value load_module_settings(const std::wstring& powertoy_name);
  void save_general_settings(web::json::value& settings);
  web::json::value load_general_settings();

}
