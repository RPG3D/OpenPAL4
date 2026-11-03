#pragma once
#include <cstdint>
#include <iosfwd>
#include <map>
#include <string>
#include <string_view>
namespace pal4::portable {
using TextCatalog=std::map<std::uint32_t,std::string>;
// 共用字典传输：原CP936载荷在读取时转为UTF8，调用者指定四字节格式标记。
TextCatalog ReadTextCatalog(std::istream&,std::string_view magic);
}
