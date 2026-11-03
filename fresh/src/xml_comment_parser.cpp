#include "pal4/xml_comment_parser.h"

#include "pal4/xml_document_error.h"
#include "pal4/xml_runtime_text.h"

namespace pal4 {

std::uint32_t XmlCommentParser::Parse(
    const std::uint32_t node, const std::uint32_t source,
    std::array<std::uint32_t, 4>* position, const int mode,
    IXmlNodeFactoryRuntime& runtime) {
    const auto parent = XmlNode::GetParent(node, runtime);
    (void)runtime.AssignString(node + 32u, 0x8dc780u, 0u);
    const auto text = XmlRuntimeText::SkipWhitespaceAndBom(source, mode, runtime);
    if (position != nullptr) {
        XmlRuntimeText::Advance(*position, text, mode, runtime);
        runtime.WriteWord(node + 4u, (*position)[0]);
        runtime.WriteWord(node + 8u, (*position)[1]);
    }
    if (!XmlRuntimeText::HasPrefix(text, "<!--", false, runtime)) {
        if (parent != 0u)
            (void)XmlDocumentError::SetWithPosition(
                parent, 11u, text, position, mode, runtime);
        return 0u;
    }

    return XmlRuntimeText::ParseDecoded(
        text + 4u, node + 32u, "-->", mode, runtime);
}

}  // namespace pal4
