#include "pal4/xml_content_parser.h"

#include "pal4/xml_document_error.h"
#include "pal4/xml_comment_parser.h"
#include "pal4/xml_generic_tag_parser.h"
#include "pal4/xml_runtime_text.h"
#include "pal4/xml_text_section_parser.h"
#include "pal4/xml_text_node.h"

namespace pal4 {

std::uint32_t XmlContentParser::Parse(
    const std::uint32_t element, const std::uint32_t source,
    std::array<std::uint32_t, 4>* position, const int mode,
    IXmlElementRuntime& runtime) {
    const auto parent = XmlNode::GetParent(element, runtime);
    auto current = XmlRuntimeText::SkipWhitespaceAndBom(source, mode, runtime);
    if (current == 0u) {
        if (parent != 0u)
            (void)XmlDocumentError::SetWithPosition(
                parent, 6u, 0u, nullptr, mode, runtime);
        return 0u;
    }
    auto raw = source;
    for (;;) {
        if (runtime.ReadByte(current) == 0u) return current;
        std::uint32_t node = 0u;
        std::uint32_t next = 0u;
        if (runtime.ReadByte(current) == '<') {
            if (XmlRuntimeText::HasPrefix(current, "</", false, runtime))
                return current;
            node = XmlNodeFactory::Create(element, current, mode, runtime);
            if (node == 0u) return 0u;
            const auto kind = runtime.ReadWord(node + 20u);
            next = kind == 1u ?
                XmlElementParser::Parse(node, current, position, mode, runtime) :
                kind == 3u ?
                XmlGenericTagParser::Parse(node, current, position, mode, runtime) :
                kind == 2u ?
                XmlCommentParser::Parse(node, current, position, mode, runtime) :
                kind == 4u ?
                XmlTextSectionParser::Parse(node, current, position, mode, runtime) :
                runtime.ParseNode(node, current, position, mode);
            (void)XmlNode::AddChild(element, node, runtime);
        } else {
            node = runtime.Allocate(48u);
            if (node == 0u) {
                if (parent != 0u)
                    (void)XmlDocumentError::SetWithPosition(
                        parent, 3u, 0u, nullptr, mode, runtime);
                return 0u;
            }
            (void)XmlParserLifecycle::Initialize(node, 4u, runtime);
            runtime.WriteWord(node, 0x849aa0u);
            (void)runtime.AssignString(node + 32u, 0x8dc780u, 0u);
            runtime.WriteByte(node + 44u, 0u);
            const auto text_source = runtime.ReadByte(0x8c1f98u) != 0u ?
                current : raw;
            next = XmlTextSectionParser::Parse(
                node, text_source, position, mode, runtime);
            if (XmlTextNode::IsWhitespaceOnly(node, runtime)) {
                (void)runtime.DeleteNode(node, 1u, 0u);
                node = 0u;
            }
            if (node != 0u)
                (void)XmlNode::AddChild(element, node, runtime);
        }
        if (next == current) return 0u;
        raw = next;
        current = XmlRuntimeText::SkipWhitespaceAndBom(next, mode, runtime);
        if (current == 0u) {
            if (parent != 0u)
                (void)XmlDocumentError::SetWithPosition(
                    parent, 6u, 0u, nullptr, mode, runtime);
            return 0u;
        }
    }
}

}  // namespace pal4
