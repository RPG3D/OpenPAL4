#pragma once
#include <windows.h>
#include <string_view>
namespace pal4::test {
void RegisterShadowCollisionPlugin();
void RunPresentationCase(std::string_view name, HWND window);
}
