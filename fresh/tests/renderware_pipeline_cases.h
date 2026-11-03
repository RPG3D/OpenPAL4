#pragma once
#include <string_view>
namespace pal4::test {
void RunPipelineCase(std::string_view name, bool& fail_next);
}
