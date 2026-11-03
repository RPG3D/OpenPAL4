#include "pal4/xml_element_parser.h"

#include "pal4/xml_document_error.h"
#include "pal4/xml_content_parser.h"
#include "pal4/xml_attribute_parser.h"
#include "pal4/xml_name_parser.h"
#include "pal4/xml_parser_document.h"
#include "pal4/xml_runtime_text.h"

#include <string>

namespace pal4 {

namespace {

void ReportError(const std::uint32_t parent, const std::uint32_t code,
                 const std::uint32_t end,
                 std::array<std::uint32_t, 4>* position,
                 const int mode, IXmlElementRuntime& runtime) {
    if (parent != 0u)
        (void)XmlDocumentError::SetWithPosition(
            parent, code, end, position, mode, runtime);
}

void DestroyAttribute(const std::uint32_t attribute,
                      IXmlElementRuntime& runtime) {
    (void)XmlAttributeLifecycle::Cleanup(attribute, runtime);
    (void)runtime.Free(attribute);
}

}  // namespace

std::uint32_t XmlElementParser::Parse(
    const std::uint32_t element, const std::uint32_t source,
    std::array<std::uint32_t, 4>* position, const int mode,
    IXmlElementRuntime& runtime) {
    const auto text = XmlRuntimeText::SkipWhitespaceAndBom(source, mode, runtime);
    const auto parent = XmlNode::GetParent(element, runtime);
    if (text == 0u || runtime.ReadByte(text) == 0u) {
        ReportError(parent, 4u, 0u, nullptr, mode, runtime);
        return 0u;
    }
    if (position != nullptr) {
        XmlRuntimeText::Advance(*position, text, mode, runtime);
        runtime.WriteWord(element + 4u, (*position)[0]);
        runtime.WriteWord(element + 8u, (*position)[1]);
    }
    if (runtime.ReadByte(text) != '<') {
        ReportError(parent, 4u, text, position, mode, runtime);
        return 0u;
    }
    const auto name_start = XmlRuntimeText::SkipWhitespaceAndBom(
        text + 1u, mode, runtime);
    const auto name_end = XmlNameParser::ReadRuntime(
        name_start, element + 32u, mode, runtime);
    if (name_end == 0u || runtime.ReadByte(name_end) == 0u) {
        ReportError(parent, 5u, name_start, position, mode, runtime);
        return 0u;
    }
    std::string closing = "</";
    for (auto address = name_start; address < name_end; ++address)
        closing.push_back(static_cast<char>(runtime.ReadByte(address)));
    closing.push_back('>');

    auto current = name_end;
    for (;;) {
        if (runtime.ReadByte(current) == 0u) return current;
        const auto token = XmlRuntimeText::SkipWhitespaceAndBom(
            current, mode, runtime);
        if (token == 0u || runtime.ReadByte(token) == 0u) {
            ReportError(parent, 7u, current, position, mode, runtime);
            return 0u;
        }
        if (runtime.ReadByte(token) == '/') {
            if (runtime.ReadByte(token + 1u) == '>') return token + 2u;
            ReportError(parent, 8u, token + 1u, position, mode, runtime);
            return 0u;
        }
        if (runtime.ReadByte(token) == '>') {
            const auto end = XmlContentParser::Parse(
                element, token + 1u, position, mode, runtime);
            if (end == 0u || runtime.ReadByte(end) == 0u) return 0u;
            if (XmlRuntimeText::HasPrefix(
                    end, closing.c_str(), false, runtime))
                return end + static_cast<std::uint32_t>(closing.size());
            ReportError(parent, 9u, end, position, mode, runtime);
            return 0u;
        }

        const auto attribute = runtime.Allocate(36u);
        if (attribute == 0u) {
            ReportError(parent, 3u, current, position, mode, runtime);
            return 0u;
        }
        (void)XmlAttributeLifecycle::Initialize(attribute, runtime);
        runtime.WriteWord(attribute + 16u, parent);
        const auto next = XmlAttributeParser::Parse(
            attribute, token, position, mode, runtime);
        if (next == 0u || runtime.ReadByte(next) == 0u) {
            ReportError(parent, 4u, token, position, mode, runtime);
            DestroyAttribute(attribute, runtime);
            return 0u;
        }
        const auto name_block = runtime.ReadWord(attribute + 20u);
        const auto existing = XmlAttributeRoot::FindByName(
            element + 44u, name_block + 8u, runtime);
        if (existing != 0u) {
            const auto value_block = runtime.ReadWord(attribute + 24u);
            const auto value = value_block + 8u;
            (void)runtime.AssignString(existing + 24u, value,
                                       runtime.StringLength(value));
            DestroyAttribute(attribute, runtime);
            return 0u;
        }
        (void)XmlAttributeRoot::AddChild(element + 44u, attribute, runtime);
        current = next;
    }
}

}  // namespace pal4
