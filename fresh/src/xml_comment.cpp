#include "pal4/xml_comment.h"

#include "pal4/xml_parser_node.h"
#include "pal4/xml_serializer.h"

namespace pal4 {

void XmlComment::Cleanup(const std::uint32_t comment,
                         IXmlParserNodeRuntime& parser) {
    parser.WriteWord(comment, kVtable);
    XmlParserLifecycle::Destroy(comment, parser);
}

int XmlComment::WriteToFile(
    const std::uint32_t comment, std::FILE* stream, const int depth,
    IXmlSerializationRuntime& serialization) {
    for (int index = 0; index < depth; ++index)
        (void)std::fprintf(stream, "    ");
    const auto block = serialization.ReadWord(comment + 32u);
    const auto text = serialization.ReadText(block + 8u);
    return std::fprintf(stream, "<!--%s-->", text.c_str());
}

std::uint32_t XmlComment::IsComment(
    const std::uint32_t comment, const std::uint32_t visitor,
    IXmlLeafRuntime& runtime) {
    return runtime.CallVisitor(visitor, comment, 24u);
}

std::uint32_t XmlComment::Create(
    const std::uint32_t source, IXmlParserNodeRuntime& parser,
    IXmlLeafRuntime& runtime) {
    const auto comment = runtime.Allocate(44u);
    if (comment == 0u) return 0u;
    (void)XmlParserLifecycle::Initialize(comment, 2u, parser);
    parser.WriteWord(comment, kVtable);
    (void)XmlNode::SetAttribute(source, comment, parser);
    return comment;
}

}  // namespace pal4
