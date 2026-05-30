#include <cstdlib>
#include <regex>
#include <format>
#include <utility>
#include <string_view>
#include <print>
#include <span>

#include <recurse.hpp>
#include <args.hpp>
#include <utf8.hpp>
#include <json.hpp>
#include <curl/curl.h>

namespace fs = std::filesystem;
using std::vector;
using std::println;

bool suitable_ruby(const char*begin, const char*end);
bool has_ruby(const fs::path&dirpath);
result_t generate_api_call(const fs::path&dirpath);
vector<result_t> check_api_call(vector<result_t>&& list);
json::value call_curl(const json::value&req);
std::string call_generate(const json::value&req);
json::object_t call_check(const json::object_t&req);

vector<result_t> recurse(const fs::path&dirpath){
  println("Start recurse : {}", dirpath);
  vector<result_t> unchecked;
  for(const auto&itr:fs::recursive_directory_iterator(dirpath)){
    if(!itr.is_directory()) continue;
    const fs::path path(itr.path());
    if(has_ruby(path)) continue;
    println("Add checklist : {}", path);
    unchecked.push_back(generate_api_call(path));
  }
  return check_api_call(std::move(unchecked));
}

bool suitable_ruby(const char*begin, const char*end){
  for(;begin<end;){
    uint32_t code = utf8::decode_one(begin, end);
    if(code == 0) return false;
    if(utf8::isdigit(code) || utf8::isalpha(code) || utf8::ishiragana(code) || utf8::iskatakana(code));
    else
      return false;
  }
  return true;
}

bool has_ruby(const fs::path&dirpath){
  // 《がある場合，.*《[^《]*》$を満たさなければならない
  // つまり最後に出てくる《以降に》が無ければならず，またそれが最後の文字でなければならない
  // また最後の《》に挟まれている文字列は英数字，ひらがな，カタカナのいずれかでなければならない．
  const std::regex r("《[^《》]+》$");
  std::cmatch m;
  const char* begin = dirpath.c_str();
  const char* end = begin + dirpath.string().size();
  if(!std::regex_search(begin, end, m, r))
    return false;
  std::sub_match w = m[0];
  auto itr = w.first+3, etr = w.second -3;
  return suitable_ruby(itr,etr);
}

// libcurlのコールバック
size_t WriteCallback(void*contents, size_t size, size_t nmemb, std::string*userp){
  userp->append((char*)contents, size*nmemb);
  return size*nmemb;
}


result_t generate_api_call(const fs::path&dirpath){
  json::value req(json::value::Object_t{});
  const std::string dirname=dirpath.filename();
  req["model"] = config_t::genmodel();
  req["prompt"] = std::format(
R"(ディレクトリ名「{}」の漢字部分を含めた全体の読み仮名をひらがなで出力してください．
ディレクトリ名に記号(、。♡☆など)が含まれる場合は削除してください．
ただし長音記号(ー)は自然な発音の母音として読んでください．
英数字は元の文字をそのまま出力してください．
出力は必ず以下のJSONフォーマットのみとし，余計な説明は一切含めないでください．
{{"reading": "よみがな"}})"
    ,dirname
  );
  req["stream"] = false;
  req["format"] = std::string_view("json");

  std::string res;
  try{ res = call_generate(req); }
  catch(result_t::error_kind kind){ return {dirpath, {}, kind}; }
  
  println("Raw response: {}", res);

  if(!suitable_ruby(res.c_str(),res.c_str()+res.size()))
    return {dirpath, {}, result_t::invalid};
  return {dirpath, res, result_t::success};
}

vector<result_t> check_api_call(vector<result_t>&&list){
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
  for(size_t i=0;i<list.size();i++){
    object_t req{
      {string("previous directory name"), value(list[i].previous.filename().string())},
      {string("generated directory name"), value(list[i].current.value_or("").string())}
    };
    object_t res;
    try{ res = call_check(req); }
    catch(...){
      discard(list[i], "Check LLM response was broken.");
      continue;
    }
    if(!list[i].current.has_value()) continue;
    try{
      bool valid = res.at("valid").boolean();
      if(valid){
        println("Valid : {}\n\t{}", list[i].previous, list[i].current.value_or("[None]"));
      }else{
        std::string instead = res.at("corrected").str();
        println("Invalid : {}\n\tGenerated: {}\n\tInstead: {}", list[i].previous, list[i].current.value_or("[None]"), instead);
        if(instead.empty()){
          list[i].current = std::optional<fs::path>();
          list[i].error=result_t::no_alternative;
          println("\terror message: {}", result_t::why(result_t::no_alternative));
        }else{
          list[i].current = instead;
          list[i].error = result_t::success;
        }
      }
    }catch(...){
      discard(list[i], std::format(
        "JSON parse error: check_api_call.\n{}"
        , json::to_readable(res)
      ));
      continue;
    }
  }
  return list;
}

json::value call_curl(const json::value&req){
  CURL*curl=curl_easy_init();
  if(!curl){
    println("curl init was failed");
    std::exit(1);
  }
  curl_easy_setopt(curl, CURLOPT_URL, "http://localhost:11434/api/generate");
  struct curl_slist* headers=curl_slist_append(nullptr, "Content-Type: application/json");
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
  std::string response, req_str;
  try{
    req_str = json::to_string(req);
  }catch(...){
    throw result_t::to_json;
  }
  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, req_str.c_str());
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
  const CURLcode code = curl_easy_perform(curl);
  curl_easy_cleanup(curl);
  if(code != CURLcode::CURLE_OK){
    throw result_t::api;
  }

  try{
    return json::value::load(response);
  }catch(...){
    throw result_t::from_json;
  }
}

std::string call_generate(const json::value&req){
  json::value res_json=call_curl(req);
  try{
    auto res_body = std::move(res_json["response"].str());
    auto obj = json::value::load(res_body);
    return obj["reading"].str();
  }catch(...){
    println("JSON pase error: call_generate");
    println("\t: {} response", res_json.object().contains("response") ? "has" : "doesn't have");
    println("{}", json::to_readable(res_json));
    throw result_t::from_json;
  }
}

json::value call_check_object_to_json(const json::object_t&v){
  json::value req(json::value::Object_t{});
  req["model"] = config_t::chkmodel();
  req["prompt"] = std::format(
R"(以下の変換ルールに従って，元のディレクトリ名から生成された読み仮名が正しく作られているか厳密に検証してください．
変換ルール:
1. 漢字・ひらがな・カタカナは，全体の正しい読み仮名(ひらがな)に変換されていること．
2. 記号(_や-,♡など)は読み飛ばされ，消去されていること．ただし，長音(ー)は自然な母音になっていること．
3. 英単語は日本語にせず，元のアルファベットのまま残っていること．
4. 生成されたディレクトリ名が空文字列なら検証は不要です．
検証対象は以下の形式で与えられる．
{{
    "previous directory name": "元のディレクトリ名",
    "generated directory name": "せいせいされたでぃれくとりめい"
}}
結果は以下の形式で出力してください．
{{
    "valid": true/false,
    "corrected": "もしvalidがfalseならあなたが正しい読み仮名をここに書いてください．ただし曖昧で決定できない場合はfalseのときも空文字列にしてください．"
}}
検証する対象は以下です．
{})"
    , json::to_string(v)
  );
  req["stream"] = false;
  req["format"] = std::string_view("json");
  return req;
}

json::object_t call_check(const json::object_t&req){
  json::value res_json = call_curl(call_check_object_to_json(req));
  json::value obj;
  std::string body;
  try{
    body = std::move(res_json["response"].str());
  }catch(...){
    println("JSON pase error: call_check\n{}",json::to_readable(res_json,4));
    throw result_t::from_json;
  }
  try{
    return (obj = json::value::load(body)).object();
  }catch(...){
    println("response parse error: call_check\n\t:{}",body);
    throw result_t::from_json;
  }
}