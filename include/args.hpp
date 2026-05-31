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

using namespace std::literals::chrono_literals;

constexpr inline size_t selectable_options = 5;

struct mutable_config_t{
  bool yes;
  size_t max_token;
  std::chrono::seconds timeout;
  std::string generate_model, check_model;
  std::vector<std::filesystem::path> dirlist;

  mutable_config_t()=delete;
  mutable_config_t(const mutable_config_t&)=default;
  mutable_config_t& operator=(const mutable_config_t&)=default;
  mutable_config_t(mutable_config_t&&)=default;
  mutable_config_t& operator=(mutable_config_t&&)=default;
  template<std::integral Rep, class Period, class GM_t, class CM_t, class DL_t>
  mutable_config_t(bool y, size_t mt, std::chrono::duration<Rep, Period> to, GM_t&&gm, CM_t&&cm, DL_t&&dl)
  : yes(y), max_token(mt), timeout(to),
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
  false,
  static_cast<size_t>(1UL << 16),
  5min,
  "gemma4:e4b",
  "gemma4:e4b",
  std::vector<std::filesystem::path>()
);

config_t proc_args(int argc, char**argv);
