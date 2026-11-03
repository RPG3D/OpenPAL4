#pragma once
#include <string_view>
namespace pal4::test {
void RegisterDdsPlugin();
void RunDdsCase(std::string_view name);
}
