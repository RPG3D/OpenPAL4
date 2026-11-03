#include "pal4/script_property_forwarders.h"

#include <array>
#include <cstddef>
#include <stdexcept>

namespace pal4 {
namespace {

struct TailEntry {
    std::uint32_t address;
    std::uint32_t virtual_offset;
};

constexpr std::array<TailEntry, 57> kUnguardedTailEntries{{
    {0x56e680u, 0x70u}, {0x56e690u, 0x74u}, {0x56e6a0u, 0x78u},
    {0x56e6b0u, 0x7cu}, {0x56e6c0u, 0x94u}, {0x56e6d0u, 0xb4u},
    {0x56e6e0u, 0xa4u}, {0x56e6f0u, 0x90u}, {0x56e700u, 0xb0u},
    {0x56e720u, 0xa0u}, {0x56e740u, 0x80u}, {0x56e750u, 0x88u},
    {0x56e760u, 0x84u}, {0x56e770u, 0xbcu}, {0x56e780u, 0xc4u},
    {0x56e790u, 0xc0u}, {0x56e7a0u, 0xd0u}, {0x56e7c0u, 0xccu},
    {0x56e7e0u, 0xd4u}, {0x56e7f0u, 0xdcu}, {0x56e810u, 0xd8u},
    {0x56e830u, 0x98u}, {0x56e840u, 0xb8u}, {0x56e850u, 0xa8u},
    {0x56e860u, 0x8cu}, {0x56e870u, 0xacu}, {0x56e890u, 0x9cu},
    {0x56e8b0u, 0xe0u}, {0x56e8c0u, 0xe8u}, {0x56e8e0u, 0xe4u},
    {0x56e900u, 0xecu}, {0x56e910u, 0xf0u}, {0x56e920u, 0xf4u},
    {0x56e930u, 0xf8u}, {0x56e940u, 0xfcu}, {0x56e950u, 0x100u},
    {0x56e960u, 0x104u}, {0x56e970u, 0x108u}, {0x56e980u, 0x10cu},
    {0x56e990u, 0x110u}, {0x56e9a0u, 0x114u}, {0x56e9b0u, 0x118u},
    {0x56e9c0u, 0x11cu}, {0x56e9d0u, 0x120u}, {0x56e9e0u, 0x124u},
    {0x56e9f0u, 0x128u}, {0x56ea00u, 0x12cu}, {0x56ea10u, 0x130u},
    {0x56ea20u, 0x13cu}, {0x56ea30u, 0x148u}, {0x56ea50u, 0x154u},
    {0x56ea70u, 0x138u}, {0x56ea80u, 0x144u}, {0x56eaa0u, 0x150u},
    {0x56eac0u, 0x134u}, {0x56ead0u, 0x140u}, {0x56eaf0u, 0x14cu},
}};

ScriptForwardResult Forward(std::uint32_t object,
                            std::uint32_t offset,
                            std::size_t argument_count,
                            ScriptPropertyForwarders::Arguments arguments,
                            IScriptPropertyForwardRuntime& runtime) {
    if (arguments.size() != argument_count)
        throw std::invalid_argument("script property argument count");
    const auto link = runtime.ReadWord(object + 8u);
    if (link == 0u) return runtime.PreservedRegisters();
    return runtime.CallVirtual(link, offset, arguments);
}

}  // namespace

ScriptForwardResult ScriptPropertyForwarders::ForwardUnguarded(
    std::uint32_t entry, std::uint32_t object, Arguments arguments,
    IScriptPropertyForwardRuntime& runtime) {
    for (const auto& item : kUnguardedTailEntries) {
        if (item.address == entry) {
            const auto link = runtime.ReadWord(object + 8u);
            return runtime.CallVirtual(link, item.virtual_offset, arguments);
        }
    }
    throw std::invalid_argument("unknown script property tail entry");
}

ScriptForwardResult ScriptPropertyForwarders::Forward28(
    std::uint32_t object, Arguments arguments,
    IScriptPropertyForwardRuntime& runtime) {
    return Forward(object, 28u, 1u, arguments, runtime);
}
ScriptForwardResult ScriptPropertyForwarders::Forward32(
    std::uint32_t object, Arguments arguments,
    IScriptPropertyForwardRuntime& runtime) {
    return Forward(object, 32u, 1u, arguments, runtime);
}
ScriptForwardResult ScriptPropertyForwarders::Forward36(
    std::uint32_t object, Arguments arguments,
    IScriptPropertyForwardRuntime& runtime) {
    return Forward(object, 36u, 2u, arguments, runtime);
}
ScriptForwardResult ScriptPropertyForwarders::Forward40(
    std::uint32_t object, Arguments arguments,
    IScriptPropertyForwardRuntime& runtime) {
    return Forward(object, 40u, 2u, arguments, runtime);
}
ScriptForwardResult ScriptPropertyForwarders::Forward44(
    std::uint32_t object, Arguments arguments,
    IScriptPropertyForwardRuntime& runtime) {
    return Forward(object, 44u, 2u, arguments, runtime);
}
ScriptForwardResult ScriptPropertyForwarders::Forward48(
    std::uint32_t object, Arguments arguments,
    IScriptPropertyForwardRuntime& runtime) {
    return Forward(object, 48u, 2u, arguments, runtime);
}
ScriptForwardResult ScriptPropertyForwarders::Forward52(
    std::uint32_t object, Arguments arguments,
    IScriptPropertyForwardRuntime& runtime) {
    return Forward(object, 52u, 2u, arguments, runtime);
}
ScriptForwardResult ScriptPropertyForwarders::Forward56(
    std::uint32_t object, Arguments arguments,
    IScriptPropertyForwardRuntime& runtime) {
    return Forward(object, 56u, 2u, arguments, runtime);
}
ScriptForwardResult ScriptPropertyForwarders::Forward60(
    std::uint32_t object, Arguments arguments,
    IScriptPropertyForwardRuntime& runtime) {
    return Forward(object, 60u, 2u, arguments, runtime);
}
ScriptForwardResult ScriptPropertyForwarders::Forward64(
    std::uint32_t object, Arguments arguments,
    IScriptPropertyForwardRuntime& runtime) {
    return Forward(object, 64u, 2u, arguments, runtime);
}
ScriptForwardResult ScriptPropertyForwarders::Forward68(
    std::uint32_t object, Arguments arguments,
    IScriptPropertyForwardRuntime& runtime) {
    return Forward(object, 68u, 3u, arguments, runtime);
}
ScriptForwardResult ScriptPropertyForwarders::Forward72(
    std::uint32_t object, Arguments arguments,
    IScriptPropertyForwardRuntime& runtime) {
    return Forward(object, 72u, 3u, arguments, runtime);
}
ScriptForwardResult ScriptPropertyForwarders::Forward76(
    std::uint32_t object, Arguments arguments,
    IScriptPropertyForwardRuntime& runtime) {
    return Forward(object, 76u, 3u, arguments, runtime);
}
ScriptForwardResult ScriptPropertyForwarders::Forward80(
    std::uint32_t object, Arguments arguments,
    IScriptPropertyForwardRuntime& runtime) {
    return Forward(object, 80u, 1u, arguments, runtime);
}
ScriptForwardResult ScriptPropertyForwarders::Forward84(
    std::uint32_t object, Arguments arguments,
    IScriptPropertyForwardRuntime& runtime) {
    return Forward(object, 84u, 1u, arguments, runtime);
}
ScriptForwardResult ScriptPropertyForwarders::Forward88(
    std::uint32_t object, Arguments arguments,
    IScriptPropertyForwardRuntime& runtime) {
    return Forward(object, 88u, 1u, arguments, runtime);
}
ScriptForwardResult ScriptPropertyForwarders::Forward92(
    std::uint32_t object, Arguments arguments,
    IScriptPropertyForwardRuntime& runtime) {
    return Forward(object, 92u, 1u, arguments, runtime);
}
ScriptForwardResult ScriptPropertyForwarders::Forward96(
    std::uint32_t object, Arguments arguments,
    IScriptPropertyForwardRuntime& runtime) {
    return Forward(object, 96u, 4u, arguments, runtime);
}
ScriptForwardResult ScriptPropertyForwarders::Forward100(
    std::uint32_t object, Arguments arguments,
    IScriptPropertyForwardRuntime& runtime) {
    return Forward(object, 100u, 4u, arguments, runtime);
}
ScriptForwardResult ScriptPropertyForwarders::Forward104(
    std::uint32_t object, Arguments arguments,
    IScriptPropertyForwardRuntime& runtime) {
    return Forward(object, 104u, 4u, arguments, runtime);
}
ScriptForwardResult ScriptPropertyForwarders::Forward108(
    std::uint32_t object, Arguments arguments,
    IScriptPropertyForwardRuntime& runtime) {
    return Forward(object, 108u, 4u, arguments, runtime);
}

}  // namespace pal4
