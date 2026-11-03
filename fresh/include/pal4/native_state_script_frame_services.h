#pragma once
#include <cstdint>
#include <functional>

namespace pal4 {
class INativeScriptHandlerWaitRuntime;

// Borrows a script owner without exposing its runner or taking over its lifetime.
// The callback processes handler-only waits and the owner's SDK frame policy;
// it must propagate script failure and must not decode movies or advance fades.
struct NativeStateScriptFrameServices {
    std::uint32_t executor{};
    std::function<void(float, INativeScriptHandlerWaitRuntime&)> process_waits;
};
}
