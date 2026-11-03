#include "pal4/xml_node_workflow.h"

namespace pal4 {

std::uint32_t XmlNodeWorkflow::AcceptVisitor(
    const std::uint32_t node, const std::uint32_t visitor,
    IXmlDocumentRuntime& runtime) {
    const auto root = node + 44u;
    const auto first_attribute = runtime.ReadWord(node + 76u);
    const auto attributes = first_attribute == root ? 0u : first_attribute;
    if (runtime.VisitorEnter(visitor, node, attributes, 4u)) {
        auto child = runtime.ReadWord(node + 24u);
        while (child != 0u) {
            if (!runtime.VisitChild(child, visitor, 64u)) break;
            child = runtime.ReadWord(child + 40u);
        }
    }
    return runtime.VisitorLeave(visitor, node, 12u);
}

std::uint32_t XmlNodeWorkflow::ParseAndClone(
    const std::uint32_t node, IXmlDocumentRuntime& runtime) {
    const auto parser = runtime.Allocate(80u);
    if (parser == 0u) return 0u;
    const auto source = runtime.ReadWord(node + 32u) + 8u;
    try {
        (void)XmlParserDocument::Parse(parser, source, runtime);
    } catch (...) {
        (void)runtime.Free(parser);
        throw;
    }
    (void)XmlParserDocument::CloneInto(node, parser, runtime);
    return parser;
}

std::uint32_t XmlNodeWorkflow::AcceptVisitor2(
    const std::uint32_t node, const std::uint32_t visitor,
    IXmlDocumentRuntime& runtime) {
    if (runtime.VisitorEnter(visitor, node, 0u, 8u)) {
        auto child = runtime.ReadWord(node + 24u);
        while (child != 0u) {
            if (!runtime.VisitChild(child, visitor, 64u)) break;
            child = runtime.ReadWord(child + 40u);
        }
    }
    return runtime.VisitorLeave(visitor, node, 16u);
}

}  // namespace pal4
