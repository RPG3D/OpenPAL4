#include "pal4/xml_serializer.h"

#include "pal4/compact_string_buffer.h"
#include "pal4/xml_escaper.h"

namespace pal4 {

std::uint32_t XmlSerializationRuntimeStub::ReadWord(
    std::uint32_t) { return 0u; }
std::uint8_t XmlSerializationRuntimeStub::ReadByte(
    std::uint32_t) { return 0u; }
std::string XmlSerializationRuntimeStub::ReadText(
    std::uint32_t) { return {}; }
std::uint32_t XmlSerializationRuntimeStub::StoreTemporary(
    std::string_view) { return CompactStringBuffer::kEmptyBlock; }
std::uint32_t XmlSerializationRuntimeStub::FreeTemporary(
    std::uint32_t) { return 0u; }
int XmlSerializationRuntimeStub::WriteVirtual(
    std::uint32_t, std::FILE*, int, std::uint32_t) { return 0; }
std::uint32_t XmlSerializationRuntimeStub::NextSibling(
    std::uint32_t) { return 0u; }
bool XmlSerializationRuntimeStub::IsInline(
    std::uint32_t, std::uint32_t) { return false; }

namespace {

void WriteIndent(std::FILE* stream, const int depth) {
    for (int index = 0; index < depth; ++index)
        (void)std::fprintf(stream, "    ");
}

}  // namespace

std::uint32_t XmlSerializer::WriteAttribute(
    const std::uint32_t attribute, std::FILE* stream, const int depth,
    IXmlSerializationRuntime& runtime) {
    return WriteAttributeRecord(attribute, stream, depth, nullptr, runtime);
}

std::uint32_t XmlSerializer::WriteAttributeRecord(
    const std::uint32_t attribute, std::FILE* stream,
    const int depth, std::string* appended,
    IXmlSerializationRuntime& runtime) {
    (void)depth;
    const auto name_block = runtime.ReadWord(attribute + 20u);
    const auto value_block = runtime.ReadWord(attribute + 24u);
    const auto raw_name = runtime.ReadText(name_block + 8u);
    const auto raw_value = runtime.ReadText(value_block + 8u);
    std::string name;
    std::string value;
    XmlEscaper::Append(name, raw_name);
    XmlEscaper::Append(value, raw_value);

    const auto name_temp = runtime.StoreTemporary(name);
    std::uint32_t value_temp = CompactStringBuffer::kEmptyBlock;
    try {
        value_temp = runtime.StoreTemporary(value);
        const char quote = raw_value.find('"') == std::string::npos ? '"' : '\'';
        if (stream != nullptr)
            (void)std::fprintf(stream, "%s=%c%s%c", name.c_str(),
                               quote, value.c_str(), quote);
        if (appended != nullptr) {
            appended->append(name);
            appended->push_back('=');
            appended->push_back(quote);
            appended->append(value);
            appended->push_back(quote);
        }
    } catch (...) {
        if (value_temp != CompactStringBuffer::kEmptyBlock)
            (void)runtime.FreeTemporary(value_temp);
        if (name_temp != CompactStringBuffer::kEmptyBlock)
            (void)runtime.FreeTemporary(name_temp);
        throw;
    }
    if (value_temp != CompactStringBuffer::kEmptyBlock)
        (void)runtime.FreeTemporary(value_temp);
    return name_temp == CompactStringBuffer::kEmptyBlock ? name_temp :
           runtime.FreeTemporary(name_temp);
}

int XmlSerializer::WriteNode(
    const std::uint32_t node, std::FILE* stream,
    const int depth, IXmlSerializationRuntime& runtime) {
    WriteIndent(stream, depth);
    const auto block = runtime.ReadWord(node + 32u);
    const auto name = runtime.ReadText(block + 8u);
    (void)std::fprintf(stream, "<%s", name.c_str());

    const auto root = node + 44u;
    auto attribute = runtime.ReadWord(node + 76u);
    if (attribute != root && attribute != 0u) {
        do {
            (void)std::fprintf(stream, " ");
            (void)runtime.WriteVirtual(attribute, stream, depth, 4u);
            attribute = runtime.NextSibling(attribute);
        } while (attribute != 0u);
    }

    const auto first = runtime.ReadWord(node + 24u);
    if (first == 0u) return std::fprintf(stream, " />");

    if (first == runtime.ReadWord(node + 28u) &&
        runtime.IsInline(first, 44u)) {
        (void)std::fprintf(stream, ">");
        (void)runtime.WriteVirtual(first, stream, depth + 1, 4u);
    } else {
        (void)std::fprintf(stream, ">");
        auto child = first;
        while (child != 0u) {
            if (!runtime.IsInline(child, 44u))
                (void)std::fprintf(stream, "\n");
            (void)runtime.WriteVirtual(child, stream, depth + 1, 4u);
            child = runtime.ReadWord(child + 40u);
        }
        (void)std::fprintf(stream, "\n");
        WriteIndent(stream, depth);
    }
    return std::fprintf(stream, "</%s>", name.c_str());
}

int XmlSerializer::WriteChildren(
    const std::uint32_t node, std::FILE* stream,
    const int depth, IXmlSerializationRuntime& runtime) {
    auto child = runtime.ReadWord(node + 24u);
    int result = 0;
    while (child != 0u) {
        (void)runtime.WriteVirtual(child, stream, depth, 4u);
        result = std::fprintf(stream, "\n");
        child = runtime.ReadWord(child + 40u);
    }
    return result;
}

int XmlSerializer::WriteCData(
    const std::uint32_t node, std::FILE* stream, const int depth,
    IXmlSerializationRuntime& runtime) {
    if (runtime.ReadByte(node + 44u) != 0u) {
        (void)std::fprintf(stream, "\n");
        WriteIndent(stream, depth);
        const auto block = runtime.ReadWord(node + 32u);
        const auto text = runtime.ReadText(block + 8u);
        return std::fprintf(stream, "<![CDATA[%s]]>\n", text.c_str());
    }
    const auto block = runtime.ReadWord(node + 32u);
    const auto raw_text = runtime.ReadText(block + 8u);
    std::string escaped;
    XmlEscaper::Append(escaped, raw_text);
    const auto temporary = runtime.StoreTemporary(escaped);
    (void)std::fprintf(stream, "%s", escaped.c_str());
    return static_cast<int>(
        temporary == CompactStringBuffer::kEmptyBlock ? temporary :
        runtime.FreeTemporary(temporary));
}

int XmlSerializer::WriteOpenTag(
    const std::uint32_t node, std::FILE* stream, const int depth,
    IXmlSerializationRuntime& runtime) {
    WriteIndent(stream, depth);
    const auto block = runtime.ReadWord(node + 32u);
    const auto name = runtime.ReadText(block + 8u);
    return std::fprintf(stream, "<%s>", name.c_str());
}

}  // namespace pal4
