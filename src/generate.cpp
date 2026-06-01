#include <print>

#include <json.hpp>

#include <utf8.hpp>
#include <args.hpp>
#include <generate.hpp>
#include <api.hpp>

using std::println;
namespace fs = std::filesystem;

std::string call_generate(const config_t&config, const json::value&req);

result_t generate_api_call(const config_t&config, const fs::path&dirpath){
  json::value req(json::value::Object_t{});
  const std::string dirname=dirpath.filename();
  req["model"] = config->generate_model;
  req["prompt"] = std::format(
R"(ディレクトリ名「{}」の漢字部分を含めた全体の読み仮名をひらがなで出力してください．
ディレクトリ名に記号(、。♡☆など)が含まれる場合は削除してください．
ただし長音記号(ー)は自然な発音の母音として読んでください．
英数字は元の文字をそのまま出力してください．
既にルビが振られている場合はreadingは空文字列にしてください．
漢数字，ローマ数字(I,V,X)や①のような丸付きの数字はASCIIの数字0-9に変換してください．
出力は必ず以下のJSONフォーマットのみとし，余計な説明は一切含めないでください．
{{
    "reading": "よみがな"
}})"
    ,dirname
  );
  req["stream"] = false;
  req["format"] = std::string_view("json");
  req["think"]  = config->think;
  req["options"] = json::object_t{
    {"num_ctx",config->num_ctx},
  };
  req["temperature"] = 0.2; // 生成タスクは少しランダム性を追加

  std::string res;
  try{ res = call_generate(config, req); }
  catch(result_t::error_kind kind){ return {dirpath, {}, kind}; }
  
  println("Raw response: {}", res);

  if(!utf8::suitable_ruby(res.c_str(),res.c_str()+res.size()))
    return {dirpath, {}, result_t::invalid};
  return {dirpath, res, result_t::success};
}

std::string call_generate(const config_t&config, const json::value&req){
  json::object_t res_json=call_curl(config, req).object();
  try{
    auto res_body = std::move(res_json.at("response").str());
    if(res_body.empty())
      throw result_t::from_json;
    auto obj = json::value::load(res_body).object();
    return obj.at("reading").str();
  }catch(result_t::error_kind e){
    throw e;
  }catch(...){
    println("JSON pase error: call_generate");
    bool has_response = res_json.contains("response");
    println("\t: {} response",  has_response ? "has" : "doesn't have");
    if(!has_response){
      try{ println("{}", json::to_readable(res_json)); }
      catch(...){ throw result_t::from_json; }
    }
    throw result_t::from_json;
  }
}