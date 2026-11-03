#include "pal4/xml_file_parser.h"

#include "pal4/compact_string_buffer.h"

namespace pal4 {

std::uint32_t XmlFileParser::Initialize(
    const std::uint32_t parser, IXmlDocumentRuntime& runtime) {
    (void)XmlParserLifecycle::Initialize(parser, 0u, runtime);
    runtime.WriteWord(parser + 52u, CompactStringBuffer::kEmptyBlock);
    runtime.WriteWord(parser + 64u, 0xffffffffu);
    runtime.WriteWord(parser + 60u, 0xffffffffu);
    runtime.WriteWord(parser, 0x849a10u);
    runtime.WriteWord(parser + 56u, 4u);
    runtime.WriteByte(parser + 68u, 0u);
    runtime.WriteByte(parser + 44u, 0u);
    runtime.WriteWord(parser + 48u, 0u);
    try {
        (void)runtime.AssignString(parser + 52u, kEmptyErrorText,
                                   runtime.StringLength(kEmptyErrorText));
    } catch (...) {
        const auto block = runtime.ReadWord(parser + 52u);
        if (block != CompactStringBuffer::kEmptyBlock)
            (void)runtime.Free(block);
        XmlParserLifecycle::Destroy(parser, runtime);
        throw;
    }
    runtime.WriteWord(parser + 64u, 0u);
    runtime.WriteWord(parser + 60u, 0u);
    return parser;
}

void XmlFileParser::Cleanup(
    const std::uint32_t parser, IXmlDocumentRuntime& runtime) {
    runtime.WriteWord(parser, 0x849a10u);
    const auto block = runtime.ReadWord(parser + 52u);
    if (block != CompactStringBuffer::kEmptyBlock)
        (void)runtime.Free(block);
    XmlParserLifecycle::Destroy(parser, runtime);
}

std::uint32_t XmlFileParser::CloneInto(
    const std::uint32_t source, const std::uint32_t destination,
    IXmlDocumentRuntime& runtime) {
    (void)XmlNode::SetAttribute(source, destination, runtime);
    runtime.WriteByte(destination + 44u, runtime.ReadByte(source + 44u));
    const auto text = runtime.ReadWord(source + 52u) + 8u;
    std::uint32_t result = runtime.AssignString(
        destination + 52u, text, runtime.StringLength(text));
    auto child = runtime.ReadWord(source + 24u);
    while (child != 0u) {
        const auto clone = runtime.CloneChild(child, 60u);
        result = XmlNode::AddChild(destination, clone, runtime);
        child = runtime.ReadWord(child + 40u);
    }
    return result;
}

std::uint32_t XmlFileParser::CloneNew(
    const std::uint32_t source, IXmlDocumentRuntime& runtime) {
    const auto parser = runtime.Allocate(72u);
    if (parser == 0u) return 0u;
    try {
        (void)Initialize(parser, runtime);
    } catch (...) {
        (void)runtime.Free(parser);
        throw;
    }
    (void)CloneInto(source, parser, runtime);
    return parser;
}

}  // namespace pal4
