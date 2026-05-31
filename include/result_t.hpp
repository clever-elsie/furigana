#pragma once
#include <vector>
#include <filesystem>
#include <optional>
#include <meta>

struct result_t{
  using path = std::filesystem::path;
  enum error_kind: unsigned char{
    success,
    undecidable, // 複数候補で決定不能
    to_json,     // リクエストのjson化失敗
    api,         // APIエラーなど
    filesystem,  // ファイル名が長すぎるなど
    from_json,   // LLMのjsonが形式不良
    invalid,     // LLMの出力形式が不適格
    no_alternative, // 曖昧などで一意に決まらない
    other,       // その他
  };
  path previous;
  std::optional<path> current;
  error_kind error;
  
  constexpr static std::string_view why(error_kind k){
    template for(constexpr auto v:std::define_static_array(std::meta::enumerators_of(^^error_kind))){
      if(k == [:v:])
        return std::meta::identifier_of(v);
    }
    return "<unknown>";
  }
};
