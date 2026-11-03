#include "pal4/xml_node_attribute_writer.h"

namespace pal4 {
namespace {

enum class ValueType { Int, Float, String };

void ReleaseKey(std::uint32_t data, IStringAssignRuntime& strings) {
    if (data == 0u)
        return;
    const auto header = data - 1u;
    const auto reference = strings.ReadByte(header);
    if (reference == 0u || reference == 0xffu)
        strings.Free(header);
    else
        strings.WriteByte(header,
            static_cast<std::uint8_t>(reference - 1u));
}

std::uint8_t Write(std::uint32_t document_address,
                   std::uint32_t key_address, std::uint32_t value_address,
                   IXmlNodeAttributeWriterRuntime& runtime,
                   ValueType type) {
    auto& memory = runtime.Memory();
    auto& strings = runtime.Strings();
    const auto length = memory.StringLength(key_address);
    StringBuffer buffer;
    ConfigStringState key{};
    key.flag = static_cast<std::uint8_t>(value_address);
    XmlAttributeWriterTemporary attribute{};
    bool key_ready = false;
    bool attribute_ready = false;

    try {
        if (buffer.AllocateText(length, 1u, strings)) {
            key.data = buffer.State().data;
            key.capacity = buffer.State().capacity;
            key_ready = true;
            strings.Copy(key.data, key_address, length);
            key.size = length;
            strings.WriteByte(key.data + length, 0u);
        }
        key_ready = true;

        const auto tree_address = document_address + 0x14u;
        const auto header = memory.ReadDword(tree_address + 4u);
        auto candidate = ConfigValueLookup::FindConfigItem(
            tree_address, key, memory);
        if (candidate != header &&
            ConfigValueLookup::CompareText(key, candidate + 0x0cu,
                                           memory) < 0)
            candidate = header;

        ReleaseKey(key.data, strings);
        key_ready = false;
        if (candidate != header && candidate + 0x1cu != 0u) {
            switch (type) {
            case ValueType::Int:
                return runtime.SetInt(candidate + 0x1cu, value_address);
            case ValueType::Float:
                return runtime.SetFloat(candidate + 0x1cu, value_address);
            case ValueType::String:
                return runtime.SetString(candidate + 0x1cu, value_address);
            }
        }

        std::uint32_t new_attribute = 0u;
        switch (type) {
        case ValueType::Int:
            new_attribute = runtime.InitializeIntAttribute(
                attribute, key_address, value_address);
            break;
        case ValueType::Float:
            new_attribute = runtime.InitializeFloatAttribute(
                attribute, key_address, value_address);
            break;
        case ValueType::String:
            new_attribute = runtime.InitializeStringAttribute(
                attribute, key_address, value_address);
            break;
        }
        attribute_ready = true;
        const auto result = runtime.AddAttribute(
            document_address, new_attribute);
        runtime.DestroyAttribute(attribute);
        attribute_ready = false;
        return result;
    } catch (...) {
        if (attribute_ready)
            runtime.DestroyAttribute(attribute);
        if (key_ready)
            ReleaseKey(key.data, strings);
        throw;
    }
}

}  // namespace

std::uint8_t XmlNodeAttributeWriter::WriteInt(
    const std::uint32_t document_address,
    const std::uint32_t key_address,
    const std::uint32_t value_address,
    IXmlNodeAttributeWriterRuntime& runtime) {
    return Write(document_address, key_address, value_address,
                 runtime, ValueType::Int);
}

std::uint8_t XmlNodeAttributeWriter::WriteFloat(
    const std::uint32_t document_address,
    const std::uint32_t key_address,
    const std::uint32_t value_address,
    IXmlNodeAttributeWriterRuntime& runtime) {
    return Write(document_address, key_address, value_address,
                 runtime, ValueType::Float);
}

std::uint8_t XmlNodeAttributeWriter::WriteString(
    const std::uint32_t document_address,
    const std::uint32_t key_address,
    const std::uint32_t value_address,
    IXmlNodeAttributeWriterRuntime& runtime) {
    return Write(document_address, key_address, value_address,
                 runtime, ValueType::String);
}

}  // namespace pal4
