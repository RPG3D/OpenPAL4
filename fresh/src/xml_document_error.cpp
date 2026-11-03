#include "pal4/xml_document_error.h"

#include "pal4/xml_text_cursor.h"
#include "pal4/xml_runtime_text.h"

namespace pal4 {

namespace {

class CursorAdapter final : public IXmlTextCursorRuntime {
public:
    explicit CursorAdapter(IXmlParserNodeRuntime& runtime) : runtime_(runtime) {}
    std::uint32_t ReadWord(std::uint32_t address) override {
        return runtime_.ReadWord(address);
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        runtime_.WriteWord(address, value);
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        return runtime_.ReadByte(address);
    }

private:
    IXmlParserNodeRuntime& runtime_;
};

}  // namespace

std::uint8_t XmlDocumentError::Set(
    const std::uint32_t document, const std::uint32_t code,
    const std::uint32_t end, const std::uint32_t cursor,
    const int utf8_mode, IXmlParserNodeRuntime& runtime) {
    const auto previous = runtime.ReadByte(document + 44u);
    if (previous != 0u) return previous;
    runtime.WriteByte(document + 44u, 1u);
    runtime.WriteWord(document + 48u, code);
    const auto message = runtime.ReadWord(0x8c20a0u + code * 4u);
    (void)runtime.AssignString(document + 52u, message,
                               runtime.StringLength(message));
    runtime.WriteWord(document + 64u, 0xffffffffu);
    runtime.WriteWord(document + 60u, 0xffffffffu);
    if (end == 0u || cursor == 0u)
        return static_cast<std::uint8_t>(end);
    CursorAdapter adapter{runtime};
    XmlTextCursor::Advance(cursor, end, utf8_mode, adapter);
    runtime.WriteWord(document + 60u, runtime.ReadWord(cursor));
    const auto column = runtime.ReadWord(cursor + 4u);
    runtime.WriteWord(document + 64u, column);
    return static_cast<std::uint8_t>(column);
}

std::uint8_t XmlDocumentError::SetWithPosition(
    const std::uint32_t document, const std::uint32_t code,
    const std::uint32_t end, std::array<std::uint32_t, 4>* position,
    const int utf8_mode, IXmlParserNodeRuntime& runtime) {
    const auto previous = runtime.ReadByte(document + 44u);
    if (previous != 0u) return previous;
    runtime.WriteByte(document + 44u, 1u);
    runtime.WriteWord(document + 48u, code);
    const auto message = runtime.ReadWord(0x8c20a0u + code * 4u);
    (void)runtime.AssignString(document + 52u, message,
                               runtime.StringLength(message));
    runtime.WriteWord(document + 64u, 0xffffffffu);
    runtime.WriteWord(document + 60u, 0xffffffffu);
    if (end == 0u || position == nullptr)
        return static_cast<std::uint8_t>(end);
    XmlRuntimeText::Advance(*position, end, utf8_mode, runtime);
    runtime.WriteWord(document + 60u, (*position)[0]);
    runtime.WriteWord(document + 64u, (*position)[1]);
    return static_cast<std::uint8_t>((*position)[1]);
}

}  // namespace pal4
