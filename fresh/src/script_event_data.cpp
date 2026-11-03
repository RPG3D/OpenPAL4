#include "pal4/script_event_data.h"

#include <cstring>
#include <stdexcept>

namespace pal4 {

void ScriptEventDataRuntimeStub::DestroyXmlNode(std::uint32_t) {}
std::uint32_t ScriptEventDataRuntimeStub::Free(std::uint32_t) { return 0u; }
std::uint8_t ScriptEventDataRuntimeStub::ReadByte(std::uint32_t) { return 0u; }
void ScriptEventDataRuntimeStub::WriteByte(std::uint32_t, std::uint8_t) {}

void ScriptEventStringRuntimeStub::ThrowLength() {
    throw std::length_error("script event string length");
}
std::uint8_t ScriptEventStringRuntimeStub::SetErrorFlag(
    ScriptEventDataState&, std::uint8_t) { return 0u; }
void ScriptEventStringRuntimeStub::Reallocate(
    ScriptEventDataState& state, std::uint32_t length) {
    state.words[1] = 0x10000u;
    state.words[3] = length;
}
std::uint8_t ScriptEventStringRuntimeStub::ReadByte(std::uint32_t) {
    return 0u;
}
void ScriptEventStringRuntimeStub::WriteByte(std::uint32_t, std::uint8_t) {}
void ScriptEventStringRuntimeStub::Copy(
    std::uint32_t, const char*, std::uint32_t) {}

std::uint32_t ScriptEventDocumentRuntimeStub::Allocate(std::uint32_t) {
    return 0u;
}
std::uint32_t ScriptEventDocumentRuntimeStub::InitializeDocument(
    std::uint32_t, const char*, std::uint32_t) { return 0u; }
void ScriptEventDocumentRuntimeStub::Free(std::uint32_t) {}
std::uint8_t ScriptEventCompareRuntimeStub::ReadByte(std::uint32_t) {
    return 0u;
}

ScriptEventDataState& ScriptEventData::Initialize(
    ScriptEventDataState& state, std::uint32_t object_address) {
    const auto address_high_byte = object_address >> 24u;
    state.words[0] = (state.words[0] & 0xffffff00u) | address_high_byte;
    state.words[1] = 0u;
    state.words[2] = 0u;
    state.words[3] = 0u;
    state.words[4] = 3u;
    state.words[5] = (state.words[5] & 0xffffff00u) | address_high_byte;
    state.words[6] = 0u;
    state.words[7] = 0u;
    state.words[8] = 0u;
    state.words[9] = 0u;
    return state;
}

std::uint32_t ScriptEventData::Cleanup(
    ScriptEventDataState& state, IScriptEventDataRuntime& runtime) {
    const auto owned_node = state.words[9];
    if (owned_node != 0u) {
        runtime.DestroyXmlNode(owned_node);
        static_cast<void>(runtime.Free(owned_node));
        state.words[9] = 0u;
    }

    const auto begin = state.words[6];
    const auto end = state.words[7];
    for (auto node = begin; node != end; node += 60u)
        runtime.DestroyXmlNode(node);
    static_cast<void>(runtime.Free(begin));
    state.words[6] = 0u;
    state.words[7] = 0u;
    state.words[8] = 0u;

    std::uint32_t result = 0u;
    const auto string_data = state.words[1];
    if (string_data != 0u) {
        const auto header = string_data - 1u;
        const auto count = runtime.ReadByte(header);
        if (count == 0u || count == 0xffu) {
            result = runtime.Free(header);
        } else {
            runtime.WriteByte(header, static_cast<std::uint8_t>(count - 1u));
            result = header;
        }
    }
    state.words[1] = 0u;
    state.words[2] = 0u;
    state.words[3] = 0u;
    return result;
}

std::uint8_t ScriptEventData::AssignCString(
    ScriptEventDataState& state, const char* source,
    IScriptEventStringRuntime& runtime) {
    const auto source_length = std::strlen(source);
    if (source_length > 0xfffffffdu) runtime.ThrowLength();
    const auto length = static_cast<std::uint32_t>(source_length);

    const auto current_data = state.words[1];
    if (current_data != 0u) {
        const auto count = runtime.ReadByte(current_data - 1u);
        if (count != 0u && count != 0xffu) {
            if (length == 0u) {
                runtime.WriteByte(current_data - 1u,
                                  static_cast<std::uint8_t>(count - 1u));
                state.words[1] = 0u;
                state.words[2] = 0u;
                state.words[3] = 0u;
                return static_cast<std::uint8_t>(current_data);
            }
            runtime.Reallocate(state, length);
            runtime.Copy(state.words[1], source, length);
            state.words[2] = length;
            runtime.WriteByte(state.words[1] + length, 0u);
            return static_cast<std::uint8_t>(length);
        }
    }

    if (length == 0u) return runtime.SetErrorFlag(state, 1u);
    if (state.words[3] > 31u || state.words[3] < length) {
        static_cast<void>(runtime.SetErrorFlag(state, 1u));
        runtime.Reallocate(state, length);
    }
    runtime.Copy(state.words[1], source, length);
    state.words[2] = length;
    runtime.WriteByte(state.words[1] + length, 0u);
    return static_cast<std::uint8_t>(length);
}

std::uint32_t ScriptEventData::GetOrCreateConditionDocument(
    ScriptEventDataState& state, IScriptEventDocumentRuntime& runtime) {
    if (state.words[9] != 0u) return state.words[9];
    const auto allocated = runtime.Allocate(60u);
    if (allocated == 0u) return 0u;
    try {
        state.words[9] = runtime.InitializeDocument(
            allocated, "behAIAtomic-condition-attribute", 0u);
    } catch (...) {
        runtime.Free(allocated);
        throw;
    }
    return state.words[9];
}

std::int32_t ScriptEventData::CompareCString(
    const ScriptEventDataState& state, const char* text,
    IScriptEventCompareRuntime& runtime) {
    const auto text_length = std::strlen(text);
    const auto stored_length = state.words[2];
    const auto common = stored_length < text_length
        ? stored_length : text_length;
    for (std::size_t i = 0u; i < common; ++i) {
        const auto stored = runtime.ReadByte(
            state.words[1] + static_cast<std::uint32_t>(i));
        const auto requested = static_cast<std::uint8_t>(text[i]);
        if (stored != requested)
            return stored < requested ? -1 : 1;
    }
    if (stored_length < text_length) return -1;
    if (stored_length > text_length) return 1;
    return 0;
}

}  // namespace pal4
