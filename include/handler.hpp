#include <utility>
#include <vector>
#include <string_view>
#include <initializer_list>

#include <ranges>
#include <algorithm>

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
void max_token_handler(mutable_config_t&,size_t&,const std::vector<std::string_view>&);
void timeout_handler(mutable_config_t&,size_t&,const std::vector<std::string_view>&);
void generate_model_handler(mutable_config_t&,size_t&,const std::vector<std::string_view>&);
void check_model_handler(mutable_config_t&,size_t&,const std::vector<std::string_view>&);

const auto option_handler = []{
  std::array<handler_set, selectable_options> ret;
  ret.at(0) = handler_set{
    yes_handler,
    2, {"-y", "--yes"},
    false, true
  };
  ret.at(1) = handler_set{
    max_token_handler,
    2, {"-m", "--max-token"},
    true, true
  };
  ret.at(2) = handler_set{
    timeout_handler,
    2, {"-t", "--timeout"},
    true, true
  };
  ret.at(3) = handler_set{
    generate_model_handler,
    3, {"-g", "--generate", "--generate-model"},
    true, true
  };
  ret.at(4) = handler_set{
    check_model_handler,
    3, {"-c", "--check", "--check-model"},
    true, true
  };
  return ret;
}();