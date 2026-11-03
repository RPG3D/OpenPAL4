#include "pal4/xml_attribute_parser.h"

#include "pal4/xml_document_error.h"
#include "pal4/xml_name_parser.h"
#include "pal4/xml_runtime_text.h"

#include <cctype>

namespace pal4 {

namespace {

void ReportError(const std::uint32_t attribute, const std::uint32_t end,
                 std::array<std::uint32_t, 4>* position, const int mode,
                 IXmlNodeFactoryRuntime& runtime) {
    const auto parent = runtime.ReadWord(attribute + 16u);
    if (parent != 0u)
        (void)XmlDocumentError::SetWithPosition(
            parent, 7u, end, position, mode, runtime);
}

}  // namespace

std::uint32_t XmlAttributeParser::Parse(
    const std::uint32_t attribute, const std::uint32_t source,
    std::array<std::uint32_t, 4>* position, const int mode,
    IXmlNodeFactoryRuntime& runtime) {
    const auto text = XmlRuntimeText::SkipWhitespaceAndBom(source, mode, runtime);
    if (text == 0u || runtime.ReadByte(text) == 0u) return 0u;
    if (position != nullptr) {
        XmlRuntimeText::Advance(*position, text, mode, runtime);
        runtime.WriteWord(attribute + 4u, (*position)[0]);
        runtime.WriteWord(attribute + 8u, (*position)[1]);
    }
    const auto name_end = XmlNameParser::ReadRuntime(
        text, attribute + 20u, mode, runtime);
    if (name_end == 0u || runtime.ReadByte(name_end) == 0u) {
        ReportError(attribute, text, position, mode, runtime);
        return 0u;
    }
    const auto equals = XmlRuntimeText::SkipWhitespaceAndBom(
        name_end, mode, runtime);
    if (equals == 0u || runtime.ReadByte(equals) != '=') {
        ReportError(attribute, equals, position, mode, runtime);
        return 0u;
    }
    const auto value = XmlRuntimeText::SkipWhitespaceAndBom(
        equals + 1u, mode, runtime);
    if (value == 0u || runtime.ReadByte(value) == 0u) {
        ReportError(attribute, value, position, mode, runtime);
        return 0u;
    }
    const auto first = runtime.ReadByte(value);
    if (first == '\'' || first == '"') {
        const char delimiter[] = {static_cast<char>(first), '\0'};
        return XmlRuntimeText::ParseDecoded(
            value + 1u, attribute + 24u, delimiter, mode, runtime);
    }
    (void)runtime.AssignString(attribute + 24u, 0x8dc780u, 0u);
    auto end = value;
    while (runtime.ReadByte(end) != 0u) {
        const auto byte = runtime.ReadByte(end);
        if (std::isspace(byte) != 0 || byte == '/' || byte == '>') break;
        if (byte == '\'' || byte == '"') {
            ReportError(attribute, end, position, mode, runtime);
            return 0u;
        }
        ++end;
    }
    if (end != value)
        (void)runtime.AssignString(attribute + 24u, value, end - value);
    return end;
}

}  // namespace pal4
