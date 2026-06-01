#include <cstdint>
#include <cstdlib>
#include <cctype>
#include <utility>

#include <print>
#include <fstream>
#include <sstream>

#include <json.hpp>

#include <handler.hpp>
using std::println;
using std::print;

void yes_handler(mutable_config_t& config,[[maybe_unused]]size_t&,[[maybe_unused]]const std::vector<std::string_view>&){
  config.yes = true;
}

void check_already_has_ruby_handler(mutable_config_t& config,[[maybe_unused]]size_t&,[[maybe_unused]]const std::vector<std::string_view>&){
  config.check_already_has_ruby = true;
}

void https_handler(mutable_config_t&config,[[maybe_unused]]size_t&,[[maybe_unused]]const std::vector<std::string_view>&){
  config.https = true;
}

void help_handler([[maybe_unused]]mutable_config_t&,[[maybe_unused]]size_t&,[[maybe_unused]]const std::vector<std::string_view>&){
  println(R"(すべてのコマンドラインオプション
-v, -h, --help  : ヘルプ．指定するとこの画面を表示して終了
-y, --yes       : LLMの提案する読み仮名を自動的に採用 (推奨) デフォルトでオフ
-a, --check-all : 既に読み仮名のあるディレクトリの読み仮名も検証．デフォルトでオフ
-s, --https     : LLMサーバhttpsサーバなら必要． デフォルトでオフ
-i, --ip        : IPアドレスを指定．デフォルトはlocalhost
-p, --port      : ポート番号を指定．デフォルトは11434
-b, --batch     : バッチの単位数を指定．デフォルトは1
-n, --num-ctx   : LLMのコンテキストウィンドウ長．デフォルトは65536
-T, --timeout   : LLMの解答のタイムアウト時間．デフォルトは5分．指定は秒単位
-g, --generate, --generate-model :
    読み仮名生成に使うモデル．デフォルトはgemma4:e4b
-c, --check, --check-model :
    読み仮名の検証に使うモデル．デフォルトはgemma4:e4b
-w, --word, --wordset:
    辞書となるjsonファイル
)");
  std::exit(0);
}

namespace{
  // -t=5, -t 5などを適切に読み取る
  // 必要に応じてiをインクリメントする
  static std::string
  extract_next_or_after_equal(
    size_t&i,
    const std::vector<std::string_view>& args
  ){
    const size_t eq = args[i].find('=');
    if(eq==std::string::npos) // 2引数
      return std::string(args[++i]);
    // =より後ろ
    const char* begin = args[i].data() + eq+1;
    const char* end = args[i].data() + args[i].size();
    return std::string(begin, end);
  }
}

void ip_handler(mutable_config_t&config,size_t&i,const std::vector<std::string_view>&args){
  auto opt = extract_next_or_after_equal(i, args);
  config.ip = std::move(opt);
}

void port_handler(mutable_config_t&config,size_t&i,const std::vector<std::string_view>&args){
  const size_t p=i;
  auto opt = extract_next_or_after_equal(i, args);
  try{
    uint64_t n = std::stoul(opt);
    config.port = n;
  }catch(const std::exception&err){
    println("Error Invalid Argument : {}", err.what());
    if(p==i) println("Option: {}", args[p]);
    else println("Option: {} {}", args[p], args[i]);
    std::exit(1);
  }
}

void batch_handler(mutable_config_t&config,size_t&i,const std::vector<std::string_view>&args){
  const size_t p=i;
  auto opt = extract_next_or_after_equal(i, args);
  try{
    uint64_t n = std::stoul(opt);
    config.batch = n;
  }catch(const std::exception&err){
    println("Error Invalid Argument : {}", err.what());
    if(p==i) println("Option: {}", args[p]);
    else println("Option: {} {}", args[p], args[i]);
    std::exit(1);
  }
}

void num_ctx_handler(mutable_config_t& config,size_t&i,const std::vector<std::string_view>&args){
  const size_t p=i;
  auto opt = extract_next_or_after_equal(i, args);
  try{
    uint64_t n = std::stoul(opt);
    config.num_ctx = n;
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

void wordset_handler(mutable_config_t&config,size_t&i,const std::vector<std::string_view>&args){
  auto opt = extract_next_or_after_equal(i, args);
  if(!std::filesystem::exists(opt)){
    println("Invalid word set. This file does not exists. {}", opt);
    std::exit(1);
  }
  std::ifstream ifs(opt);
  if(!ifs){
    println("File open error: {}", opt);
    std::exit(1);
  }
  ifs.seekg(0,std::ios::end);
  std::streamsize size = ifs.tellg();
  ifs.seekg(0, std::ios::beg);
  std::string content(size, '\0');
  if(!ifs.read(content.data(), size)){
    println("File read error: {}", opt);
    std::exit(1);
  }
  json::value array;
  try{
    array=json::value::load(content);
  }catch(...){
    println("JSON parse error: at wordset_handler {}", opt);
    std::exit(1);
  }
  if(!array.is<json::value::Array_t>()){
    println("Invalid wordset {}. json file must contains array", opt);
    std::exit(1);
  }
  for(auto&obj:array.array()){
    if(!obj.is<json::value::Object_t>()){
      println("Invalid wordset {}. elements of array must be objects", opt);
      std::exit(1);
    }
    auto&object = obj.object();
    bool valid = true;
    if(!(valid&=object.contains("raw")))
      println("Invalid wordset {}. lack of key \"raw\"", opt);
    if(!(valid&=object.contains("out")))
      println("Invalid wordset {}. lack of key \"out\"", opt);
    if(!(valid&=(object.size()==2)))
      println("Invalid wordset {}. elements count expected is 2. But in reality is {}", opt, object.size());
    if(!valid) std::exit(1);
    config.wordset.append_range(json::to_string(object));
  }
}
