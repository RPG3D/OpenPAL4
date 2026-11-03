#include "pal4/script_inventory_text.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <stdexcept>

namespace pal4 {
namespace {

template <class Copy>
void Append(ScriptInventoryTextState& state, std::uint32_t length,
            IScriptInventoryTextRuntime& runtime, Copy copy) {
    if (std::numeric_limits<std::uint32_t>::max() - state.text.length <= length)
        runtime.ThrowLength();
    if (length == 0u)
        return;
    const auto new_length = state.text.length + length;
    if (!runtime.AllocateText(state.text, new_length, 0u))
        return;
    copy(state.text.data + state.text.length);
    runtime.SetTextSize(state.text, new_length);
}

}  // namespace

void ScriptInventoryTextRuntimeStub::SetErrorFlag(
    ScriptInventoryTextState&, std::uint8_t) {}
void ScriptInventoryTextRuntimeStub::RegisterAtExit(std::uint32_t) {}
std::int32_t ScriptInventoryTextRuntimeStub::InvokeValue(
    std::uint32_t, std::uint32_t) { return 0; }
std::uint32_t ScriptInventoryTextRuntimeStub::FindEntry(
    std::int32_t, std::uint32_t) { return 0u; }
InventoryDescription ScriptInventoryTextRuntimeStub::GetDescription(
    std::uint32_t) { return {}; }
bool ScriptInventoryTextRuntimeStub::AllocateText(
    StringBufferState&, std::uint32_t, std::uint8_t) { return false; }
void ScriptInventoryTextRuntimeStub::CopyLiteral(
    std::uint32_t, const char*, std::uint32_t) {}
void ScriptInventoryTextRuntimeStub::CopyDescription(
    std::uint32_t, std::uint32_t, std::uint32_t) {}
void ScriptInventoryTextRuntimeStub::SetTextSize(
    StringBufferState& text, std::uint32_t length) { text.length = length; }
[[noreturn]] void ScriptInventoryTextRuntimeStub::ThrowLength() {
    throw std::length_error("script inventory text overflow");
}

const char* ScriptEventValueRuntimeStub::ReadCString(std::uint32_t) {
    return nullptr;
}
std::int32_t ScriptEventValueRuntimeStub::InvokeParsedValue(
    std::uint32_t, std::int32_t value) { return value; }
std::uint8_t ScriptInventoryTextCleanupRuntimeStub::ReadByte(
    std::uint32_t) { return 0u; }
void ScriptInventoryTextCleanupRuntimeStub::WriteByte(
    std::uint32_t, std::uint8_t) {}
void ScriptInventoryTextCleanupRuntimeStub::Free(std::uint32_t) {}

std::uint32_t ScriptInventoryText::Format(
    ScriptInventoryTextState& state, std::uint32_t callback_object,
    std::uint32_t argument, IScriptInventoryTextRuntime& runtime) {
    if ((state.guard & 1u) == 0u) {
        state.guard |= 1u;
        state.first_byte = static_cast<std::uint8_t>(argument);
        runtime.SetErrorFlag(state, 0u);
        runtime.RegisterAtExit(kCleanupCallback);
    }

    const auto value = callback_object != 0u
        ? runtime.InvokeValue(callback_object, argument) : 0;
    const auto entry = runtime.FindEntry(value, 7u);
    char decimal[511]{};
    (void)std::snprintf(decimal, sizeof(decimal), "%d", value);
    const auto decimal_length = static_cast<std::uint32_t>(std::strlen(decimal));
    if (runtime.AllocateText(state.text, decimal_length, 1u)) {
        runtime.CopyLiteral(state.text.data, decimal, decimal_length);
        runtime.SetTextSize(state.text, decimal_length);
    }

    if (entry == 0u) {
        Append(state, 3u, runtime, [&](std::uint32_t destination) {
            runtime.CopyLiteral(destination, "...", 3u);
        });
        return kBufferAddress;
    }
    Append(state, 1u, runtime, [&](std::uint32_t destination) {
        runtime.CopyLiteral(destination, " ", 1u);
    });
    const auto description = runtime.GetDescription(entry);
    Append(state, description.length, runtime,
           [&](std::uint32_t destination) {
               runtime.CopyDescription(destination, description.data,
                                       description.length);
           });
    return kBufferAddress;
}

std::int32_t ScriptInventoryText::ForwardParsedValue(
    std::uint32_t callback_object, std::uint32_t argument_record,
    IScriptEventValueRuntime& runtime) {
    const auto source = runtime.ReadCString(argument_record);
    const auto value = std::atoi(source != nullptr ? source : "");
    return callback_object != 0u
        ? runtime.InvokeParsedValue(callback_object, value) : value;
}

void ScriptInventoryText::CleanupGlobal(
    ScriptInventoryTextState& state,
    IScriptInventoryTextCleanupRuntime& runtime) {
    if (state.text.data != 0u) {
        const auto header = state.text.data - 1u;
        const auto reference = runtime.ReadByte(header);
        if (reference == 0u || reference == 0xffu)
            runtime.Free(header);
        else
            runtime.WriteByte(header,
                              static_cast<std::uint8_t>(reference - 1u));
    }
    state.text = {};
}

}  // namespace pal4
