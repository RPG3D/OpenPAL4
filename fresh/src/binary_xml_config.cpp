#include "pal4/binary_xml_config.h"

#include <bit>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <limits>
#include <optional>
#include <span>
#include <utility>

namespace pal4 {
namespace {
std::uint32_t RawBits(std::span<const std::uint8_t> bytes,std::uint32_t fallback){
    if(bytes.size()>=4)fallback=0;
    for(std::size_t i=0;i<bytes.size() && i<4;++i){const auto shift=static_cast<unsigned>(i*8);fallback=(fallback&~(0xffu<<shift))|(std::uint32_t(bytes[i])<<shift);}
    return fallback;
}
}

float BinaryXmlAttribute::ToFloat(const float fallback) const {
    if (const auto* number = std::get_if<float>(&value)) return *number;
    if (const auto* integer = std::get_if<std::int32_t>(&value))
        return static_cast<float>(*integer);
    if (const auto* character = std::get_if<std::int8_t>(&value))
        return static_cast<float>(*character);
    if (const auto* text = std::get_if<std::string>(&value)) {
        char* end = nullptr;
        const auto parsed = std::strtof(text->c_str(), &end);
        return end != text->c_str() ? parsed : fallback;
    }
    if (const auto* bytes = std::get_if<std::vector<std::uint8_t>>(&value))
        return std::bit_cast<float>(RawBits(*bytes,std::bit_cast<std::uint32_t>(fallback)));
    return fallback;
}

namespace {

class Reader final {
public:
    explicit Reader(std::span<const std::uint8_t> bytes) : bytes_(bytes) {}

    [[nodiscard]] std::size_t Remaining() const { return bytes_.size() - offset_; }
    [[nodiscard]] std::size_t Position() const { return offset_; }

    bool ReadWord(std::uint32_t& value) {
        if (Remaining() < 4u) return false;
        value = static_cast<std::uint32_t>(bytes_[offset_]) |
                (static_cast<std::uint32_t>(bytes_[offset_ + 1u]) << 8u) |
                (static_cast<std::uint32_t>(bytes_[offset_ + 2u]) << 16u) |
                (static_cast<std::uint32_t>(bytes_[offset_ + 3u]) << 24u);
        offset_ += 4u;
        return true;
    }

    bool ReadByte(std::uint8_t& value) {
        if (Remaining() == 0u) return false;
        value = bytes_[offset_++];
        return true;
    }

    bool ReadString(const std::uint32_t size, std::string& value) {
        if (size > Remaining()) return false;
        value.assign(reinterpret_cast<const char*>(bytes_.data() + offset_),
                     size);
        offset_ += size;
        return true;
    }

    bool ReadBytes(const std::uint32_t size,
                   std::vector<std::uint8_t>& value) {
        if (size > Remaining()) return false;
        value.assign(bytes_.begin() + offset_, bytes_.begin() + offset_ + size);
        offset_ += size;
        return true;
    }

    bool ReadNode(BinaryXmlNode& node, const std::uint32_t depth) {
        if (depth > 64u) return false;
        std::uint32_t name_size = 0u;
        std::uint32_t attribute_count = 0u;
        if (!ReadWord(name_size) || !ReadString(name_size, node.name) ||
            !ReadWord(attribute_count) || attribute_count > Remaining() / 8u) {
            return false;
        }
        node.attributes.reserve(attribute_count);
        for (std::uint32_t index = 0u; index < attribute_count; ++index) {
            BinaryXmlAttribute attribute;
            std::uint32_t name_length = 0u;
            if (!ReadWord(attribute.type) || !ReadWord(name_length) ||
                !ReadString(name_length, attribute.name)) {
                return false;
            }
            std::uint32_t raw = 0u;
            switch (attribute.type) {
            case 0u: {
                std::uint8_t value = 0u;
                if (!ReadByte(value)) return false;
                attribute.value = static_cast<std::int8_t>(value);
                break;
            }
            case 1u:
                if (!ReadWord(raw)) return false;
                attribute.value = static_cast<std::int32_t>(raw);
                break;
            case 2u:
                if (!ReadWord(raw)) return false;
                attribute.value = std::bit_cast<float>(raw);
                break;
            case 3u: {
                std::uint32_t size = 0u;
                std::string value;
                if (!ReadWord(size) || !ReadString(size, value)) return false;
                attribute.value = std::move(value);
                break;
            }
            case 4u: {
                std::uint32_t size = 0u;
                std::vector<std::uint8_t> value;
                if (!ReadWord(size) || !ReadBytes(size, value)) return false;
                attribute.value = std::move(value);
                break;
            }
            default:
                break;
            }
            node.attributes.push_back(std::move(attribute));
        }

        std::uint32_t child_count = 0u;
        if (!ReadWord(child_count) || child_count > Remaining() / 12u) {
            return false;
        }
        node.children.reserve(child_count);
        for (std::uint32_t index = 0u; index < child_count; ++index) {
            BinaryXmlNode child;
            if (!ReadNode(child, depth + 1u)) return false;
            node.children.push_back(std::move(child));
        }
        return true;
    }

private:
    std::span<const std::uint8_t> bytes_;
    std::size_t offset_ = 0u;
};

class Writer final {
public:
    bool WriteNode(const BinaryXmlNode& node, std::uint32_t depth) {
        if (depth > 64u || !WriteString(node.name) ||
            !WriteCount(node.attributes.size()))
            return false;
        for (const auto& attribute : node.attributes) {
            if (!WriteWord(attribute.type) || !WriteString(attribute.name))
                return false;
            switch (attribute.type) {
            case 0u: {
                const auto* value = std::get_if<std::int8_t>(&attribute.value);
                if (!value) return false;
                bytes_.push_back(static_cast<std::uint8_t>(*value));
                break;
            }
            case 1u: {
                const auto* value = std::get_if<std::int32_t>(&attribute.value);
                if (!value || !WriteWord(static_cast<std::uint32_t>(*value)))
                    return false;
                break;
            }
            case 2u: {
                const auto* value = std::get_if<float>(&attribute.value);
                if (!value || !WriteWord(std::bit_cast<std::uint32_t>(*value)))
                    return false;
                break;
            }
            case 3u: {
                const auto* value = std::get_if<std::string>(&attribute.value);
                if (!value || !WriteString(*value)) return false;
                break;
            }
            case 4u: {
                const auto* value =
                    std::get_if<std::vector<std::uint8_t>>(&attribute.value);
                if (!value || !WriteCount(value->size())) return false;
                bytes_.insert(bytes_.end(), value->begin(), value->end());
                break;
            }
            default:
                return false;
            }
        }
        if (!WriteCount(node.children.size())) return false;
        for (const auto& child : node.children)
            if (!WriteNode(child, depth + 1u)) return false;
        return true;
    }

    bool WriteWord(std::uint32_t value) {
        for (unsigned shift = 0u; shift < 32u; shift += 8u)
            bytes_.push_back(static_cast<std::uint8_t>(value >> shift));
        return true;
    }

    [[nodiscard]] std::vector<std::uint8_t> Take() { return std::move(bytes_); }

private:
    bool WriteCount(std::size_t count) {
        if (count > std::numeric_limits<std::uint32_t>::max()) return false;
        return WriteWord(static_cast<std::uint32_t>(count));
    }

    bool WriteString(std::string_view value) {
        if (!WriteCount(value.size())) return false;
        bytes_.insert(bytes_.end(), value.begin(), value.end());
        return true;
    }

    std::vector<std::uint8_t> bytes_;
};

}  // namespace

std::uint32_t BinaryXmlAttribute::ToInt(
    const std::uint32_t fallback) const {
    if (const auto* character = std::get_if<std::int8_t>(&value)) {
        return static_cast<std::uint32_t>(static_cast<std::int32_t>(*character));
    }
    if (const auto* integer = std::get_if<std::int32_t>(&value)) {
        return static_cast<std::uint32_t>(*integer);
    }
    if (const auto* number = std::get_if<float>(&value)) {
        if (!std::isfinite(*number) ||
            *number < static_cast<float>(std::numeric_limits<std::int32_t>::min()) ||
            *number >= 2147483648.0f) {
            return 0x80000000u;
        }
        return static_cast<std::uint32_t>(static_cast<std::int32_t>(*number));
    }
    if (const auto* text = std::get_if<std::string>(&value)) {
        std::int32_t parsed = static_cast<std::int32_t>(fallback);
#ifdef _MSC_VER
        (void)::sscanf_s(text->c_str(), "%d", &parsed);
#else
        (void)std::sscanf(text->c_str(), "%d", &parsed);
#endif
        return static_cast<std::uint32_t>(parsed);
    }
    if (const auto* bytes = std::get_if<std::vector<std::uint8_t>>(&value)) {
        return RawBits(*bytes,fallback);
    }
    return fallback;
}

const BinaryXmlAttribute* BinaryXmlNode::FindAttribute(
    const std::string_view key) const {
    for (const auto& attribute : attributes) {
        if (attribute.name == key) return &attribute;
    }
    return nullptr;
}

bool BinaryXmlConfig::Parse(const std::span<const std::uint8_t> bytes) {
    std::size_t consumed = 0u;
    return ParsePrefix(bytes, consumed);
}

bool BinaryXmlConfig::ParsePrefix(
    const std::span<const std::uint8_t> bytes, std::size_t& consumed) {
    consumed = 0u;
    root_ = {};
    major_ = 0u;
    minor_ = 0u;
    Reader reader(bytes);
    std::uint32_t major = 0u;
    std::uint32_t minor = 0u;
    if (!reader.ReadWord(major) || !reader.ReadWord(minor) ||
        major > 1u || (major == 1u && minor > 1u)) {
        return false;
    }
    BinaryXmlNode root;
    if (!reader.ReadNode(root, 0u)) return false;
    consumed = reader.Position();
    root_ = std::move(root);
    major_ = major;
    minor_ = minor;
    return true;
}

bool BinaryXmlConfig::Load(const std::filesystem::path& path) {
    std::ifstream stream(path, std::ios::binary);
    if (!stream) return Parse({});
    const std::vector<std::uint8_t> bytes{
        std::istreambuf_iterator<char>(stream),
        std::istreambuf_iterator<char>()};
    return Parse(bytes);
}

std::optional<std::vector<std::uint8_t>> BinaryXmlConfig::Serialize() const {
    return Serialize(root_, major_, minor_);
}

std::optional<std::vector<std::uint8_t>> BinaryXmlConfig::Serialize(
    const BinaryXmlNode& root, const std::uint32_t major,
    const std::uint32_t minor) {
    if (major > 1u || (major == 1u && minor > 1u)) return std::nullopt;
    Writer writer;
    (void)writer.WriteWord(major);
    (void)writer.WriteWord(minor);
    if (!writer.WriteNode(root, 0u)) return std::nullopt;
    return writer.Take();
}

}  // namespace pal4
