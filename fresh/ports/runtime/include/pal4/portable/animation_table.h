#pragma once
#include <cstdint>
#include <span>
#include <string>
#include <utility>
#include <vector>
namespace pal4::portable {
// 保留原.tab的别名/资源顺序；不复制VC6字符串或数组内部实现。
std::vector<std::pair<std::string,std::string>> ReadAnimationTable(std::span<const std::uint8_t> bytes);
}
