#include <cstdint>
#include <cstdlib>
#include <cctype>
#include <utility>

#include <print>
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
-t, --think     : thinkオプション．モデル依存．true,false,high,medium,low．デフォルトはtrue
-T, --timeout   : LLMの解答のタイムアウト時間．デフォルトは5分．指定は秒単位
-g, --generate, --generate-model :
    読み仮名生成に使うモデル．デフォルトはgemma4:e4b
-c, --check, --check-model :
    読み仮名の検証に使うモデル．デフォルトはgemma4:e4b
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

void think_handler(mutable_config_t&config,size_t&i,const std::vector<std::string_view>&args){
  auto opt = extract_next_or_after_equal(i, args);
  if(opt == "true"){
    config.think = true;
    return;
  }else if(opt=="false"){
    config.think = false;
    return;
  }
  for(const auto&level:{"high", "medium", "low"})
    if(opt == level){
      config.think = std::move(opt);
      return;
    }
  println("Unknown think level : {}", opt);
  std::exit(1);
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

