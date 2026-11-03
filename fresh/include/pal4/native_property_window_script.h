#pragma once
#include <cstdint>
#include <functional>
#include <string_view>
namespace pal4 {
class NativePkFileManagerRuntime;
class NativeTimedScriptStack;
struct NativePropertyWindowScriptServices {
    NativePkFileManagerRuntime& packages;
    NativeTimedScriptStack& scripts;
    std::uint32_t arena,ui_manager;
    std::string_view arena_script;
    std::function<void(std::string_view,std::uint32_t)> sound;
    std::function<void(std::uint32_t,std::uint32_t,std::uint32_t)> event;
};
class NativePropertyWindowScript final {
public:
    // Queue the current arena's EarthBall under PFNSkipScript[EarthBall], then
    // close System. Call inside DispatchUiScript to preserve preemption policy.
    static void Execute(NativePropertyWindowScriptServices);
};
}
