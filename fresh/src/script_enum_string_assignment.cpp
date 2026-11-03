#include "pal4/script_enum_string_assignment.h"

#include <stdexcept>

namespace pal4 {

[[noreturn]] void ScriptEnumStringRuntimeStub::ThrowRange() {
    throw std::out_of_range("script enum string range");
}
std::uint32_t ScriptEnumStringRuntimeStub::DetachAndReset(
    ScriptEnumStringState&) { return 0u; }
bool ScriptEnumStringRuntimeStub::AllocateText(
    ScriptEnumStringState&, std::uint32_t, std::uint8_t) { return false; }
void ScriptEnumStringRuntimeStub::SetTextSize(
    ScriptEnumStringState& state, std::uint32_t length) {
    state.text.length = length;
}
std::uint32_t ScriptEnumStringRuntimeStub::EmptyStringAddress() {
    return 0u;
}
std::uint8_t ScriptEnumStringRuntimeStub::ReadByte(std::uint32_t) {
    return 0xffu;
}
void ScriptEnumStringRuntimeStub::WriteByte(
    std::uint32_t, std::uint8_t) {}
void ScriptEnumStringRuntimeStub::Copy(
    std::uint32_t, std::uint32_t, std::uint32_t) {}
void ScriptEnumStringRuntimeStub::Free(std::uint32_t) {}

std::uint8_t ScriptEnumStringAssignment::Assign(
    ScriptEnumStringState& destination,
    const ScriptEnumStringState& source,
    IScriptEnumStringRuntime& runtime) {
    const auto length = source.text.length;
    if (&destination == &source) {
        if (destination.text.length < length)
            runtime.ThrowRange();
        (void)runtime.DetachAndReset(destination);
        const auto remaining = destination.text.length - length;
        if (remaining != 0u) {
            runtime.Copy(destination.text.data + length,
                         destination.text.data + length + remaining, 0u);
            const auto new_length = destination.text.length - remaining;
            if (runtime.AllocateText(destination, new_length, 0u))
                runtime.SetTextSize(destination, new_length);
        }
        return static_cast<std::uint8_t>(
            runtime.DetachAndReset(destination));
    }

    const auto candidate_data = length != 0u
        ? (source.text.data != 0u ? source.text.data
                                  : runtime.EmptyStringAddress())
        : 0u;
    if (length != 0u && runtime.ReadByte(candidate_data - 1u) < 0xfeu) {
        const auto previous = destination.text.data;
        if (previous != 0u) {
            const auto reference = runtime.ReadByte(previous - 1u);
            if (reference == 0u || reference == 0xffu)
                runtime.Free(previous - 1u);
            else
                runtime.WriteByte(previous - 1u,
                    static_cast<std::uint8_t>(reference - 1u));
        }
        destination.text = {candidate_data, source.text.length,
                            source.text.capacity};
        const auto reference = static_cast<std::uint8_t>(
            runtime.ReadByte(candidate_data - 1u) + 1u);
        runtime.WriteByte(candidate_data - 1u, reference);
        return reference;
    }

    if (!runtime.AllocateText(destination, length, 1u))
        return 0u;
    const auto source_data = source.text.data != 0u
        ? source.text.data : runtime.EmptyStringAddress();
    runtime.Copy(destination.text.data, source_data, length);
    destination.text.length = length;
    runtime.WriteByte(destination.text.data + length, 0u);
    return static_cast<std::uint8_t>(length);
}

}  // namespace pal4
