#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmsystem.h>

#include <cstdint>
#include <iostream>

int main() {
    HMODULE module = GetModuleHandleW(L"winmm.dll");
    if (module == nullptr) {
        std::cerr << "WINMM is not loaded\n";
        return 1;
    }
    const auto imported = reinterpret_cast<DWORD(WINAPI*)()>(
        GetProcAddress(module, "timeGetTime"));
    if (imported == nullptr) {
        std::cerr << "WINMM timeGetTime export is missing\n";
        return 1;
    }

    const DWORD first = timeGetTime();
    const DWORD second = imported();
    if (static_cast<std::uint32_t>(second - first) > 1000u) {
        std::cerr << "direct and exported timeGetTime calls disagree\n";
        return 1;
    }
    std::cout << "WINMM timeGetTime boundary passed\n";
}
