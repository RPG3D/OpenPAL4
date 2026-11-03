#include "pal4/message_task.h"

#include <cstdint>
#include <stdexcept>

namespace pal4 {
namespace {

void ReleaseTemporary(const MessageStringTemporary& temporary,
                      IMessageStringRuntime& runtime) {
    if (temporary.data == 0u)
        return;
    const auto count = runtime.RefCount(temporary.data);
    if (count != 0u && count != 0xffu)
        runtime.SetRefCount(temporary.data,
                            static_cast<std::uint8_t>(count - 1u));
    else
        runtime.Free(temporary.data - 1u);
}

}  // namespace

std::uint8_t MessageTask::LoadString(
    MessageTaskState& state, const std::uint32_t config,
    IMessageStringRuntime& runtime) {
    if (config == 0u)
        return 0u;

    const auto temporary = runtime.ReadString(
        config, "Attr_Task_Message_What", kEmptyConfigAddress);
    try {
        if (temporary.aliases_destination) {
            runtime.EraseDestination(state, temporary.length, 0xffffffffu);
            runtime.EraseDestination(state, 0u, 0u);
        } else {
            const auto source = temporary.data != 0u
                                    ? temporary.data
                                    : kEmptyTextAddress;
            if (temporary.length != 0u && runtime.RefCount(source) < 0xfeu) {
                (void)runtime.ResetDestination(state, 72u, 1u);
                state.words[19u] = source;
                state.words[20u] = temporary.length;
                state.words[21u] = temporary.capacity;
                runtime.SetRefCount(source, static_cast<std::uint8_t>(
                                                runtime.RefCount(source) + 1u));
            } else if (runtime.AllocateDestination(
                           state, temporary.length, 1u)) {
                runtime.CopyBytes(state.words[19u], source, temporary.length);
                state.words[20u] = temporary.length;
                runtime.WriteNull(state.words[19u] + temporary.length);
            }
        }
    } catch (...) {
        ReleaseTemporary(temporary, runtime);
        throw;
    }
    ReleaseTemporary(temporary, runtime);
    return 1u;
}

std::uint8_t MessageTask::ResetString(
    MessageTaskState& state, IMessageStringRuntime& runtime) {
    const auto text = runtime.ResetText();
    if (text.size() > 0xfffffffdu)
        throw std::length_error("MessageTask reset string exceeds x86 limit");
    const auto length = static_cast<std::uint32_t>(text.size());
    const auto pointer = state.words[19u];
    const auto count = pointer != 0u ? runtime.RefCount(pointer) : 0u;

    if (pointer != 0u && count != 0u && count != 0xffu && length == 0u) {
        runtime.SetRefCount(pointer, static_cast<std::uint8_t>(count - 1u));
        state.words[19u] = 0u;
        state.words[20u] = 0u;
        state.words[21u] = 0u;
        state.words[22u] = 0u;
        return static_cast<std::uint8_t>(pointer);
    }

    if (length == 0u) {
        const auto result = runtime.ResetDestination(state, 72u, 1u);
        state.words[22u] = 0u;
        return static_cast<std::uint8_t>(result);
    }

    if (pointer != 0u && count != 0u && count != 0xffu) {
        runtime.ReallocateDestination(state, length);
    } else if (state.words[21u] > 0x1fu || state.words[21u] < length) {
        (void)runtime.ResetDestination(state, 72u, 1u);
        runtime.ReallocateDestination(state, length);
    }
    runtime.CopyLiteral(state.words[19u], text);
    state.words[20u] = length;
    runtime.WriteNull(state.words[19u] + length);
    state.words[22u] = 0u;
    return static_cast<std::uint8_t>(length);
}

}  // namespace pal4
