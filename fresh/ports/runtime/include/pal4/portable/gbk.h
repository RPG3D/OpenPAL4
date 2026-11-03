#pragma once
#include <string>
#include <string_view>
namespace pal4::portable {
// 严格CP936：ASCII、0x80欧元与有效双字节。拒绝截断、未映射及非法序列。
std::string DecodeGbk(std::string_view input);
}
