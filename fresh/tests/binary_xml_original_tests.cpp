#include "pal4/binary_xml_config.h"

#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <iterator>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

void CheckSynthetic() {
    pal4::BinaryXmlNode root;
    root.name = "GlobalData";
    root.attributes = {
        {"byte", 0u, std::int8_t{-7}},
        {"integer", 1u, std::int32_t{-12345}},
        {"float", 2u, 1.25f},
        {"text", 3u, std::string("abc")},
        {"bytes", 4u, std::vector<std::uint8_t>{0u, 1u, 255u}},
    };
    root.children.push_back({"Child", {{"value", 1u, std::int32_t{42}}},
                             {}});
    const auto encoded = pal4::BinaryXmlConfig::Serialize(root);
    Check(encoded.has_value(), "serialize all original binary XML types");
    pal4::BinaryXmlConfig parsed;
    Check(parsed.Parse(*encoded) && parsed.Root().children.size() == 1u &&
              parsed.Root().FindAttribute("integer")->ToInt(0) ==
                  static_cast<std::uint32_t>(-12345) &&
              parsed.Serialize() == encoded,
          "binary XML types and child round trip");
    Check(parsed.Root().FindAttribute("byte")->ToFloat(0.0f) == -7.0f &&
              parsed.Root().FindAttribute("integer")->ToFloat(0.0f) == -12345.0f &&
              parsed.Root().FindAttribute("float")->ToFloat(0.0f) == 1.25f &&
              parsed.Root().FindAttribute("text")->ToFloat(50.0f) == 50.0f &&
              parsed.Root().FindAttribute("bytes")->ToFloat(50.0f) == 50.0f,
          "binary XML floating attributes convert numeric types and preserve fallback");
    const pal4::BinaryXmlAttribute speed{
        "speed", 3u, std::string("  75.5 trailing")};
    Check(speed.ToFloat(50.0f) == 75.5f,
          "text floating attribute accepts the existing numeric prefix convention");
    std::vector<std::uint8_t> consecutive = *encoded;
    consecutive.insert(consecutive.end(), encoded->begin(), encoded->end());
    std::size_t consumed = 0u;
    Check(parsed.ParsePrefix(consecutive, consumed) &&
              consumed == encoded->size() &&
              parsed.ParsePrefix(
                  std::span(consecutive).subspan(consumed), consumed) &&
              consumed == encoded->size(),
          "binary XML prefix parsing preserves consecutive document bounds");
    consumed = 99u;
    Check(!parsed.ParsePrefix(std::span(*encoded).first(7u), consumed) &&
              consumed == 0u,
          "truncated binary XML prefix reports no consumed bytes");
    root.attributes.front().type = 5u;
    Check(!pal4::BinaryXmlConfig::Serialize(root),
          "unknown binary XML attribute type is rejected");
}

class SettingsCursor final {
public:
    explicit SettingsCursor(const std::vector<std::uint8_t>& bytes)
        : bytes_(bytes) {}
    std::uint32_t Word() {
        Check(position_ + 4u <= bytes_.size(), "original settings word bounds");
        std::uint32_t value{};
        std::memcpy(&value, bytes_.data() + position_, sizeof(value));
        position_ += 4u;
        return value;
    }
    void Skip(std::size_t length) {
        Check(length <= bytes_.size() - position_,
              "original settings field bounds");
        position_ += length;
    }
    [[nodiscard]] std::size_t Position() const { return position_; }

private:
    const std::vector<std::uint8_t>& bytes_;
    std::size_t position_{};
};

void CheckOriginalSettings(const std::string& root) {
    std::ifstream file(root + "\\pal4.set", std::ios::binary);
    Check(static_cast<bool>(file), "open original pal4.set");
    const std::vector<std::uint8_t> bytes{
        std::istreambuf_iterator<char>(file),
        std::istreambuf_iterator<char>()};
    SettingsCursor cursor(bytes);
    const auto header_count = cursor.Word();
    Check(header_count == 5u, "original settings header count");
    cursor.Skip(header_count * 4u);
    cursor.Skip(4u);  // Movie flags at global-data offset 0x18.
    for (int index = 0; index < 2; ++index) {
        const auto length = cursor.Word();
        Check(length == 256u, "original dialog flag string length");
        cursor.Skip(length);
    }
    const auto extra_count = cursor.Word();
    cursor.Skip(static_cast<std::size_t>(extra_count) * 4u);
    cursor.Skip(5u * 4u + 1u + 2u * 4u);
    for (int index = 0; index < 4; ++index) {
        const auto node_count = cursor.Word();
        cursor.Skip(static_cast<std::size_t>(node_count) * 4u);
    }
    pal4::BinaryXmlConfig xml;
    const std::span tail(bytes.data() + cursor.Position(),
                         bytes.size() - cursor.Position());
    const bool parsed = xml.Parse(tail);
    const auto roundtrip = parsed ? xml.Serialize() : std::nullopt;
    Check(parsed && xml.Root().name == "GlobalData" &&
              roundtrip &&
              *roundtrip == std::vector<std::uint8_t>(tail.begin(), tail.end()),
          "original pal4.set binary XML round trips byte for byte");
}

}  // namespace

int main(int argc, char** argv) {
    try {
        Check(argc == 2, "provide original PAL4 installation");
        CheckSynthetic();
        std::ifstream file(std::string(argv[1]) + "\\config.cfg",
                           std::ios::binary);
        Check(static_cast<bool>(file), "open original config.cfg");
        const std::vector<std::uint8_t> original{
            std::istreambuf_iterator<char>(file),
            std::istreambuf_iterator<char>()};
        pal4::BinaryXmlConfig config;
        Check(config.Parse(original), "parse original config.cfg");
        const auto encoded = config.Serialize();
        Check(encoded && *encoded == original,
              "serialize original config.cfg byte for byte");
        CheckOriginalSettings(argv[1]);
        std::cout << "Original binary XML round trip: "
                  << original.size() << " bytes\n";
        return EXIT_SUCCESS;
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return EXIT_FAILURE;
    }
}
