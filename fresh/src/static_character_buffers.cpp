#include "pal4/static_character_buffers.h"

namespace pal4 {

std::uint32_t StaticCharacterBuffers::InitializeWide(
    const std::uint8_t incoming_stack_byte,
    IStaticCharacterBufferRuntime& runtime) {
    state_.wide_header_byte = incoming_stack_byte;
    const auto begin = runtime.Allocate(0x100u);
    state_.wide_begin = begin;
    for (std::uint32_t index = 0u; index < 128u; ++index) {
        const auto current = begin + index * 2u;
        if (current != 0u) runtime.WriteWord(current, 0u);
    }
    const auto end = begin + 0x100u;
    state_.wide_end = end;
    state_.wide_capacity_end = end;
    return end;
}

int StaticCharacterBuffers::InitializeWideAndRegister(
    const std::uint8_t incoming_stack_byte,
    IStaticCharacterBufferRuntime& runtime) {
    static_cast<void>(InitializeWide(incoming_stack_byte, runtime));
    return runtime.RegisterExit(kWideCleanupEntry);
}

void StaticCharacterBuffers::CleanupWide(
    IStaticCharacterBufferRuntime& runtime) {
    if ((state_.cleanup_bits & 1u) != 0u) return;
    state_.cleanup_bits = static_cast<std::uint8_t>(
        state_.cleanup_bits | 1u);
    runtime.Free(state_.wide_begin);
    state_.wide_begin = 0u;
    state_.wide_end = 0u;
    state_.wide_capacity_end = 0u;
}

std::uint32_t StaticCharacterBuffers::InitializeMultibyte(
    const std::uint8_t incoming_stack_byte,
    IStaticCharacterBufferRuntime& runtime) {
    state_.multibyte_header_byte = incoming_stack_byte;
    const auto begin = runtime.Allocate(0x80u);
    state_.multibyte_begin = begin;
    for (std::uint32_t index = 0u; index < 128u; ++index) {
        const auto current = begin + index;
        if (current != 0u) runtime.WriteByte(current, 0u);
    }
    const auto end = begin + 0x80u;
    state_.multibyte_end = end;
    state_.multibyte_capacity_end = end;
    return end;
}

int StaticCharacterBuffers::InitializeMultibyteAndRegister(
    const std::uint8_t incoming_stack_byte,
    IStaticCharacterBufferRuntime& runtime) {
    static_cast<void>(InitializeMultibyte(incoming_stack_byte, runtime));
    return runtime.RegisterExit(kMultibyteCleanupEntry);
}

void StaticCharacterBuffers::CleanupMultibyte(
    IStaticCharacterBufferRuntime& runtime) {
    if ((state_.cleanup_bits & 2u) != 0u) return;
    state_.cleanup_bits = static_cast<std::uint8_t>(
        state_.cleanup_bits | 2u);
    runtime.Free(state_.multibyte_begin);
    state_.multibyte_begin = 0u;
    state_.multibyte_end = 0u;
    state_.multibyte_capacity_end = 0u;
}

}  // namespace pal4
