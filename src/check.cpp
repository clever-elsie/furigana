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
    std::string MLC = tar.current.value_or(""); // most likely candidate
    json::value::Array_t candidates;
    if(MLC.size()) candidates.push_back(MLC);
    constexpr size_t max_trial = 10;
    for(size_t trial=0;trial<max_trial;++trial){
        const object_t req{
            {string("previous directory name"), value(tar.previous.filename().string())},
            {string("generated directory name"), candidates}
        };
        object_t res;
        try{ res = call_check(config, req); }
        catch(...){
            discard(tar, "Check LLM response was broken.");
            return tar;
        }
        // valid, correct
        // valid が [0, candidates.size()) ならそれを答えとする．
        // そうでないとき，correctを候補に加える．
        try{
            if(!res.contains("valid")) continue; // validがない場合は失敗
            int64_t valid = [&]{
                const auto&v=res.at("valid");
                if(v.is<json::value::types::Number>())
                    return v.integer();
                else if(v.is<json::value::types::String>())
                    return std::stol(v.str());
                return -1;
            }();
            if(0<=valid&&valid<(int64_t)candidates.size()){
                tar.current = candidates[valid].str();
                tar.error = result_t::success;
                println("Valid : {}\n\t{}", tar.previous, tar.current.value());
                return tar;
            }else{
                if(!res.contains("correct")) continue; // correctがない場合は失敗
                std::string instead = res.at("correct").str();
                if(instead.empty() || !utf8::suitable_ruby(instead.data(), instead.data() + instead.size()))
                    continue;
                if(tar.current.has_value())
                    if(tar.current.value() == instead){// 一致するならOK
                        tar.error = result_t::success;
                        println("Valid : {}\n\t{}", tar.previous, tar.current.value());
                        return tar;
                    }
                println("Invalid : {}", tar.previous);
                println("\tGenerated: {}", tar.current.value_or("[None]"));
                println("\tInstead  : {}", instead);
                tar.current = instead;
                tar.error = result_t::success;
            }
        }catch(...){
            std::string json;
            try{ json = json::to_readable(res); }
            catch(...){}
            discard(tar, std::format("JSON parse error: check_api_call.\n{}", json));
        }
    }
    tar.error = result_t::no_alternative;
    tar.current = std::nullopt;
    return tar;
}



json::value call_check_object_to_json(const config_t&config, const json::object_t&v){
    json::value req(json::value::Object_t{});
    req["model"] = config->check_model;
    req["prompt"] = std::format(
            R"("request":{{
    以下の変換ルールに従って，元のディレクトリ名から生成された読み仮名が正しく作られているか厳密に検証してください．
    変換ルール:
    1. 漢字・ひらがな・カタカナは，全体の正しい読み仮名(ひらがな)に変換されている
    2. ディレクトリ名に記号(、。♡☆など)が含まれる場合は削除する
    3. 長音記号(ー)は自然な発音の母音として読む
    4. 「々」は前の漢字の読み方を繰り返す
    4. 英数字は元の文字を出力
    6. 漢数字，ローマ数字(I,V,X)や①のような丸付きの数字はASCIIの数字0-9に変換
    7. 伏字として記号が用いられている場合は，尤もらしい読みを補う

    検証ルール
    1. ルールを満たすか確認する．
    3. thinkingとcriticismを行ってください．
    4. wordsetの単語の対応表は最優先でチェックすること．
    5. 候補が複数ある場合は，正しいものを選び，正しいものがない場合はそれらを元に正しい読み仮名を作り出してください．
    6. 候補が存在しない場合はルールに従って読み仮名を生成してください．

    検証対象は以下の形式で与えられる．
    "validation check target":{{
        "previous directory name": "元のディレクトリ名",
        "generated directory names":[
            "せいせいされたでぃれくとりめい",
            "せいせいされたでぃれくとりめい2",...
        ]
    }}
    結果は以下の形式で出力してください．
    {{
        "thinking": ここに詳しい思考過程を書く．
        "criticism": ここに推論に対する批判を書く．
        "valid": 候補に正しいものがあると判断したらその番号をここに書いてください．もし，正しい候補が無ければ-1を書いてください．出力は整数で行ってください．
        "correct": もしvalidが-1ならあなたが正しい読み仮名をここに書いてください．
    }}
}},
"wordset": {},
"validation check target" : {}
)" , config->wordset, json::to_string(v)
    );
    req["stream"] = false;
    req["format"] = std::string_view("json");
    req["options"] = json::object_t{
        {"num_ctx",config->num_ctx},
        {"temperature", 0.01}, // 検証タスクでは温度は小さく
    };
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
