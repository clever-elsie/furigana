#include <cstdlib>
#include <print>

#include <curl/curl.h>

#include <args.hpp>
#include <api.hpp>
#include <result_t.hpp>

using std::println;

// libcurlのコールバック
size_t WriteCallback(void*contents, size_t size, size_t nmemb, std::string*userp){
  userp->append((char*)contents, size*nmemb);
  return size*nmemb;
}

json::value call_curl(const config_t&config, const json::value&req){
  CURL*curl=curl_easy_init();
  if(!curl){
    println("curl init was failed");
    std::exit(1);
  }
  // URLの設定は初回だけでいいのでstatic
  static std::string URL("http://" + config->ip + ":" + std::to_string(config->port) + "/api/generate");
  curl_easy_setopt(curl, CURLOPT_URL, URL.c_str());
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
  curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5l);
  curl_easy_setopt(curl, CURLOPT_TIMEOUT, config->timeout.count());
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