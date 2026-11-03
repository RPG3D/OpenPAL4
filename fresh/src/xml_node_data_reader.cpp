#include "pal4/xml_node_data_reader.h"

#include "xml_data_scoped.h"

#include <limits>
#include <optional>
#include <stdexcept>
#include <vector>

namespace pal4 {
namespace {

using detail::XmlAttribute;
using detail::XmlTemporary;

std::uint32_t BorrowedAddress(const void* storage) {
    const auto address = reinterpret_cast<std::uintptr_t>(storage);
    if (address > std::numeric_limits<std::uint32_t>::max())
        throw std::logic_error("XML record adapter requires local 32-bit storage");
    return static_cast<std::uint32_t>(address);
}

void ReadAttributeValue(const std::uint32_t type,
                        const std::uint32_t attribute,
                        IXmlNodeDataRuntime& runtime) {
    switch (type) {
    case 0u:
        runtime.SetChar(attribute, runtime.ReadByte());
        break;
    case 1u:
        runtime.SetInt(attribute, runtime.ReadInt32());
        break;
    case 2u:
        runtime.SetFloatBits(attribute,
                             static_cast<std::uint32_t>(runtime.ReadInt32()));
        break;
    case 3u: {
        const auto length = runtime.ReadInt32();
        if (length <= 0) {
            runtime.SetString(attribute, 0u);
        } else {
            XmlTemporary text(runtime, static_cast<std::uint32_t>(length) + 1u);
            runtime.ReadBytes(text.Address(), static_cast<std::uint32_t>(length));
            runtime.WriteByte(text.Address() + static_cast<std::uint32_t>(length), 0u);
            runtime.SetString(attribute, text.Address());
        }
        break;
    }
    case 4u: {
        const auto length = runtime.ReadInt32();
        if (length > 0) {
            XmlTemporary data(runtime, static_cast<std::uint32_t>(length));
            runtime.ReadBytes(data.Address(), static_cast<std::uint32_t>(length));
            runtime.SetRaw(attribute, data.Address(),
                           static_cast<std::uint32_t>(length));
        }
        break;
    }
    default:
        break;
    }
}

}  // namespace

std::uint8_t XmlNodeDataReader::Read(const std::uint32_t parent,
                                      const std::uint32_t node,
                                      IXmlNodeDataRuntime& runtime) {
    const auto name_length = runtime.ReadInt32();
    if (name_length > 0) {
        XmlTemporary name(runtime, static_cast<std::uint32_t>(name_length) + 1u);
        runtime.ReadBytes(name.Address(), static_cast<std::uint32_t>(name_length));
        runtime.WriteByte(name.Address() + static_cast<std::uint32_t>(name_length), 0u);
        runtime.SetName(node, name.Address(), parent);
    } else {
        runtime.SetName(node, 0u, parent);
    }

    const auto attribute_count = runtime.ReadInt32();
    for (std::int32_t index = 0; index < attribute_count; ++index) {
        const auto type = static_cast<std::uint32_t>(runtime.ReadInt32());
        const auto length = runtime.ReadInt32();
        if (length < 0 || length == std::numeric_limits<std::int32_t>::max())
            throw std::length_error("invalid XML attribute name length");
        std::optional<XmlAttribute> attribute;
        {
            XmlTemporary name(runtime, static_cast<std::uint32_t>(length) + 1u);
            runtime.ReadBytes(name.Address(), static_cast<std::uint32_t>(length));
            runtime.WriteByte(name.Address() + static_cast<std::uint32_t>(length), 0u);
            attribute.emplace(runtime, type, name.Address());
        } // IDA5F78BC frees the name before reading/setting the value.
        ReadAttributeValue(type, attribute->Address(), runtime);
        runtime.AddAttribute(node, attribute->Address());
    }

    const auto child_count = runtime.ReadInt32();
    for (std::int32_t index = 0; index < child_count; ++index) {
        const auto child = runtime.CreateNode();
        if (child == 0u) throw std::bad_alloc{};
        try {(void)Read(node, child, runtime);}
        catch (...) {runtime.DestroyNode(child);throw;}
    }
    return 1u;
}

std::uint8_t XmlNodeDataReader::ReadVirtualStream(
    const std::uint32_t parent, const std::uint32_t node,
    IXmlNodeDataRuntime& runtime) {
    return Read(parent, node, runtime);
}

XmlNodeDataRecordAdapter::XmlNodeDataRecordAdapter(
    IXmlNodeDataInput& input, XmlDocumentTreeGlobals& globals,
    IXmlDocumentRecordRuntime& documents, IXmlAttributeRecordRuntime& attributes)
    : input_(input), globals_(globals), documents_(documents),
      attributes_(attributes) {}

std::int32_t XmlNodeDataRecordAdapter::ReadInt32() {
    std::uint8_t bytes[4]{};
    input_.Read(bytes, sizeof(bytes));
    const auto value = static_cast<std::uint32_t>(bytes[0]) |
                       (static_cast<std::uint32_t>(bytes[1]) << 8u) |
                       (static_cast<std::uint32_t>(bytes[2]) << 16u) |
                       (static_cast<std::uint32_t>(bytes[3]) << 24u);
    return static_cast<std::int32_t>(value);
}

std::uint8_t XmlNodeDataRecordAdapter::ReadByte() {
    std::uint8_t value = 0u;
    input_.Read(&value, sizeof(value));
    return value;
}

void XmlNodeDataRecordAdapter::ReadBytes(const std::uint32_t destination,
                                          const std::uint32_t size) {
    std::vector<std::uint8_t> bytes(size, 0u);
    input_.Read(bytes.data(), size);
    for (std::uint32_t index = 0u; index < size; ++index)
        attributes_.WriteByte(destination + index, bytes[index]);
}

std::uint32_t XmlNodeDataRecordAdapter::AllocateTemporary(const std::uint32_t size) {
    return attributes_.AllocateMemory(size);
}

void XmlNodeDataRecordAdapter::FreeTemporary(const std::uint32_t address) {
    (void)attributes_.FreeBlock(address);
}

void XmlNodeDataRecordAdapter::WriteByte(const std::uint32_t address,
                                          const std::uint8_t value) {
    attributes_.WriteByte(address, value);
}

void XmlNodeDataRecordAdapter::SetName(const std::uint32_t node,
                                        const std::uint32_t name,
                                        const std::uint32_t parent) {
    (void)XmlDocumentRecord::SetName(node, name, parent, documents_);
}

std::uint32_t XmlNodeDataRecordAdapter::InitializeAttribute(
    const std::uint32_t type, const std::uint32_t name,
    std::span<std::uint32_t, 7> storage) {
    const auto attribute = BorrowedAddress(storage.data());
    try {
        return XmlAttributeRecord::InitializeWithValue(
            attribute, type, name, attributes_);
    } catch (...) {
        (void)XmlAttributeRecord::Destroy(attribute, attributes_);
        throw;
    }
}

void XmlNodeDataRecordAdapter::SetChar(const std::uint32_t attribute,
                                        const std::uint8_t value) {
    (void)XmlAttributeRecord::SetChar(attribute, BorrowedAddress(&value), attributes_);
}

void XmlNodeDataRecordAdapter::SetInt(const std::uint32_t attribute,
                                       const std::int32_t value) {
    (void)XmlAttributeRecord::SetInt(attribute, BorrowedAddress(&value), attributes_);
}

void XmlNodeDataRecordAdapter::SetFloatBits(const std::uint32_t attribute,
                                              const std::uint32_t bits) {
    (void)XmlAttributeRecord::SetFloat(attribute, BorrowedAddress(&bits), attributes_);
}

void XmlNodeDataRecordAdapter::SetString(const std::uint32_t attribute,
                                          const std::uint32_t text) {
    if (text != 0u) {
        (void)XmlAttributeRecord::SetString(attribute, text, attributes_);
        return;
    }
    static constexpr char empty = '\0';
    (void)XmlAttributeRecord::SetString(attribute, BorrowedAddress(&empty), attributes_);
}

void XmlNodeDataRecordAdapter::SetRaw(const std::uint32_t attribute,
                                       const std::uint32_t data,
                                       const std::uint32_t size) {
    (void)XmlAttributeRecord::SetValue(attribute, data,
                                        BorrowedAddress(&size), attributes_);
}

void XmlNodeDataRecordAdapter::AddAttribute(const std::uint32_t node,
                                              const std::uint32_t attribute) {
    (void)XmlDocumentRecord::AddAttribute(node, attribute, documents_);
}

void XmlNodeDataRecordAdapter::DestroyAttribute(const std::uint32_t attribute) {
    (void)XmlAttributeRecord::Destroy(attribute, attributes_);
}

std::uint32_t XmlNodeDataRecordAdapter::CreateNode() {
    const auto node = documents_.AllocateMemory(60u);
    if (node == 0u) throw std::bad_alloc{};
    try {
        (void)XmlDocumentRecord::Initialize(node, globals_, documents_);
    } catch (...) {
        (void)documents_.FreeMemory(node);
        throw;
    }
    return node;
}
void XmlNodeDataRecordAdapter::DestroyNode(std::uint32_t node) {
    (void)XmlDocumentRecord::DestroyNode(node,globals_,documents_);
    (void)documents_.FreeMemory(node);
}

}  // namespace pal4
