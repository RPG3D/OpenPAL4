#include "pal4/event_property_script.h"

namespace pal4 {

std::uint32_t EventPropertyScriptRuntimeStub::ReadWord(std::uint32_t) {
    return 0u;
}
std::uint8_t EventPropertyScriptRuntimeStub::ReadByte(std::uint32_t) {
    return 0u;
}
void EventPropertyScriptRuntimeStub::WriteByte(std::uint32_t, std::uint8_t) {}
std::uint32_t EventPropertyScriptRuntimeStub::Free(std::uint32_t) {
    return 0u;
}
std::uint32_t EventPropertyScriptRuntimeStub::NameRecord(
    std::uint32_t, std::uint32_t) { return 0u; }
std::uint32_t EventPropertyScriptRuntimeStub::StringLength(
    std::uint32_t) { return 0u; }
void EventPropertyScriptRuntimeStub::SetErrorFlag(
    EventScriptSearchState&, std::uint8_t) {}
void EventPropertyScriptRuntimeStub::SetSearchText(
    EventScriptSearchState&, std::uint32_t, std::uint32_t) {}
std::uint32_t EventPropertyScriptRuntimeStub::FindNode(
    std::uint32_t, const EventScriptSearchState&) { return 0u; }
void EventPropertyScriptRuntimeStub::ConvertAttribute(
    std::uint32_t, EventScriptTextState&, std::uint32_t) {}
void EventPropertyScriptRuntimeStub::SetRecord(
    std::uint32_t, std::uint32_t, const EventScriptTextState&,
    std::uint32_t) {}
void EventPropertyScriptRuntimeStub::InitializeString(
    StringBufferState&, std::uint32_t) {}
void EventPropertyScriptRuntimeStub::CleanupString(StringBufferState&) {}
void EventPropertyScriptRuntimeStub::SetString(
    std::uint32_t, std::uint32_t, const StringBufferState&,
    std::uint32_t) {}

std::uint32_t EventPropertyScript::Find(
    const std::uint32_t wrapper, const std::uint32_t source,
    std::uint32_t& owner,
    IEventPropertyScriptRuntime& runtime) {
    const auto record = runtime.NameRecord(source, 4u);
    auto name = runtime.ReadWord(record + 4u);
    if (name == 0u) name = StringBuffer::EmptyLogMessageAddress();
    owner = runtime.ReadWord(wrapper + 4u);
    EventScriptSearchState search{};
    search.tag = source & 0xffu;
    runtime.SetErrorFlag(search, 0u);
    runtime.SetSearchText(search, name, runtime.StringLength(name));
    std::uint32_t node = 0u;
    try {
        node = runtime.FindNode(owner + 20u, search);
    } catch (...) {
        runtime.SetErrorFlag(search, 1u);
        throw;
    }
    runtime.SetErrorFlag(search, 1u);
    return node;
}

std::uint32_t EventPropertyScript::ReleaseText(
    const EventScriptTextState& text,
    IEventPropertyScriptRuntime& runtime) {
    if (text.data == 0u) return 0u;
    const auto header = text.data - 1u;
    const auto references = runtime.ReadByte(header);
    if (references == 0u || references == 0xffu)
        return runtime.Free(header);
    runtime.WriteByte(header, static_cast<std::uint8_t>(references - 1u));
    return header;
}

std::uint32_t EventPropertyScript::SetFromScript(
    const std::uint32_t wrapper, const std::uint32_t source,
    IEventPropertyScriptRuntime& runtime) {
    std::uint32_t owner = 0u;
    const auto node = Find(wrapper, source, owner, runtime);
    if (node == runtime.ReadWord(owner + 24u) || node == 0xffffffe4u)
        return node;
    EventScriptTextState text{};
    runtime.ConvertAttribute(node + 28u, text, 0x8e425cu);
    try {
        runtime.SetRecord(source, runtime.ReadWord(wrapper + 8u), text, 20u);
    } catch (...) {
        (void)ReleaseText(text, runtime);
        throw;
    }
    return ReleaseText(text, runtime);
}

std::uint32_t EventPropertyScript::SetStringFromScript(
    const std::uint32_t wrapper, const std::uint32_t source,
    IEventPropertyScriptRuntime& runtime) {
    std::uint32_t owner = 0u;
    const auto node = Find(wrapper, source, owner, runtime);
    if (node == runtime.ReadWord(owner + 24u) || node == 0xffffffe4u)
        return node;
    EventScriptTextState text{};
    runtime.ConvertAttribute(node + 28u, text, 0x8e425cu);
    try {
        StringBufferState temporary{};
        runtime.InitializeString(temporary,
            text.data == 0u ? StringBuffer::EmptyLogMessageAddress() :
                              text.data);
        try {
            runtime.SetString(source, runtime.ReadWord(wrapper + 8u),
                              temporary, 20u);
        } catch (...) {
            runtime.CleanupString(temporary);
            throw;
        }
        runtime.CleanupString(temporary);
    } catch (...) {
        (void)ReleaseText(text, runtime);
        throw;
    }
    return ReleaseText(text, runtime);
}

}  // namespace pal4
