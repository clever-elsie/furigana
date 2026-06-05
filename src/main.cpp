#include <algorithm>
#include <ranges>
#include <utility>

#include <vector>
#include <span>
#include <optional>

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
using std::print;
using std::println;
using fsrditr = fs::recursive_directory_iterator;
using std::views::filter;
using std::views::transform;
using std::views::enumerate;

vector<fs::path> sorted_dirlist(const config_t&config, const fs::path&dir);
bool commit(const config_t&config, result_t&r);

int main(int argc, char**argv){
  const config_t config = proc_args(argc, argv);
  println("{}", config);
  using std::span;
  using std::optional;

  vector<optional<result_t>> batch_holder(config->batch);
  for(const auto&dir:config->dirlist){
    const vector<fs::path> paths = sorted_dirlist(config, dir);
    const size_t total_batch_count = (paths.size()+config->batch-1)/config->batch;
    size_t batch_count = 0;
    for(size_t top = 0; top<paths.size(); top+=config->batch, ++batch_count){
      batch_holder.assign(config->batch, std::nullopt);
      const size_t n = std::min(top+config->batch, paths.size())-top;
      span<const fs::path> dirlist(&paths[top], n);
      span<optional<result_t>> batch(batch_holder.data(), n);
      if(config->batch > 1)
          println("batch {:7}/{}", batch_count+1, total_batch_count);
      for(const auto&[i,p]:dirlist|enumerate){
        print("\tgenerate: ");
        if(config->batch>1)
            print("batch {:4}/{}: ", i+1, config->batch);
        println("total {:7}/{}: {}", top+i+1, paths.size(), p);
        batch[i] = [&]{
          // 既にルビがある場合は検証のみ
          if(utf8::has_ruby(p))
            return result_t{p, {}, result_t::success};
          return generate_api_call(config, p);
        }();
      }
      for(auto&&[i, r]:batch|enumerate){
        if(!r.has_value()) continue; // こっちはない
        print("\tcheck: ");
        if(config->batch>1)
            print("batch {:4}/{}: ", i+1, config->batch);
        println("total {:7}/{}: {}", top+i+1, paths.size(), r.value().previous);
        r = check_api_call(config, std::move(r.value()));
      }
      for(auto&&[i, r]:batch|enumerate){
        if(!r.has_value()) continue; // こっちはない
        print("\tcommit: ");
        if(config->batch>1)
            print("batch {:4}/{}: ", i+1, config->batch);
        println("total {:7}/{}: {}", top+i+1, paths.size(), r.value().previous);
        if(!commit(config, r.value())) continue;
      }
    }
  }
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
