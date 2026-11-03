#include "pal4/xml_document_content_parser.h"

#include "pal4/xml_document_error.h"
#include "pal4/xml_comment_parser.h"
#include "pal4/xml_declaration_parser.h"
#include "pal4/xml_generic_tag_parser.h"
#include "pal4/xml_runtime_text.h"
#include "pal4/xml_text_section_parser.h"

namespace pal4 {

std::uint32_t XmlDocumentContentParser::Parse(
    const std::uint32_t document, const std::uint32_t source,
    const std::array<std::uint32_t, 2>* position, int mode,
    IXmlDocumentContentRuntime& runtime) {
    runtime.WriteByte(document + 44u, 0u);
    runtime.WriteWord(document + 48u, 0u);
    (void)runtime.AssignString(document + 32u, 0x8dc780u, 0u);
    runtime.WriteWord(document + 64u, 0u);
    runtime.WriteWord(document + 60u, 0u);
    if (source == 0u || runtime.ReadByte(source) == 0u) {
        (void)XmlDocumentError::Set(document, 13u, 0u, 0u, 0, runtime);
        return 0u;
    }

    const auto line = position == nullptr ? 0u : (*position)[0];
    const auto column = position == nullptr ? 0u : (*position)[1];
    runtime.WriteWord(document + 4u, line);
    runtime.WriteWord(document + 8u, column);
    std::array<std::uint32_t, 4> cursor{
        line, column, source, runtime.ReadWord(document + 56u)};
    if (mode == 0 && runtime.ReadByte(source) == 0xefu &&
        runtime.ReadByte(source + 1u) == 0xbbu &&
        runtime.ReadByte(source + 2u) == 0xbfu) {
        mode = 1;
        runtime.WriteByte(document + 68u, 1u);
    }

    auto current = XmlRuntimeText::SkipWhitespaceAndBom(source, mode, runtime);
    if (current == 0u) {
        (void)XmlDocumentError::Set(document, 13u, 0u, 0u, 0, runtime);
        return 0u;
    }
    while (runtime.ReadByte(current) != 0u) {
        const auto node = XmlNodeFactory::Create(document, current, mode, runtime);
        if (node == 0u) break;
        const auto kind = runtime.ReadWord(node + 20u);
        const auto next = kind == 1u ?
            XmlElementParser::Parse(node, current, &cursor, mode, runtime) :
            kind == 3u ?
            XmlGenericTagParser::Parse(node, current, &cursor, mode, runtime) :
            kind == 2u ?
            XmlCommentParser::Parse(node, current, &cursor, mode, runtime) :
            kind == 4u ?
            XmlTextSectionParser::Parse(node, current, &cursor, mode, runtime) :
            kind == 5u ?
            XmlDeclarationParser::Parse(node, current, &cursor, mode, runtime) :
            runtime.ParseNode(node, current, &cursor, mode);
        (void)XmlNode::AddChild(document, node, runtime);
        if (mode == 0) {
            const auto declaration = runtime.CallVirtual(node, 52u);
            if (declaration != 0u) {
                const auto block = runtime.ReadWord(declaration + 48u);
                const auto encoding = block + 8u;
                if (runtime.ReadByte(encoding) == 0u ||
                    XmlRuntimeText::HasPrefix(encoding, "UTF-8", true, runtime) ||
                    XmlRuntimeText::HasPrefix(encoding, "UTF8", true, runtime))
                    mode = 1;
                else
                    mode = 2;
            }
        }
        if (next == current) return 0u;
        current = XmlRuntimeText::SkipWhitespaceAndBom(next, mode, runtime);
        if (current == 0u) break;
    }
    if (runtime.ReadWord(document + 24u) == 0u) {
        (void)XmlDocumentError::Set(document, 13u, 0u, 0u, mode, runtime);
        return 0u;
    }
    return current;
}

}  // namespace pal4
