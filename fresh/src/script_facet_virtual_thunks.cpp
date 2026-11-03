#include "pal4/script_facet_virtual_thunks.h"

#include <array>
#include <stdexcept>

namespace pal4 {
namespace {

constexpr std::array<std::uint32_t, 42> kVtableOffsets{
    0x7cu, 0xbcu, 0x9cu, 0xdcu, 0x5cu, 0x3cu, 0x1cu,
    0x80u, 0xc0u, 0xa0u, 0xe0u, 0x40u, 0xc4u, 0xa4u,
    0xe4u, 0x64u, 0x44u, 0x08u, 0xc8u, 0xa8u, 0x88u,
    0x68u, 0x28u, 0xccu, 0xacu, 0x8cu, 0x2cu, 0xb0u,
    0x90u, 0xd0u, 0x70u, 0xb4u, 0x94u, 0xd4u, 0x74u,
    0x34u, 0xb8u, 0x98u, 0xd8u, 0x58u, 0x38u, 0x18u,
};

}  // namespace

std::uint32_t ScriptFacetVirtualThunks::Dispatch(
    std::uint32_t native_entry, std::uint32_t object,
    std::span<const std::uint32_t> arguments,
    IScriptFacetVirtualRuntime& runtime) {
    if (native_entry < 0x588c00u || native_entry > 0x588e90u ||
        (native_entry - 0x588c00u) % 0x10u != 0u)
        throw std::invalid_argument("unknown script facet virtual thunk");
    const auto index = (native_entry - 0x588c00u) / 0x10u;
    return runtime.Invoke(object, kVtableOffsets[index], arguments);
}

}  // namespace pal4
