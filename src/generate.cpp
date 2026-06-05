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
R"("request": {{
  target_directory_nameのディレクトリ名の漢字部分を含めた全体の読み仮名をひらがなで出力してください．
  1. ディレクトリ名に記号(、。♡☆など)が含まれる場合は削除してください．
  2. ただし長音記号(ー)は自然な発音の母音として読んでください．
  3. また，「々」は前の漢字の読み方を踏襲してください．
  4. 英数字は元の文字をそのまま出力してください．
  5. 既にルビが振られている場合はreadingは空文字列にしてください．
  6. 漢数字，ローマ数字(I,V,X)や①のような丸付きの数字はASCIIの数字0-9に変換してください．
  7. 伏字として記号が用いられている場合は，伏字を補った尤もらしい単語とみなして読み仮名を生成してください．伏字を補うためにできるだけ多くの単語と比較してください．
  8. 読み仮名の考察において，漢字から読み仮名，読み仮名から漢字の変換で双方十分に尤もらしいことを確かめてください．
  9. 読み仮名を推察する段階において，できるだけ多くの候補を試すこと．
  10. 十分に議論が尽くされるまで何度もthinkingとcriticismを交互に繰り返すこと．
  11. wordsetの単語の対応表は最優先でチェックすること．
  出力は必ず以下のJSONフォーマットのみとし，余計な説明は一切含めないでください．
  {{
      "thinking_1": ここに読み仮名の詳細な考察を書く．
      "criticism_1": ここに推論に対する批判を書く．
      "conclusion": ここに結論と根拠を書く．
      "turn": thinkingとcriticismをセットで1とカウントして，結論までに必要だったターン数を整数で書く．
      "reading": "よみがな"
  }}
  thinkingでは漢字のまとまりに注意し，意味としての妥当性を満たす読みを考えてください．
  criticismではルールに厳密に合うか，また音素の抜け漏れが無いか検証してください．
  conclutionではthinkingとconclusionの結果をもとに，尤もらしい読み仮名を生成してください．
  readingは結論として得られた読み仮名のみを出力してください．
}},
"examples":[
  {{
    "input": "スーパーカー",
    "answer": "すうぱあかあ"
  }},
  {{
    "input": "甘々",
    "answer": "あまあま"
  }},
  {{
    "input": "question IV",
    "answer": "question 4"
  }},
  {{
    "input": "百人力",
    "answer": "100にんりき"
  }},
  {{
    "input": "スター☆ライト",
    "answer": "すたーらいと"
  }},
],
"target_directory_name": {},
"wordset": {}
)"
    ,dirname, config->wordset
  );
  req["stream"] = false;
  req["format"] = std::string_view("json");
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
