#include <cstdlib>
#include <utility>
#include <vector>
#include <string_view>
#include <initializer_list>
#include <unordered_map>

#include <ranges>
#include <algorithm>
#include <print>

#include <args.hpp>

using handler = void(*)(
  mutable_config_t&,
  size_t&,
  const std::vector<std::string_view>&
);
struct handler_set{
  constexpr static int max_option_arg_variant=3;
  handler fn;
  int valid_variants;
  std::array<std::string_view, max_option_arg_variant>
    vary;
  bool can_starts_with;
  bool can_exact_match;
  
  bool equal_or(const std::string_view& s)const{
    using namespace std::views;
    for(const auto&v:vary|take(valid_variants))
      if(s==v)return true;
    return false;
  }
  
  bool starts_with_or(const std::string_view&s)const{
    using namespace std::views;
    for(const auto&v:vary|take(valid_variants))
      if(s.starts_with(v)) return true;
    return false;
  }
};

// ハンドラーはハンドラーが最後に使った引数にiがある状態で終了する
void yes_handler(mutable_config_t&,size_t&,const std::vector<std::string_view>&);
void check_already_has_ruby_handler(mutable_config_t&,size_t&,const std::vector<std::string_view>&);
void https_handler(mutable_config_t&,size_t&,const std::vector<std::string_view>&);
void ip_handler(mutable_config_t&,size_t&,const std::vector<std::string_view>&);
void port_handler(mutable_config_t&,size_t&,const std::vector<std::string_view>&);
void batch_handler(mutable_config_t&,size_t&,const std::vector<std::string_view>&);
void num_ctx_handler(mutable_config_t&,size_t&,const std::vector<std::string_view>&);
void think_handler(mutable_config_t&,size_t&,const std::vector<std::string_view>&);
void timeout_handler(mutable_config_t&,size_t&,const std::vector<std::string_view>&);
void generate_model_handler(mutable_config_t&,size_t&,const std::vector<std::string_view>&);
void check_model_handler(mutable_config_t&,size_t&,const std::vector<std::string_view>&);
void help_handler(mutable_config_t&,size_t&,const std::vector<std::string_view>&);

const auto option_handler = []{
  std::array<handler_set, selectable_options+1> ret;
  size_t i = 0;
  ret.at(i++) = handler_set{
    yes_handler,
    2, {"-y", "--yes"},
    false, true
  };
  ret.at(i++) = handler_set{
    check_already_has_ruby_handler,
    2, {"-a", "--check-all"},
    false, true
  };
  ret.at(i++) = handler_set{
    https_handler,
    2, {"-s", "--https"},
    false, true
  };
  ret.at(i++) = handler_set{
    ip_handler,
    2, {"-i", "--ip"},
    true, true
  };
  ret.at(i++) = handler_set{
    port_handler,
    2, {"-p", "--port"},
    true, true
  };
  ret.at(i++) = handler_set{
    batch_handler,
    2, {"-b", "--batch"},
    true, true
  };
  ret.at(i++) = handler_set{
    num_ctx_handler,
    2, {"-n", "--num-ctx"},
    true, true
  };
  ret.at(i++) = handler_set{
    think_handler,
    2, {"-t", "--think"},
    true, true
  };
  ret.at(i++) = handler_set{
    timeout_handler,
    2, {"-T", "--timeout"},
    true, true
  };
  ret.at(i++) = handler_set{
    generate_model_handler,
    3, {"-g", "--generate", "--generate-model"},
    true, true
  };
  ret.at(i++) = handler_set{
    check_model_handler,
    3, {"-c", "--check", "--check-model"},
    true, true
  };
  ret.at(i++) = handler_set{
    help_handler,
    3, {"-v", "-h", "--help"},
    false, true
  };
  // コマンドラインオプションの任意のペアが衝突してはいけない
  std::unordered_map<std::string_view,size_t> set;
  for(size_t i=0;i<ret.size();++i)
    for(int j=0;j<ret[i].valid_variants;++j){
      const auto itr = set.find(ret[i].vary[j]);
      if(itr!=set.end()){
        std::println("command line option {} was collision. {} and {} option", ret[i].vary[j], itr->second, i);
        std::exit(1);
      }
      set.emplace(ret[i].vary[j], i);
    }
  return ret;
}();