#include <commit.hpp>
#include <format>
#include <print>

using std::println;
namespace fs = std::filesystem;

void commit(const std::vector<result_t>&list){
  for(const auto&[prev,cur,err]:list){
    if(!cur.has_value()){
      println("Error : {} at {}", result_t::why(err), prev);
      continue;
    }
    const std::string dirname = prev.filename();
    const std::string ruby = cur.value().string();
    const fs::path next = prev.parent_path()/(dirname +"《"+ruby+"》");
    if(fs::exists(next)){
      println("Try rename {}", prev);
      println("Duplicate dirname: {}", next);
      println("Skip this rename");
      continue;
    }
    println("Rename: from {}", prev);
    println("Rename:   to {}", next);
    std::error_code ec;
    fs::rename(prev,next,ec);
    if(ec){
      println("Failed rename. from {} to {}", prev, next);
    }
  }
}