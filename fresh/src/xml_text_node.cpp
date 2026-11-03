#include "pal4/xml_text_node.h"

#include "pal4/xml_parser_document.h"
#include "pal4/xml_parser_node.h"

#include <cctype>

namespace pal4 {

std::uint32_t XmlTextNode::AcceptVisitor(
    const std::uint32_t node, const std::uint32_t visitor,
    IXmlLeafRuntime& runtime) {
    return runtime.CallVisitor(visitor, node, 28u);
}

std::uint32_t XmlTextNode::Create(
    const std::uint32_t source, IXmlDocumentRuntime& parser) {
    const auto node = parser.Allocate(48u);
    if (node == 0u) return 0u;
    (void)XmlParserLifecycle::Initialize(node, 4u, parser);
    parser.WriteWord(node, kVtable);
    try {
        (void)parser.AssignString(node + 32u, kInitialText,
                                  parser.StringLength(kInitialText));
    } catch (...) {
        XmlParserLifecycle::Destroy(node, parser);
        (void)parser.Free(node);
        throw;
    }
    parser.WriteByte(node + 44u, 0u);
    (void)XmlNode::SetAttributeWithFlag(source, node, parser);
    return node;
}

void XmlTextNode::Cleanup(const std::uint32_t node,
                          IXmlParserNodeRuntime& parser) {
    parser.WriteWord(node, kVtable);
    XmlParserLifecycle::Destroy(node, parser);
}

bool XmlTextNode::IsWhitespaceOnly(
    const std::uint32_t node, IXmlParserNodeRuntime& parser) {
    const auto block = parser.ReadWord(node + 32u);
    const auto length = parser.ReadWord(block);
    for (std::uint32_t index = 0u; index < length; ++index) {
        const auto byte = parser.ReadByte(block + 8u + index);
        if (std::isspace(byte) == 0 && byte != '\n' && byte != '\r')
            return false;
    }
    return true;
}

}  // namespace pal4
