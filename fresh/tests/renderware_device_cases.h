#pragma once
#include <string_view>
namespace pal4::test {
void PrepareDeviceCase(std::string_view name);
void RunDeviceCase(std::string_view name);
}
