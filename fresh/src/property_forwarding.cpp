#include "pal4/property_forwarding.h"

namespace pal4 {

std::uint32_t PropertyForwardingRuntimeStub::ReadWord(std::uint32_t) {
    return 0u;
}
std::uint32_t PropertyForwardingRuntimeStub::Invoke0(std::uint32_t,
                                                     std::uint32_t) {
    return 0u;
}
std::uint32_t PropertyForwardingRuntimeStub::Invoke1(std::uint32_t,
                                                     std::uint32_t,
                                                     std::uint32_t) {
    return 0u;
}
std::uint32_t PropertyForwardingRuntimeStub::Invoke2(std::uint32_t,
                                                     std::uint32_t,
                                                     std::uint32_t,
                                                     std::uint32_t) {
    return 0u;
}

std::uint32_t PropertyForwarding::GetValue(
    const std::uint32_t context, const std::uint32_t property,
    IPropertyForwardingRuntime& runtime) {
    return runtime.Invoke1(property, 4u, context);
}

std::uint32_t PropertyForwarding::SetValue(
    const std::uint32_t context, const std::uint32_t property,
    IPropertyForwardingRuntime& runtime) {
    return runtime.Invoke1(property, 28u, context);
}

std::uint32_t PropertyForwarding::GetValue2(
    const std::uint32_t context, const std::uint32_t property,
    IPropertyForwardingRuntime& runtime) {
    return runtime.Invoke1(property, 8u, context);
}

std::uint32_t PropertyForwarding::SetValue2(
    const std::uint32_t context, const std::uint32_t property,
    IPropertyForwardingRuntime& runtime) {
    return runtime.Invoke1(property, 32u, context);
}

std::uint32_t PropertyForwarding::GetFromOffset(
    const std::uint32_t wrapper, const std::uint32_t offset,
    IPropertyForwardingRuntime& runtime) {
    const auto property = runtime.ReadWord(wrapper + 12u);
    const auto base = runtime.ReadWord(wrapper + 16u);
    return runtime.Invoke1(property, 16u, offset + base);
}

std::uint32_t PropertyForwarding::SetFromOffset(
    const std::uint32_t wrapper, const std::uint32_t offset,
    const std::uint32_t value, IPropertyForwardingRuntime& runtime) {
    const auto property = runtime.ReadWord(wrapper + 12u);
    const auto base = runtime.ReadWord(wrapper + 16u);
    return runtime.Invoke2(property, 20u, offset + base, value);
}

std::uint32_t PropertyForwarding::GetFromVtable(
    const std::uint32_t wrapper, IPropertyForwardingRuntime& runtime) {
    return runtime.Invoke0(runtime.ReadWord(wrapper + 12u), 4u);
}

std::uint32_t PropertyForwarding::GetFromOffset2(
    const std::uint32_t wrapper, const std::uint32_t offset,
    IPropertyForwardingRuntime& runtime) {
    const auto property = runtime.ReadWord(wrapper + 4u);
    const auto base = runtime.ReadWord(wrapper + 8u);
    return runtime.Invoke1(property, 16u, offset + base);
}

std::uint32_t PropertyForwarding::SetFromOffset2(
    const std::uint32_t wrapper, const std::uint32_t offset,
    const std::uint32_t value, IPropertyForwardingRuntime& runtime) {
    const auto property = runtime.ReadWord(wrapper + 4u);
    const auto base = runtime.ReadWord(wrapper + 8u);
    return runtime.Invoke2(property, 20u, offset + base, value);
}

}  // namespace pal4
