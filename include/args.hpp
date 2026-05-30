#pragma once
#include <vector>
#include <filesystem>

std::vector<std::filesystem::path>
get_directories_list(int argc, char**argv);

struct config_t{
  static const std::string_view& genmodel(){
    return storage().generate_model;
  }
  static const std::string_view& chkmodel(){
    return storage().check_model;
  }
  private:
  static config_t& storage(){
    static config_t obj;
    return obj;
  }
  std::string_view generate_model, check_model;
  friend std::vector<std::filesystem::path> get_directories_list(int,char**);
};