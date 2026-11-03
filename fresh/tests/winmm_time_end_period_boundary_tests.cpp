#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmsystem.h>

#include <iostream>
#include <type_traits>

static_assert(std::is_same_v<decltype(&timeEndPeriod),
                             MMRESULT(WINAPI*)(UINT)>);

int main() {
    TIMECAPS caps{};
    if (timeGetDevCaps(&caps, sizeof(caps)) != TIMERR_NOERROR) {
        std::cerr << "WINMM timer capabilities unavailable\n";
        return 1;
    }
    HMODULE module = GetModuleHandleW(L"winmm.dll");
    const auto exported = module == nullptr ? nullptr
        : reinterpret_cast<MMRESULT(WINAPI*)(UINT)>(
              GetProcAddress(module, "timeEndPeriod"));
    if (exported == nullptr) {
        std::cerr << "WINMM timeEndPeriod export is missing\n";
        return 1;
    }
    if (timeBeginPeriod(caps.wPeriodMin) != TIMERR_NOERROR) {
        std::cerr << "WINMM timeBeginPeriod failed\n";
        return 1;
    }
    const MMRESULT direct = timeEndPeriod(caps.wPeriodMin);
    if (direct != TIMERR_NOERROR) {
        std::cerr << "linked timeEndPeriod failed\n";
        return 1;
    }
    if (timeBeginPeriod(caps.wPeriodMin) != TIMERR_NOERROR) {
        std::cerr << "second timeBeginPeriod failed\n";
        return 1;
    }
    const MMRESULT indirect = exported(caps.wPeriodMin);
    if (indirect != TIMERR_NOERROR) {
        std::cerr << "exported timeEndPeriod failed\n";
        return 1;
    }
    std::cout << "WINMM timeEndPeriod boundary passed\n";
}
