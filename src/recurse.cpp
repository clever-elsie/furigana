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
vector<result_t> check_api_call_batched(vector<result_t>&& list);
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
  return check_api_call_batched(std::move(unchecked));
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
ディレクトリ名に記号が含まれる場合は読み飛ばしてください．
ただし長音記号は自然な発音の母音として読んでください．
英語は日本語の読みを与えず元の文字をそのまま出力してください．
英語以外のアルファベットで書かれたものは一般的な日本語の読みを与えてください．
出力は必ず以下のJSONフォーマットのみとし，余計な説明は一切含めないでください．
{{"reading": "よみがな"}})"
    ,dirname
  );
  req["stream"] = false;
  req["format"] = std::string_view("json");

  std::string res;
  try{ res = call_generate(req); }
  catch(result_t::error_kind kind){ return {dirpath, {}, kind}; }

  if(!suitable_ruby(res.c_str(),res.c_str()+res.size()))
    return {dirpath, {}, result_t::invalid};
  return {dirpath, res, result_t::success};
}

void check_api_call_span(std::span<result_t>range);
vector<result_t> check_api_call_batched(vector<result_t>&&list){
  constexpr size_t step = 3;
  for(size_t i=0;i<list.size();i+=step){
    check_api_call_span(std::span<result_t>(&list[i], std::min(i+step, list.size())));
  }
  return list;
}

void check_api_call_span(std::span<result_t>range){
  json::object_t range_list;
  for(size_t j=0;j<range.size();++j){
    using json::value;
    using json::object_t;
    range_list["task_"+std::to_string(j)]=object_t{
      {std::string("previous directory name"), value(range[j].previous.string())},
      {std::string("generated directory name"), value(range[j].current.value_or("").string())}
    };
  }
  json::object_t res = call_check(range_list);
  for(size_t j=0;j<range.size();++j){
    if(!range[j].current.has_value()) continue;
    json::object_t obj;
    try{
      obj = res.at("task_"+std::to_string(j)).object();
    }catch(const std::out_of_range&err){
      println("JSON parse error: check_api_call_batched");
      println("JSON parse error: id {} was lacked", j);
      println("{}", json::to_readable(res));
      std::exit(1);
    }catch(...){
      println("JSON parse error: check_api_call_batched\n{}", json::to_readable(res[std::to_string(j)]));
      std::exit(1);
    }
    try{
      bool valid = obj.at("valid").boolean();
      if(!valid){
        std::string instead = obj.at("corrected").str();
        if(instead.empty()){
          range[j].current = std::optional<fs::path>();
          range[j].error=result_t::no_alternative;
        }else{
          range[j].current = instead;
          range[j].error = result_t::success;
        }
      }
    }catch(...){
      println("JSON parse error: check_api_call_batched");
      println("{}", json::to_readable(obj));
      std::exit(1);
    }
  }
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
    std::string& res_body = res_json["response"].str();
    json::value obj = json::value::load(res_body);
    return obj["reading"].str();
  }catch(...){
    println("JSON pase error: call_generate");
    if(bool has_response = res_json.object().contains("response");has_response){
      println("\t: has response");
    }else{
      println("\t: doesn't has reponse");
    }
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
検証対象は以下の形式で整数id付きで複数個与えられる．
{{
    "task_0":{{
        "previous directory name": "元のディレクトリ名",
        "generated directory name": "せいせいされたでぃれくとりめい"
    }},
}}
結果は以下の形式で出力してください．
"result":{{
    "task_0":{{
        "valid": true/false,
        "corrected": "もしvalidがfalseならあなたが正しい読み仮名をここに書いてください．ただし曖昧で決定できない場合はfalseのときも空文字列にしてください．"
    }},
}}
検証する対象となるリストを与えます．
{})"
    , json::to_string(v)
  );
  req["stream"] = false;
  req["format"] = std::string_view("json");
  return req;
}

json::object_t call_check(const json::object_t&req){
  json::value res_json = call_curl(call_check_object_to_json(req));
  try{
    std::string& body = res_json["response"].str();
    json::value obj = json::value::load(body);
    return obj["result"].object();
  }catch(...){
    println("JSON pase error: call_check\n{}",json::to_readable(res_json,4));
    throw result_t::from_json;
  }
}