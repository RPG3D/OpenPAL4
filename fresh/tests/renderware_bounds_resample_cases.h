#pragma once
#include <string_view>
namespace pal4::test {
void RegisterBoundsPlugin();
void RunBoundsResampleCase(std::string_view name,bool& fail_next);
}
