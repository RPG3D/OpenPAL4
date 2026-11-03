#pragma once
#include <string_view>
namespace pal4::test {
void RegisterRasterPlugin();
void RunRasterResourceCase(std::string_view name);
}
