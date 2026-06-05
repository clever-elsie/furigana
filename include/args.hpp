#pragma once
#include <type_traits>
#include <meta>

#include <utility>
#include <vector>
#include <string>

#include <chrono>
#include <filesystem>

using namespace std::literals::chrono_literals;

constexpr inline size_t selectable_options = 11;

struct mutable_config_t{
  bool yes;
  bool check_already_has_ruby;
  bool https;
  std::string ip;
  size_t port;
  size_t batch;
  size_t num_ctx;
  std::chrono::seconds timeout;
  std::string generate_model, check_model;
  std::string wordset;
  std::vector<std::filesystem::path> dirlist;

  mutable_config_t()=delete;
  mutable_config_t(const mutable_config_t&)=default;
  mutable_config_t& operator=(const mutable_config_t&)=default;
  mutable_config_t(mutable_config_t&&)=default;
  mutable_config_t& operator=(mutable_config_t&&)=default;
  template<std::integral Rep, class Period,
    class IP_t,
    class GM_t, class CM_t,
    class WDS_t,
    class DL_t>
  mutable_config_t(bool y, bool cahr,
    bool https, IP_t&&ip, size_t port,
    size_t batch, size_t nc,
    std::chrono::duration<Rep, Period> to,
    GM_t&&gm, CM_t&&cm,
    WDS_t&&wds,
    DL_t&&dl)
  : yes(y), check_already_has_ruby(cahr),
    https(https), ip(std::forward<IP_t>(ip)), port(port),
    batch(batch), num_ctx(nc),
    timeout(to),
    generate_model(std::forward<GM_t>(gm)),
    check_model(std::forward<CM_t>(cm)),
    wordset(std::forward<WDS_t>(wds)),
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

const extern mutable_config_t default_config;

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
    wordset:                {},
    dirlist:                {}
}})",
      config.yes,
      config.check_already_has_ruby,
      config.https,
      config.ip,
      config.port,
      config.batch,
      config.num_ctx,
      config.timeout,
      config.generate_model,
      config.check_model,
      config.wordset,
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
