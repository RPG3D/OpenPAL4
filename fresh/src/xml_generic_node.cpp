#include "pal4/xml_generic_node.h"

#include "pal4/xml_leaf_runtime.h"
#include "pal4/xml_parser_node.h"

namespace pal4 {

std::uint32_t XmlGenericNode::AcceptVisitor(
    const std::uint32_t node, const std::uint32_t visitor,
    IXmlLeafRuntime& runtime) {
    return runtime.CallVisitor(visitor, node, 20u);
}

std::uint32_t XmlGenericNode::Create(
    const std::uint32_t source, IXmlParserNodeRuntime& parser,
    IXmlLeafRuntime& runtime) {
    const auto node = runtime.Allocate(44u);
    if (node == 0u) return 0u;
    (void)XmlParserLifecycle::Initialize(node, 3u, parser);
    parser.WriteWord(node, kVtable);
    (void)XmlNode::SetAttributeThunk(source, node, parser);
    return node;
}

void XmlGenericNode::Cleanup(const std::uint32_t node,
                             IXmlParserNodeRuntime& parser) {
    parser.WriteWord(node, kVtable);
    XmlParserLifecycle::Destroy(node, parser);
}

}  // namespace pal4
