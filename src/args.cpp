#include <cstddef>
#include <cstdlib>

#include <vector>
#include <string>
#include <string_view>

#include <format>
#include <print>
#include <filesystem>

#include <args.hpp>
#include <handler.hpp>

namespace fs=std::filesystem;
using std::vector;
using std::println;
using std::print;

config_t proc_args(int argc, char**argv){
  auto args = [&]() -> vector<std::string_view> {
    vector<std::string_view> ret;
    for(int i=1;i<argc;++i)
      ret.push_back(argv[i]);
    return ret;
  }();

  mutable_config_t config(default_config);
  bool only_path=false;
  for(size_t i=0;i<args.size();++i){
    if(!only_path){
      if(args[i] == "--"){
        only_path = true;
        continue;
      }
      bool done = false;
      for(const auto&h:option_handler){
        bool todo = (h.can_exact_match && h.equal_or(args[i]))
                  ||(h.can_starts_with && h.starts_with_or(args[i]));
        if(todo){
          h.fn(config, i, args);
          done = true;
          break;
        }
      }
      if(done) continue;
    }
    const fs::path path(args[i]);
    println("Validation check : {}", path);
    try{
      const bool e = fs::exists(path);
      println("\t{}", e?"Exists":"Doesn't exist");
      if(!e){
        println("\tSkip this path {}", path);
        continue;
      }
      const bool d = fs::is_directory(path);
      println("\t{}Directory", d?"":"Not ");
      if(!d){
        println("\tSkip this path {}", path);
        continue;
      }
    }catch(const fs::filesystem_error& err){
      println("Error was happen: {}", err.what());
      println("Skip this path");
      continue;
    }
    config.dirlist.push_back(std::move(path));
  }
  if(config.dirlist.empty()){
    println("No valid scan path.");
    println("Nothing todo.");
    println("Finish this program.");
    std::exit(1);
  }
  return config;
}