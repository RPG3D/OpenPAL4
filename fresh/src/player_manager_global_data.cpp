#include "pal4/player_manager_global_data.h"

namespace pal4 {
namespace {

void InitializeText(PlayerManagerGlobalText& text,
                    std::uint8_t incoming_stack_byte,
                    std::string_view source,
                    IPlayerManagerGlobalRuntime& runtime) {
    text.initial_byte = incoming_stack_byte;
    text.data = 0u;
    text.length = 0u;
    text.capacity = 0u;
    runtime.SetErrorFlag(text, 1u);
    runtime.Reallocate(text, static_cast<std::uint32_t>(source.size()));
    runtime.CopyText(text.data, source);
    text.length = static_cast<std::uint32_t>(source.size());
    runtime.WriteByte(text.data + text.length, 0u);
}

void CleanupText(PlayerManagerGlobalText& text,
                 std::uint8_t& cleanup_guard, std::uint8_t guard_bit,
                 IPlayerManagerGlobalRuntime& runtime) {
    if ((cleanup_guard & guard_bit) != 0u)
        return;
    cleanup_guard = static_cast<std::uint8_t>(cleanup_guard | guard_bit);
    if (text.data != 0u) {
        const auto header = text.data - 1u;
        const auto reference_count = runtime.ReadByte(header);
        if (reference_count == 0u || reference_count == 0xffu)
            runtime.Free(header);
        else
            runtime.WriteByte(header,
                static_cast<std::uint8_t>(reference_count - 1u));
    }
    text.data = 0u;
    text.length = 0u;
    text.capacity = 0u;
}

}  // namespace

void PlayerManagerGlobalRuntimeStub::SetErrorFlag(
    PlayerManagerGlobalText&, std::uint8_t) {}
void PlayerManagerGlobalRuntimeStub::Reallocate(
    PlayerManagerGlobalText&, std::uint32_t) {}
void PlayerManagerGlobalRuntimeStub::CopyText(
    std::uint32_t, std::string_view) {}
void PlayerManagerGlobalRuntimeStub::WriteByte(
    std::uint32_t, std::uint8_t) {}
std::uint8_t PlayerManagerGlobalRuntimeStub::ReadByte(std::uint32_t) {
    return 0u;
}
void PlayerManagerGlobalRuntimeStub::Free(std::uint32_t) {}
int PlayerManagerGlobalRuntimeStub::RegisterExit(std::uint32_t) { return 0; }

int PlayerManagerGlobalData::Initialize(
    PlayerManagerGlobalText& text, std::uint8_t incoming_stack_byte,
    IPlayerManagerGlobalRuntime& runtime) {
    InitializeName(text, incoming_stack_byte, runtime);
    return runtime.RegisterExit(kCleanupCallback);
}

void PlayerManagerGlobalData::InitializeName(
    PlayerManagerGlobalText& text, std::uint8_t incoming_stack_byte,
    IPlayerManagerGlobalRuntime& runtime) {
    InitializeText(text, incoming_stack_byte, kName, runtime);
}

void PlayerManagerGlobalData::CleanupName(
    PlayerManagerGlobalText& text, std::uint8_t& cleanup_guard,
    IPlayerManagerGlobalRuntime& runtime) {
    CleanupText(text, cleanup_guard, 1u, runtime);
}

int PlayerManagerGlobalData::InitializeRotation(
    PlayerManagerGlobalText& text, std::uint8_t incoming_stack_byte,
    IPlayerManagerGlobalRuntime& runtime) {
    static_cast<void>(InitializeRotationName(text, incoming_stack_byte,
                                             runtime));
    return runtime.RegisterExit(kRotationCleanupCallback);
}

std::uint32_t PlayerManagerGlobalData::InitializeRotationName(
    PlayerManagerGlobalText& text, std::uint8_t incoming_stack_byte,
    IPlayerManagerGlobalRuntime& runtime) {
    InitializeText(text, incoming_stack_byte, kRotationName, runtime);
    return text.data;
}

void PlayerManagerGlobalData::CleanupRotationName(
    PlayerManagerGlobalText& text, std::uint8_t& cleanup_guard,
    IPlayerManagerGlobalRuntime& runtime) {
    CleanupText(text, cleanup_guard, 2u, runtime);
}

}  // namespace pal4
