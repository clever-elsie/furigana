#include <algorithm>
#include <ranges>
#include <utility>

#include <vector>

#include <filesystem>
#include <print>
#include <iostream>
#include <format>

#include <args.hpp>
#include <generate.hpp>
#include <check.hpp>
#include <utf8.hpp>

namespace fs = std::filesystem;
using std::vector;
using std::println;
using fsrditr = fs::recursive_directory_iterator;
using std::views::filter;
using std::views::transform;

void proc_top_level(const config_t&config, const fs::path&dir);

int main(int argc, char**argv){
  const config_t conf = proc_args(argc, argv);
  println("{}", conf);
  for(const auto&dir:conf->dirlist)
    proc_top_level(conf, dir);
}

vector<fs::path> sorted_dirlist(const config_t&config, const fs::path&dir){
  // all_dirsでディレクトリを収集
  // 不適なルビをrenameするのはrecursive_directory_iterator中では危険
  vector<fs::path> all_dirs(fsrditr(dir)
    |filter([](const auto&i){return i.is_directory();})
    |transform([](const auto&i){ return i.path();})
    |std::ranges::to<std::vector>()
  );
  auto remove_invalid_ruby = [](fs::path&path)->bool {
    if(utf8::has_ruby(path) && !utf8::has_suitable_ruby(path)){
      println("Invalid ruby directory {}", path);
      std::string without_ruby = utf8::without_ruby(path.filename().string());
      fs::path new_path = path.parent_path() / without_ruby;
      try{
        fs::rename(path, new_path);
      }catch(const fs::filesystem_error&err){
        println("Error rename: {}", err.what());
        return false;
      }
      path = new_path;
      return true;
    }
    return true;
  };
  vector<fs::path> paths;
  for(auto&path:all_dirs){
    // ルビが不適切な場合は一度消去する
    if(!remove_invalid_ruby(path)) continue;
    // 既にルビが存在する場合，check_already_has_rubyでなければスキップ
    if(!config->check_already_has_ruby)
      if(utf8::has_ruby(path))
        continue;
    if(utf8::no_need_ruby(path))
      continue; // ルビ不要ならいつでもスキップ
    paths.push_back(std::move(path));
  }
  // ソートして進捗を見やすく
  std::sort(paths.begin(), paths.end());
  return paths;
}

bool commit(const config_t&config, result_t&r){
  const auto&[prev, kana, err] = r;
  if(!kana.has_value()){
    println("Error : {} at {}", result_t::why(err), prev);
    return false;
  }
  const std::string dirname = prev.filename();
  const std::string ruby = kana.value().string();
  const fs::path next =[&]{
    // 既にルビが存在する場合
    if(config->check_already_has_ruby && utf8::has_ruby(prev)){
      std::string without_ruby = utf8::without_ruby(prev.filename().string());
      return prev.parent_path()/(without_ruby + "《"+ruby+"》");
    }
    return prev.parent_path()/(dirname+"《"+ruby+"》");
  }();
  try{
    if(fs::exists(next)){
      println("Try rename {}", prev);
      println("Duplicate dirname: {}", next);
      println("Skip this rename");
      return false;
    }
  }catch(const fs::filesystem_error& err){
    println("Error check existence {}", next);
    return false;
  }
  println("Rename: from {}", prev);
  println("Rename:   to {}", next);
  bool accepted = config->yes;
  if(!accepted){
    println("Rename [Y/n]");
    std::string line;
    std::cin>>line;
    std::ranges::transform(line, line.begin(), [](char x){ return std::tolower(x); });
    for(const auto&tar:{"y", "yes"})
      if(line == tar){
        accepted = true;
        break;
      }
  }
  if(!accepted){
    println("Rename was rejected. Skip {}", prev);
    return false;
  }
  try{
    fs::rename(prev,next);
  }catch(const std::filesystem::filesystem_error &err){
    println("Failed rename: {} from {} to {}", err.what(), prev, next);
    return false;
  }
  return true;
}

void proc_top_level(const config_t&config, const fs::path&dir){
  const vector<fs::path> paths = sorted_dirlist(config, dir);
  for(const auto&[i,p]:paths|std::views::enumerate){
    println("{:7}/{}: {}", i,paths.size(),p);
    // 生成
    result_t r = [&]{
      // 既にルビがある場合は検証のみ
      if(utf8::has_ruby(p))
        return result_t{p, {}, result_t::success};
      return generate_api_call(config, p);
    }();
    // 検証
    r = check_api_call(config, std::move(r));
    // コミット
    if(!commit(config, r)) continue;
  }
}