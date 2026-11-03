#include "pal4/xml_generic_tag_parser.h"

#include "pal4/xml_document_error.h"
#include "pal4/xml_runtime_text.h"

namespace pal4 {

std::uint32_t XmlGenericTagParser::Parse(
    const std::uint32_t node, const std::uint32_t source,
    std::array<std::uint32_t, 4>* position, const int mode,
    IXmlParserNodeRuntime& runtime) {
    const auto parent = XmlNode::GetParent(node, runtime);
    const auto text = XmlRuntimeText::SkipWhitespaceAndBom(source, mode, runtime);
    if (position != nullptr) {
        XmlRuntimeText::Advance(*position, text, mode, runtime);
        runtime.WriteWord(node + 4u, (*position)[0]);
        runtime.WriteWord(node + 8u, (*position)[1]);
    }
    if (text == 0u || runtime.ReadByte(text) != '<') {
        if (parent != 0u)
            (void)XmlDocumentError::SetWithPosition(
                parent, 10u, text, position, mode, runtime);
        return 0u;
    }
    (void)runtime.AssignString(node + 32u, 0x8dc780u, 0u);
    auto current = text + 1u;
    while (runtime.ReadByte(current) != 0u && runtime.ReadByte(current) != '>')
        ++current;
    if (current > text + 1u)
        (void)runtime.AssignString(node + 32u, text + 1u,
                                   current - (text + 1u));
    return runtime.ReadByte(current) == '>' ? current + 1u : current;
}

}  // namespace pal4
