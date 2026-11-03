#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace pal4 {

struct BinaryXmlAttribute {
    using Value = std::variant<std::monostate, std::int8_t, std::int32_t,
                               float, std::string, std::vector<std::uint8_t>>;

    std::string name;
    std::uint32_t type = 0u;
    Value value;
    [[nodiscard]] std::uint32_t ToInt(std::uint32_t fallback) const;
    [[nodiscard]] float ToFloat(float fallback) const;
};

struct BinaryXmlNode {
    std::string name;
    std::vector<BinaryXmlAttribute> attributes;
    std::vector<BinaryXmlNode> children;

    [[nodiscard]] const BinaryXmlAttribute* FindAttribute(
        std::string_view key) const;
};

class BinaryXmlConfig final {
public:
    [[nodiscard]] bool Parse(std::span<const std::uint8_t> bytes);
    [[nodiscard]] bool ParsePrefix(std::span<const std::uint8_t> bytes,
                                   std::size_t& consumed);
    [[nodiscard]] bool Load(const std::filesystem::path& path);
    [[nodiscard]] std::optional<std::vector<std::uint8_t>> Serialize() const;
    [[nodiscard]] static std::optional<std::vector<std::uint8_t>> Serialize(
        const BinaryXmlNode& root, std::uint32_t major = 1u,
        std::uint32_t minor = 1u);
    [[nodiscard]] const BinaryXmlNode& Root() const { return root_; }
    [[nodiscard]] std::uint32_t MajorVersion() const { return major_; }
    [[nodiscard]] std::uint32_t MinorVersion() const { return minor_; }

private:
    BinaryXmlNode root_;
    std::uint32_t major_ = 0u;
    std::uint32_t minor_ = 0u;
};

}  // namespace pal4
