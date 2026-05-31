#include <cstdint>
#include <cstdlib>
#include <cctype>
#include <utility>

#include <print>
#include <handler.hpp>
using std::println;
using std::print;

void yes_handler(mutable_config_t& config,size_t& i,const std::vector<std::string_view>& args){
  config.yes = true;
}

namespace{
  // -t=5, -t 5, -g="gemma4:e4b"などを適切に読み取る
  // 文字列に""があれば削除
  // 必要に応じてiをインクリメントする
  // 無効なら即座にexit
  static std::string
  extract_next_or_after_equal(
    size_t&i,
    const std::vector<std::string_view>& args
  ){
    const size_t eq = args[i].find('=');
    if(eq==std::string::npos) // 2引数
      return std::string(args[++i]);
    else{ // =より後ろ
      size_t begin = eq+1;
      size_t end = args[i].size();
      if(args[i][begin] == '"'){
        ++begin;
        if(args[i][end-1]=='"')
          --end;
        else{
          println("Error Invalid double quotation: {}", args[i]);
          std::exit(1);
        }
        return std::string(args[i].data() + begin, args[i].data() + end);
      }
    }
  }
}

void max_token_handler(mutable_config_t& config,size_t&i,const std::vector<std::string_view>&args){
  const size_t p=i;
  auto opt = extract_next_or_after_equal(i, args);
  try{
    uint64_t n = std::stoul(opt);
    config.max_token = n;
  }catch(const std::exception&err){
    println("Error Invalid Argument : {}", err.what());
    if(p==i) println("Option: {}", args[p]);
    else println("Option: {} {}", args[p], args[i]);
    std::exit(1);
  }
}

void timeout_handler(mutable_config_t&config,size_t&i,const std::vector<std::string_view>&args){
  const size_t p=i;
  auto opt = extract_next_or_after_equal(i, args);
  try{
    uint64_t n = std::stoul(opt);
    config.timeout = std::chrono::seconds{n};
  }catch(const std::exception&err){
    println("Error Invalid Argument : {}", err.what());
    if(p==i) println("Option: {}", args[p]);
    else println("Option: {} {}", args[p], args[i]);
    std::exit(1);
  }
}

void generate_model_handler(mutable_config_t&config,size_t&i,const std::vector<std::string_view>&args){
  auto opt = extract_next_or_after_equal(i, args);
  config.generate_model = std::move(opt);
}

void check_model_handler(mutable_config_t&config,size_t&i,const std::vector<std::string_view>&args){
  auto opt = extract_next_or_after_equal(i, args);
  config.check_model = std::move(opt);
}

