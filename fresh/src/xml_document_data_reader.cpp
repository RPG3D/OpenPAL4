#include "pal4/xml_document_data_reader.h"

#include "xml_data_scoped.h"

#include <limits>
#include <stdexcept>

#include "pal4/xml_binary_codec.h"

namespace pal4 {
namespace {

class HexRuntime final : public IXmlBinaryCodecRuntime {
public:
    HexRuntime(const std::string_view text, const std::uint32_t capacity,
               IXmlNodeDataRuntime& output)
        : text_(text), capacity_(capacity), output_(output) {}
    [[nodiscard]] std::uint32_t ReadWord(std::uint32_t) override {
        return capacity_;
    }
    [[nodiscard]] std::uint8_t ReadByte(const std::uint32_t address) override {
        const auto index = address - 1u;
        return index < text_.size()
                   ? static_cast<std::uint8_t>(text_[index]) : 0u;
    }
    void WriteByte(const std::uint32_t address,
                   const std::uint8_t value) override {
        output_.WriteByte(address, value);
    }

private:
    std::string_view text_;
    std::uint32_t capacity_;
    IXmlNodeDataRuntime& output_;
};

void WriteCString(const std::string_view text,
                  const std::uint32_t destination,
                  IXmlNodeDataRuntime& output) {
    for (std::size_t index = 0u; index < text.size(); ++index)
        output.WriteByte(destination + static_cast<std::uint32_t>(index),
                         static_cast<std::uint8_t>(text[index]));
    output.WriteByte(destination + static_cast<std::uint32_t>(text.size()), 0u);
}

}  // namespace

std::uint8_t XmlDocumentDataReader::Read(
    const std::uint32_t parent, const std::uint32_t node,
    const std::uint32_t source_node, IXmlDocumentDataSource& source,
    IXmlNodeDataRuntime& output) {
    const auto name = source.GetAttribute(source_node, "Name");
    if (name.size() >= std::numeric_limits<std::uint32_t>::max())
        throw std::length_error("XML document name is too long");
    detail::XmlTemporary node_name(output,
                                    static_cast<std::uint32_t>(name.size()) + 1u);
    WriteCString(name, node_name.Address(), output);
    output.SetName(node, node_name.Address(), parent);

    for (auto entry = source.FirstChild(source_node, "Xnd_Attr"); entry != 0u;
         entry = source.NextChild(entry, "Xnd_Attr")) {
        const auto attribute_name = source.GetAttribute(entry, "Name");
        const auto type_name = source.GetAttribute(entry, "Type");
        const auto type = source.GetType(type_name);
        const auto value = source.GetAttribute(entry, "Value");
        if (attribute_name.size() >= std::numeric_limits<std::uint32_t>::max() ||
            value.size() >= std::numeric_limits<std::uint32_t>::max())
            throw std::length_error("XML document attribute is too long");

        detail::XmlTemporary name_buffer(
            output, static_cast<std::uint32_t>(attribute_name.size()) + 1u);
        WriteCString(attribute_name, name_buffer.Address(), output);
        detail::XmlAttribute attribute(output, type, name_buffer.Address());
        if (type == 4u) {
            const auto capacity = static_cast<std::uint32_t>(value.size() / 2u);
            if (capacity != 0u) {
                detail::XmlTemporary decoded(output, capacity);
                HexRuntime codec(value, capacity, output);
                (void)XmlBinaryCodec::DecodeHex(1u, decoded.Address(), 0u, codec);
                output.SetRaw(attribute.Address(), decoded.Address(), capacity);
            }
        } else {
            detail::XmlTemporary value_buffer(
                output, static_cast<std::uint32_t>(value.size()) + 1u);
            WriteCString(value, value_buffer.Address(), output);
            output.SetString(attribute.Address(), value_buffer.Address());
        }
        output.AddAttribute(node, attribute.Address());
    }

    for (auto child = source.FirstChild(source_node, "Xnd_Node"); child != 0u;
         child = source.NextChild(child, "Xnd_Node")) {
        const auto destination = output.CreateNode();
        if (destination == 0u) throw std::bad_alloc{};
        if (Read(node, destination, child, source, output) == 0u) return 0u;
    }
    return 1u;
}

}  // namespace pal4
