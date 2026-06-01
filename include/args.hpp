#pragma once
#include <type_traits>
#include <meta>

#include <utility>
#include <vector>
#include <string>
#include <string_view>
#include <optional>

#include <chrono>
#include <filesystem>

#include <json.hpp>

using namespace std::literals::chrono_literals;

constexpr inline size_t selectable_options = 10;

struct mutable_config_t{
  bool yes;
  bool check_already_has_ruby;
  bool https;
  std::string ip;
  size_t port;
  size_t batch;
  size_t num_ctx;
  json::value think;
  std::chrono::seconds timeout;
  std::string generate_model, check_model;
  std::vector<std::filesystem::path> dirlist;

  mutable_config_t()=delete;
  mutable_config_t(const mutable_config_t&)=default;
  mutable_config_t& operator=(const mutable_config_t&)=default;
  mutable_config_t(mutable_config_t&&)=default;
  mutable_config_t& operator=(mutable_config_t&&)=default;
  template<class THINK_t,
    std::integral Rep, class Period,
    class IP_t,
    class GM_t, class CM_t,
    class DL_t>
  mutable_config_t(bool y, bool cahr,
    bool https, IP_t&&ip, size_t port,
    size_t batch, size_t nc, THINK_t&&th,
    std::chrono::duration<Rep, Period> to,
    GM_t&&gm, CM_t&&cm, DL_t&&dl)
  : yes(y), check_already_has_ruby(cahr),
    https(https), ip(std::forward<IP_t>(ip)), port(port),
    batch(batch), num_ctx(nc), think(std::forward<THINK_t>(th)),
    timeout(to),
    generate_model(std::forward<GM_t>(gm)),
    check_model(std::forward<CM_t>(cm)),
    dirlist(std::forward<DL_t>(dl))
  {}
};

class config_t{
  const mutable_config_t config;
  public:
  config_t()=delete;
  template<class T>
  requires std::same_as<std::decay_t<T>, mutable_config_t>
  config_t(T&&t): config(std::forward<T>(t)){}
  config_t(const config_t&)=default;
  config_t& operator=(const config_t&)=default;
  config_t(config_t&&)=default;
  config_t& operator=(config_t&&)=default;
  
  constexpr operator const mutable_config_t&() const noexcept{
    return config;
  }
  constexpr const mutable_config_t* operator->() const noexcept{
    return &config;
  }
};

const inline mutable_config_t default_config(
  false, // yes
  false, // check_already_has_ruby
  false, // https
  "localhost",  // ip
  11434,        // port
  1,            // batch
  65536,        // num_ctx
  true,         // think
  10min,        // timeout
  "gemma4:e4b", // generate_model
  "gemma4:e4b", // check_model
  std::vector<std::filesystem::path>() // dirlist
);

config_t proc_args(int argc, char**argv);

template<>
struct std::formatter<mutable_config_t>{
  constexpr auto parse(std::format_parse_context& ctx){
    return ctx.begin();
  }
  auto format(const mutable_config_t&config, std::format_context&ctx)const{
    return std::format_to(ctx.out(),
R"({{
    yes:                    {},
    check_already_has_ruby: {},
    https:                  {},
    ip:                     {},
    port:                   {},
    batch:                  {},
    num_ctx:                {},
    timeout:                {},
    generate_model:         {},
    check_model:            {},
    dirlist:                {}
}})",
      config.yes, config.check_already_has_ruby,
      config.https, config.ip, config.port,
      config.batch, config.num_ctx, config.timeout,
      config.generate_model, config.check_model,
      config.dirlist
    );
  }
};


template<>
struct std::formatter<config_t>{
  constexpr auto parse(std::format_parse_context& ctx){
    return ctx.begin();
  }
  auto format(const config_t&config, std::format_context&ctx)const{
    return std::format_to(ctx.out(), "{}", *reinterpret_cast<const mutable_config_t*>(&config));
  }
};