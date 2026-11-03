#include "pal4/event_property_wrapper.h"

namespace pal4 {

std::uint32_t EventPropertyWrapperRuntimeStub::ReadWord(std::uint32_t) {
    return 0u;
}
std::uint32_t EventPropertyWrapperRuntimeStub::Invoke0(std::uint32_t,
                                                        std::uint32_t) {
    return 0u;
}
std::uint32_t EventPropertyWrapperRuntimeStub::Invoke1(std::uint32_t,
                                                        std::uint32_t,
                                                        std::uint32_t) {
    return 0u;
}
std::uint32_t EventPropertyWrapperRuntimeStub::Invoke2(std::uint32_t,
                                                        std::uint32_t,
                                                        std::uint32_t,
                                                        std::uint32_t) {
    return 0u;
}
void EventPropertyWrapperRuntimeStub::WriteWord(std::uint32_t, std::uint32_t) {}
void EventPropertyWrapperRuntimeStub::DestroyChild(std::uint32_t,
                                                    std::uint32_t) {}
void EventPropertyWrapperRuntimeStub::DecrementTempOff() {}
void EventPropertyWrapperRuntimeStub::Free(std::uint32_t) {}

std::uint32_t EventPropertyWrapper::ForwardSet(
    const std::uint32_t wrapper, const std::uint32_t key,
    const std::uint32_t value, IEventPropertyWrapperRuntime& runtime) {
    const auto parent = runtime.ReadWord(wrapper + 4u);
    const auto translated = runtime.Invoke1(parent, 0u, key);
    const auto child = runtime.ReadWord(wrapper + 8u);
    return runtime.Invoke2(child, 4u, translated, value);
}

std::uint32_t EventPropertyWrapper::Call8(
    const std::uint32_t wrapper, IEventPropertyWrapperRuntime& runtime) {
    return runtime.Invoke0(runtime.ReadWord(wrapper + 8u), 8u);
}

std::uint32_t EventPropertyWrapper::Call12(
    const std::uint32_t wrapper, IEventPropertyWrapperRuntime& runtime) {
    return runtime.Invoke0(runtime.ReadWord(wrapper + 8u), 12u);
}

void EventPropertyWrapper::Cleanup(
    const std::uint32_t wrapper, IEventPropertyWrapperRuntime& runtime) {
    runtime.WriteWord(wrapper, 0x849888u);
    const auto child = runtime.ReadWord(wrapper + 8u);
    try {
        if (child != 0u) runtime.DestroyChild(child, 1u);
    } catch (...) {
        runtime.WriteWord(wrapper, 0x849898u);
        runtime.DecrementTempOff();
        throw;
    }
    runtime.WriteWord(wrapper, 0x849898u);
    runtime.DecrementTempOff();
}

std::uint32_t EventPropertyWrapper::Destroy(
    const std::uint32_t wrapper, const std::uint32_t flag,
    IEventPropertyWrapperRuntime& runtime) {
    Cleanup(wrapper, runtime);
    if ((flag & 1u) != 0u) runtime.Free(wrapper);
    return wrapper;
}

}  // namespace pal4
