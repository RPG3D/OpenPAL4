#pragma once
#include <string_view>
namespace pal4::test {
void RegisterFramePlugin();
void RunFrameCase(std::string_view name, bool& fail_next_allocation);
}
