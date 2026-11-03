#pragma once
#include <string_view>
namespace pal4::test {
void RegisterCameraPlugin();
void RegisterShadowAtomicPlugins();
void RunCameraCase(std::string_view name);
}
