#include "pal4/timer_callback.h"

namespace pal4 {
namespace {

TimerCallbackState& Construct(TimerCallbackState& callback,
                              const std::uint32_t owner,
                              const std::uint32_t function,
                              const std::uint32_t argument,
                              const std::uint32_t vtable) {
    callback.owner = owner;
    callback.function = function;
    callback.argument = argument;
    callback.vtable = vtable;
    return callback;
}

}  // namespace

TimerCallbackState& TimerCallback::ConstructFirst(
    TimerCallbackState& callback, const std::uint32_t owner,
    const std::uint32_t function, const std::uint32_t argument) {
    return Construct(callback, owner, function, argument, kFirstVtable);
}

TimerCallbackState& TimerCallback::ConstructSecond(
    TimerCallbackState& callback, const std::uint32_t owner,
    const std::uint32_t function, const std::uint32_t argument) {
    return Construct(callback, owner, function, argument, kSecondVtable);
}

std::uint32_t TimerCallback::Clone(
    const TimerCallbackState& source, ITimerCallbackRuntime& runtime) {
    const auto destination = runtime.Allocate(sizeof(TimerCallbackState));
    if (destination == 0u) {
        return 0u;
    }
    runtime.WriteWord(destination, kBaseVtable);
    runtime.WriteWord(destination + 8u, source.owner);
    runtime.WriteWord(destination + 0xcu, source.function);
    runtime.WriteWord(destination + 0x10u, source.argument);
    runtime.WriteWord(destination, kSecondVtable);
    return destination;
}

}  // namespace pal4
