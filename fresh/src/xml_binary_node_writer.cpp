#include "pal4/xml_binary_node_writer.h"

#include <bit>
#include <limits>
#include <stdexcept>

namespace pal4 {
namespace {

std::string ReadCString(const std::uint32_t address,
                        IXmlDocumentRecordRuntime& runtime) {
    if (address == 0u) return {};
    std::string text;
    for (std::uint32_t index = 0u;; ++index) {
        const auto character = runtime.ReadByte(address + index);
        if (character == 0u) return text;
        text.push_back(static_cast<char>(character));
    }
}

std::string ReadCString(const std::uint32_t address,
                        IXmlAttributeRecordRuntime& runtime) {
    if (address == 0u) return {};
    std::string text;
    for (std::uint32_t index = 0u;; ++index) {
        const auto character = runtime.ReadByte(address + index);
        if (character == 0u) return text;
        text.push_back(static_cast<char>(character));
    }
}

void WriteWord(const std::uint32_t stream, const std::uint32_t word,
               IXmlBinaryNodeWriteRuntime& runtime) {
    const std::uint8_t bytes[4]{
        static_cast<std::uint8_t>(word),
        static_cast<std::uint8_t>(word >> 8u),
        static_cast<std::uint8_t>(word >> 16u),
        static_cast<std::uint8_t>(word >> 24u)};
    runtime.Write(stream, bytes, sizeof(bytes));
}

std::uint32_t CheckedLength(const std::size_t size) {
    if (size > static_cast<std::size_t>(std::numeric_limits<std::int32_t>::max()))
        throw std::length_error("XML binary field is too long");
    return static_cast<std::uint32_t>(size);
}

void WriteString(const std::uint32_t stream, const std::string& text,
                 const bool skip_empty, IXmlBinaryNodeWriteRuntime& runtime) {
    const auto length = CheckedLength(text.size());
    WriteWord(stream, length, runtime);
    if (!skip_empty || length != 0u)
        runtime.Write(stream, text.data(), length);
}

}  // namespace

std::uint8_t XmlBinaryNodeWriter::WriteConfig(
    const std::uint32_t root, const std::uint32_t stream,
    IXmlBinaryNodeWriteRuntime& runtime) {
    if (root == 0u || stream == 0u) return 0u;
    WriteWord(stream, 1u, runtime);
    WriteWord(stream, 1u, runtime);
    return WriteNode(root, stream, runtime);
}

std::uint8_t XmlBinaryNodeWriter::WriteNode(
    const std::uint32_t node, const std::uint32_t stream,
    IXmlBinaryNodeWriteRuntime& runtime) {
    WriteString(stream, runtime.NodeName(node), true, runtime);

    const auto attribute_count = runtime.AttributeCount(node);
    WriteWord(stream, attribute_count > 0
                          ? static_cast<std::uint32_t>(attribute_count) : 0u,
              runtime);
    for (std::int32_t index = 0; index < attribute_count; ++index) {
        const auto attribute = runtime.AttributeAt(node, index);
        const auto type = runtime.AttributeType(attribute);
        WriteWord(stream, type, runtime);
        WriteString(stream, runtime.AttributeName(attribute), false, runtime);
        switch (type) {
        case 0u: {
            const auto value = runtime.AttributeChar(attribute);
            runtime.Write(stream, &value, 1u);
            break;
        }
        case 1u:
            WriteWord(stream, static_cast<std::uint32_t>(
                                  runtime.AttributeInt(attribute)), runtime);
            break;
        case 2u: {
            const auto narrowed = static_cast<float>(
                runtime.AttributeFloat(attribute));
            WriteWord(stream, std::bit_cast<std::uint32_t>(narrowed), runtime);
            break;
        }
        case 3u:
            WriteString(stream, runtime.AttributeString(attribute), true,
                        runtime);
            break;
        case 4u: {
            const auto bytes = runtime.AttributeRaw(attribute);
            const auto length = CheckedLength(bytes.size());
            WriteWord(stream, length, runtime);
            if (length != 0u) runtime.Write(stream, bytes.data(), length);
            break;
        }
        default:
            break;
        }
    }

    const auto child_count = runtime.ChildCount(node);
    WriteWord(stream, child_count > 0
                          ? static_cast<std::uint32_t>(child_count) : 0u,
              runtime);
    for (std::int32_t index = 0; index < child_count; ++index) {
        if (WriteNode(runtime.ChildAt(node, index), stream, runtime) == 0u)
            return 0u;
    }
    return 1u;
}

XmlBinaryNodeRecordAdapter::XmlBinaryNodeRecordAdapter(
    IXmlBinaryOutput& output, IXmlDocumentRecordRuntime& documents,
    IXmlAttributeRecordRuntime& attributes)
    : output_(output), documents_(documents), attributes_(attributes) {}

void XmlBinaryNodeRecordAdapter::Write(const std::uint32_t stream,
                                        const void* data,
                                        const std::uint32_t size) {
    output_.Write(stream, data, size);
}

std::string XmlBinaryNodeRecordAdapter::NodeName(const std::uint32_t node) {
    return ReadCString(documents_.ReadWord(node + 4u), documents_);
}

std::int32_t XmlBinaryNodeRecordAdapter::AttributeCount(
    const std::uint32_t node) {
    return static_cast<std::int32_t>(documents_.ReadWord(node + 52u));
}

std::uint32_t XmlBinaryNodeRecordAdapter::AttributeAt(
    const std::uint32_t node, const std::int32_t index) {
    const auto count = AttributeCount(node);
    if (index < 0 || index >= count) return 0u;
    const auto head = documents_.ReadWord(node + 24u);
    auto current = documents_.ReadWord(head);
    for (std::int32_t step = 0; step < index; ++step)
        current = documents_.GetNextNode(current);
    return current + 28u;
}

std::uint32_t XmlBinaryNodeRecordAdapter::AttributeType(
    const std::uint32_t attribute) {
    return attributes_.ReadWord(attribute + 16u);
}

std::string XmlBinaryNodeRecordAdapter::AttributeName(
    const std::uint32_t attribute) {
    return ReadCString(attributes_.ReadWord(attribute + 4u), attributes_);
}

std::uint8_t XmlBinaryNodeRecordAdapter::AttributeChar(
    const std::uint32_t attribute) {
    return attributes_.ReadByte(attribute + 24u);
}

std::int32_t XmlBinaryNodeRecordAdapter::AttributeInt(
    const std::uint32_t attribute) {
    return static_cast<std::int32_t>(attributes_.ReadWord(attribute + 24u));
}

double XmlBinaryNodeRecordAdapter::AttributeFloat(
    const std::uint32_t attribute) {
    return static_cast<double>(std::bit_cast<float>(
        attributes_.ReadWord(attribute + 24u)));
}

std::string XmlBinaryNodeRecordAdapter::AttributeString(
    const std::uint32_t attribute) {
    const auto child = attributes_.ReadWord(attribute + 24u);
    return ReadCString(attributes_.ReadWord(child + 4u), attributes_);
}

std::vector<std::uint8_t> XmlBinaryNodeRecordAdapter::AttributeRaw(
    const std::uint32_t attribute) {
    const auto size = XmlAttributeRecord::ValueSize(attribute, attributes_);
    if (static_cast<std::int32_t>(size) <= 0) return {};
    const auto address = attributes_.ReadWord(attribute + 24u);
    std::vector<std::uint8_t> bytes;
    bytes.reserve(size);
    for (std::uint32_t index = 0u; index < size; ++index)
        bytes.push_back(attributes_.ReadByte(address + index));
    return bytes;
}

std::int32_t XmlBinaryNodeRecordAdapter::ChildCount(
    const std::uint32_t node) {
    return static_cast<std::int32_t>(documents_.ReadWord(node + 56u));
}

std::uint32_t XmlBinaryNodeRecordAdapter::ChildAt(
    const std::uint32_t node, const std::int32_t index) {
    if (index < 0 || index >= ChildCount(node)) return 0u;
    const auto children = documents_.ReadWord(node + 40u);
    return documents_.ReadWord(children + static_cast<std::uint32_t>(index) * 4u);
}

}  // namespace pal4
