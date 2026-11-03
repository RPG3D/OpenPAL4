#include "pal4/xml_declaration_parser.h"

#include "pal4/xml_attribute_parser.h"
#include "pal4/xml_document_error.h"
#include "pal4/xml_parser_document.h"
#include "pal4/xml_runtime_text.h"

#include <cctype>

namespace pal4 {

namespace {

std::uint32_t CopyAttributeValue(
    const std::uint32_t node, const std::uint32_t source,
    const std::uint32_t destination, std::array<std::uint32_t, 4>* position,
    const int mode, IXmlNodeFactoryRuntime& runtime) {
    const auto attribute = runtime.Allocate(36u);
    if (attribute == 0u) return 0u;
    (void)XmlAttributeLifecycle::Initialize(attribute, runtime);
    try {
        const auto next = XmlAttributeParser::Parse(
            attribute, source, position, mode, runtime);
        const auto value = runtime.ReadWord(attribute + 24u) + 8u;
        (void)runtime.AssignString(node + destination, value,
                                   runtime.StringLength(value));
        (void)XmlAttributeLifecycle::Cleanup(attribute, runtime);
        (void)runtime.Free(attribute);
        return next;
    } catch (...) {
        (void)XmlAttributeLifecycle::Cleanup(attribute, runtime);
        (void)runtime.Free(attribute);
        throw;
    }
}

}  // namespace

std::uint32_t XmlDeclarationParser::Parse(
    const std::uint32_t node, const std::uint32_t source,
    std::array<std::uint32_t, 4>* position, const int mode,
    IXmlNodeFactoryRuntime& runtime) {
    const auto text = XmlRuntimeText::SkipWhitespaceAndBom(source, mode, runtime);
    const auto parent = XmlNode::GetParent(node, runtime);
    if (!XmlRuntimeText::HasPrefix(text, "<?xml", true, runtime)) {
        if (parent != 0u)
            (void)XmlDocumentError::SetWithPosition(
                parent, 12u, 0u, nullptr, mode, runtime);
        return 0u;
    }
    if (position != nullptr) {
        XmlRuntimeText::Advance(*position, text, mode, runtime);
        runtime.WriteWord(node + 4u, (*position)[0]);
        runtime.WriteWord(node + 8u, (*position)[1]);
    }
    for (const auto offset : {44u, 48u, 52u})
        (void)runtime.AssignString(node + offset, 0x8dc780u, 0u);
    auto current = text + 5u;
    while (runtime.ReadByte(current) != 0u) {
        if (runtime.ReadByte(current) == '>') return current + 1u;
        current = XmlRuntimeText::SkipWhitespaceAndBom(current, mode, runtime);
        if (current == 0u || runtime.ReadByte(current) == 0u) return 0u;
        if (runtime.ReadByte(current) == '>') return current + 1u;
        std::uint32_t destination = 0u;
        if (XmlRuntimeText::HasPrefix(current, "version", true, runtime))
            destination = 44u;
        else if (XmlRuntimeText::HasPrefix(current, "encoding", true, runtime))
            destination = 48u;
        else if (XmlRuntimeText::HasPrefix(current, "standalone", true, runtime))
            destination = 52u;
        if (destination != 0u) {
            current = CopyAttributeValue(
                node, current, destination, position, mode, runtime);
            if (current == 0u) return 0u;
            continue;
        }
        while (runtime.ReadByte(current) != 0u &&
               runtime.ReadByte(current) != '>' &&
               std::isspace(runtime.ReadByte(current)) == 0)
            ++current;
    }
    return 0u;
}

}  // namespace pal4
