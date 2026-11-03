#include "pal4/xml_text_section_parser.h"

#include "pal4/xml_document_error.h"
#include "pal4/xml_runtime_text.h"

#include <string>

namespace pal4 {

std::uint32_t XmlTextSectionParser::Parse(
    const std::uint32_t node, const std::uint32_t source,
    std::array<std::uint32_t, 4>* position, const int mode,
    IXmlNodeFactoryRuntime& runtime) {
    (void)runtime.AssignString(node + 32u, 0x8dc780u, 0u);
    const auto parent = XmlNode::GetParent(node, runtime);
    if (position != nullptr) {
        XmlRuntimeText::Advance(*position, source, mode, runtime);
        runtime.WriteWord(node + 4u, (*position)[0]);
        runtime.WriteWord(node + 8u, (*position)[1]);
    }
    const bool cdata = XmlRuntimeText::HasPrefix(
        source, "<![CDATA[", false, runtime);
    if (runtime.ReadByte(node + 44u) == 0u && !cdata) {
        const auto end = XmlRuntimeText::ParseDecoded(
            source, node + 32u, "<", mode, runtime, true);
        return end == 0u ? 0u : end - 1u;
    }
    runtime.WriteByte(node + 44u, 1u);
    if (!cdata) {
        if (parent != 0u)
            (void)XmlDocumentError::SetWithPosition(
                parent, 15u, source, position, mode, runtime);
        return 0u;
    }

    const auto body = source + 9u;
    auto end = body;
    while (runtime.ReadByte(end) != 0u &&
           !XmlRuntimeText::HasPrefix(end, "]]>", false, runtime))
        ++end;
    std::string raw;
    raw.reserve(end - body);
    for (auto current = body; current < end; ++current)
        raw.push_back(static_cast<char>(runtime.ReadByte(current)));
    if (!XmlRuntimeText::AssignDecoded(node + 32u, raw, runtime))
        return 0u;
    return end + 3u;
}

}  // namespace pal4
