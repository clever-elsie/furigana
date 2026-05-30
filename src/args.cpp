#include <args.hpp>
#include <cstddef>
#include <string_view>
#include <iostream>
#include <format>
#include <print>
#include <utility>
#include <concepts>
#include <type_traits>

namespace fs=std::filesystem;
using std::vector;
using std::println;

auto get_directories_list(int argc, char**argv)->vector<fs::path>{
  auto equal_or = []<class STR, class T, class... Args>(this auto self, const STR&s, const T& head,Args&&...args)
    requires std::same_as<std::decay_t<STR>, std::string> || std::same_as<std::decay_t<STR>, std::string_view>
  {
    if constexpr(sizeof...(Args) == 0)
      return s == head;
    else
      return (s==head) || self(s, std::forward<Args>(args)...);
  };

  char const*generate_model="gemma4:e4b";
  char const*check_model="gemma4:e4b";
  
  struct defer{
    defer(const char*&gm, const char*&cm):genmod(gm), chkmod(cm){}
    ~defer(){
      config_t::storage().generate_model = genmod;
      config_t::storage().check_model = chkmod;
    }
    private:
      const char*&genmod;
      const char*&chkmod;
  } def(generate_model, check_model);
  
  if(argc<=1){
    println("No Argument scan path");
    std::exit(1);
  }
  bool only_path = false;
  vector<fs::path> list;
  list.reserve(argc-1);
  for(int i=1;i<argc;++i){
    if(!only_path){
      std::string_view arg(argv[i]);
      if(arg == "--"){
         only_path=true;
         continue;
      }
      auto check_scheme = [&i,&arg, &argc, &argv](const char*&model){
        if(++i<argc)
          model = argv[i];
        else{
          println("No model name {} option.", arg);
          std::exit(1);
        }
      };
      if(equal_or(arg, "-g", "--generate", "--generate-model")){
        check_scheme(generate_model);
        println("generate model set {}", argv[i]);
        continue;
      }
      if(equal_or(arg, "-c", "--check", "--check-model")){
        check_scheme(check_model);
        println("check model set {}", argv[i]);
        continue;
      }
    }
    fs::path path(argv[i]);
    println("Validation check: {}", argv[i]);
    if(!fs::exists(path)){
      println("Not exist: {}", path);
      continue;
    }
    println("Exists: {}", argv[i]);
    if(!fs::is_directory(path)){
      println("Isn't directory: {}", path);
      continue;
    }
    println("is directory: {}", argv[i]);
    list.push_back(std::move(path));
  }
  if(list.empty()){
    println("No valid argument.");
    std::exit(1);
  }
  return list;
}