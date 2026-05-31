#include <print>
#include <filesystem>

#include <json.hpp>

#include <check.hpp>
#include <utf8.hpp>
#include <api.hpp>

using std::println;
namespace fs = std::filesystem;

json::object_t call_check(const config_t& config, const json::object_t&req);

result_t check_api_call(const config_t&config, result_t&& tar){
  using std::string;
  using json::value;
  using json::object_t;
  auto discard = [](result_t& item, const std::string&msg){
    println("{}",msg);
    println("Therefore skip this item.");
    println("dir: {}\nruby: {}\nerr: {}\n",
      item.previous,
      item.current.value_or("[None]"),
      result_t::why(item.error)
    );
    item.current.reset(); // 真偽判定していないので破棄
    item.error = result_t::invalid;
  };
  object_t req{
    {string("previous directory name"), value(tar.previous.filename().string())},
    {string("generated directory name"), value(tar.current.value_or("").string())}
  };
  object_t res;
  try{ res = call_check(config, req); }
  catch(...){
    discard(tar, "Check LLM response was broken.");
    return tar;
  }
  if(!tar.current.has_value()) return tar;
  try{
    bool valid = res.at("valid").boolean();
    if(valid){
      println("Valid : {}\n\t{}", tar.previous, tar.current.value_or("[None]"));
    }else{
      std::string instead = res.at("corrected").str();
      println("Invalid : {}\n\tGenerated: {}\n\tInstead: {}", tar.previous, tar.current.value_or("[None]"), instead);
      if(instead.empty() && !utf8::suitable_ruby(instead.data(), instead.data() + instead.size())){
        tar.current = std::optional<fs::path>();
        tar.error=result_t::no_alternative;
        println("\terror message: {}", result_t::why(result_t::no_alternative));
      }else{
        tar.current = instead;
        tar.error = result_t::success;
      }
    }
  }catch(...){
    std::string json;
    try{ json = json::to_readable(res); }
    catch(...){}
    discard(tar, std::format(
      "JSON parse error: check_api_call.\n{}"
      , json
    ));
  }
  return tar;
}



json::value call_check_object_to_json(const config_t&config, const json::object_t&v){
  json::value req(json::value::Object_t{});
  req["model"] = config->check_model;
  req["prompt"] = std::format(
R"(以下の変換ルールに従って，元のディレクトリ名から生成された読み仮名が正しく作られているか厳密に検証してください．
変換ルール:
1. 漢字・ひらがな・カタカナは，全体の正しい読み仮名(ひらがな)に変換されていること．また数字として解釈すべき部分がすべてASCIIの数字に変換されていること．
2. 記号(_や-,♡など)は読み飛ばされ，消去されていること．ただし，長音(ー)は自然な母音になっていること．
3. 英単語は日本語にせず，元のアルファベットのまま残っていること．
4. 生成されたディレクトリ名が空文字列なら検証は不要です．
5. 既にルビがディレクトリ名に含まれている場合は空文字列を出力してください．
検証対象は以下の形式で与えられる．
{{
    "previous directory name": "元のディレクトリ名",
    "generated directory name": "せいせいされたでぃれくとりめい"
}}
結果は以下の形式で出力してください．
{{
    "thought": ここに思考プロセスを詳細に記述．
    "valid": true/false,
    "corrected": "もしvalidがfalseならあなたが正しい読み仮名をここに書いてください．ただし曖昧で決定できない場合はfalseのときも空文字列にしてください．"
}}
検証する対象は以下です．
{})"
    , json::to_string(v)
  );
  req["stream"] = false;
  req["format"] = std::string_view("json");
  req["num_predict"] = config->max_token;
  req["temperature"] = 0.01; // 検証タスクでは温度は小さく
  return req;
}

json::object_t call_check(const config_t&config, const json::object_t&req){
  json::object_t res_json = call_curl(config, call_check_object_to_json(config, req)).object();
  json::value obj;
  std::string body;
  try{
    body = std::move(res_json.at("response").str());
  }catch(...){
    println("JSON pase error: call_check");
    throw result_t::from_json;
  }
  try{
    return (obj = json::value::load(body)).object();
  }catch(...){
    println("response parse error: call_check\n\t:{}",body);
    throw result_t::from_json;
  }
}