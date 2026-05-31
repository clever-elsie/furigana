#include <bit>

#include <utf8.hpp>

namespace utf8{
using namespace std;

uint32_t decode_one(const char*& it, const char* end){
  if(it>=end) return 0;
  uint8_t c = uint8_t(*it++);
  if(c < 0x80) return c;
  const int len=std::countl_one(c);
  if(len<2||len>4||end-it<len-1) return 0;
  uint32_t cp=c&(0xFFu>>len);
  for(int i=1;i<len;++i){
    uint8_t t=(uint8_t)*it++;
    if((t&0xC0)!=0x80) return 0;
    cp=(cp<<6)|(t&0x3F);
  }
  return cp;
}

void encode_one(uint32_t cp, string& out) {
  if (cp <= 0x7F) { out.push_back(static_cast<char>(cp)); return; }
  int len = (cp <= 0x7FF) ? 2 : (cp <= 0xFFFF) ? 3 : 4;
  char buf[4];
  for (int i = len - 1; i > 0; --i) {
    buf[i] = static_cast<char>(0x80 | (cp & 0x3F));
    cp >>= 6;
  }
  static const uint8_t first_mask[5] = {0, 0, 0xC0, 0xE0, 0xF0};
  buf[0] = static_cast<char>(first_mask[len] | (cp & (0xFFu >> (len + 1))));
  out.append(buf, len);
}

uint32_t normalize_hiragana_base(uint32_t cp){
  switch(cp){
    case 0x3041 ... 0x3049: // ぁ→あ, ぃ→い, ぅ→う, ぇ→え, ぉ→お
      return cp + (cp&1);
    case 0x304c ... 0x3062: // が行 ざ行 だ，ぢ
      return cp - !(cp&1); // 偶数のとき一つ前の奇数に戻す
    case 0x3063: // っ→つ
      return 0x3064;
    case 0x3064 ... 0x3069: // づ，で，ど
      return cp - (cp&1);
    case 0x306F ... 0x307D: // は行 ば行 ぱ行
      return cp - cp%3;
    case 0x3083 ... 0x3087: // ゃ→や, ゅ→ゆ, ょ→よ, ゎ→わ
      return cp + (cp&1);
    case 0x308E: return 0x308F; // ゎ→わ
    case 0x3094: return 0x3046; // ゔ→う
    case 0x3095: return 0x304B; // ゕ→か
    case 0x3096: return 0x3051; // ゖ→け
    default: return cp;
  }
}

bool suitable_ruby(const char*begin, const char*end){
  if(begin>=end) return false;
  for(;begin<end;){
    uint32_t code = utf8::decode_one(begin, end);
    if(code == 0) return false;
    // ASCII 印字可能文字とスペース
    if(code >= 0x20 && code <= 0x7E) continue;
    if(code == 0x3000) continue; // 全角スペース
    if(code == 0x30FC) continue; // 長音ー
    if(utf8::ishiragana(code) || utf8::iskatakana(code)) continue;
    return false;
  }
  return true;
}

bool has_ruby(const std::filesystem::path&dirpath){
  const std::string dirname = dirpath.filename().string();
  // 《がある場合，.*《[^《]*》$を満たさなければならない
  // つまり最後に出てくる《以降に》が無ければならず，またそれが最後の文字でなければならない
  // また最後の《》に挟まれている文字列は英数字，ひらがな，カタカナのいずれかでなければならない．
  const size_t end_pos = dirname.rfind("》");
  if(end_pos == std::string::npos || end_pos != dirname.size()-3)
    return false;
  const size_t start_pos = dirname.rfind("《");
  if(start_pos == std::string::npos)
    return false;
  if(start_pos + 3 >= end_pos)
    return false;
  return true;
}

bool has_suitable_ruby(const std::filesystem::path&dirpath){
  const std::string dirname = dirpath.filename().string();
  // 《がある場合，.*《[^《]*》$を満たさなければならない
  // つまり最後に出てくる《以降に》が無ければならず，またそれが最後の文字でなければならない
  // また最後の《》に挟まれている文字列は英数字，ひらがな，カタカナのいずれかでなければならない．
  const size_t end_pos = dirname.rfind("》");
  if(end_pos == std::string::npos || end_pos != dirname.size()-3)
    return false;
  const size_t start_pos = dirname.rfind("《");
  if(start_pos == std::string::npos)
    return false;
  if(start_pos + 3 >= end_pos)
    return false;
  const char* begin = dirname.data() + start_pos + 3;
  const char* end = dirname.data() + end_pos;
  return suitable_ruby(begin,end);
}

bool no_need_ruby(const std::filesystem::path&dirpath){
  const std::string dirname = dirpath.filename().string();
  if(dirname.empty()) return false;
  const char* begin = dirname.c_str();
  const char* end = begin + dirname.size();
  return suitable_ruby(begin,end);
}

std::string without_ruby(const std::string&dirname){
  const size_t end_pos = dirname.rfind("》");
  if(end_pos == std::string::npos || end_pos != dirname.size()-3)
    return dirname;
  const size_t start_pos = dirname.rfind("《");
  if(start_pos == std::string::npos)
    return dirname;
  if(start_pos + 3 >= end_pos)
    return dirname;
  return std::string(dirname.begin(), dirname.begin() + start_pos);
}

std::string extract_ruby(const std::string&dirname){
  const size_t end_pos = dirname.rfind("》");
  if(end_pos == std::string::npos || end_pos != dirname.size()-3)
    return "";
  const size_t start_pos = dirname.rfind("《");
  if(start_pos == std::string::npos)
    return "";
  if(start_pos + 3 >= end_pos)
    return "";
  const char* begin = dirname.data() + start_pos + 3;
  const char* end = dirname.data() + end_pos;
  return std::string(begin, end);
}

} // namespace utf8