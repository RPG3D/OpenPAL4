#pragma once
#include <windows.h>
#include <string_view>
namespace pal4::test {
void RunResetCase(std::string_view name, HWND engine_window);
}
