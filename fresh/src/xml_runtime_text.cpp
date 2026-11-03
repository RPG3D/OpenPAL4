#include "pal4/xml_runtime_text.h"

#include "pal4/xml_text_cursor.h"
#include "pal4/xml_string_parser.h"

#include <cctype>
#include <limits>
#include <string>
#include <vector>

namespace pal4 {

std::uint32_t XmlRuntimeText::SkipWhitespaceAndBom(
    std::uint32_t source, const int mode,
    IXmlParserNodeRuntime& runtime) {
    if (source == 0u || runtime.ReadByte(source) == 0u) return 0u;
    while (runtime.ReadByte(source) != 0u) {
        if (mode == 1 && runtime.ReadByte(source) == 0xefu &&
            ((runtime.ReadByte(source + 1u) == 0xbbu &&
              runtime.ReadByte(source + 2u) == 0xbfu) ||
             (runtime.ReadByte(source + 1u) == 0xbfu &&
              (runtime.ReadByte(source + 2u) == 0xbeu ||
               runtime.ReadByte(source + 2u) == 0xbfu)))) {
            source += 3u;
            continue;
        }
        const auto byte = runtime.ReadByte(source);
        if (std::isspace(byte) == 0 && byte != '\r' && byte != '\n') break;
        ++source;
    }
    return source;
}

bool XmlRuntimeText::HasPrefix(
    const std::uint32_t source, const char* prefix,
    const bool ignore_case, IXmlParserNodeRuntime& runtime) {
    if (source == 0u || runtime.ReadByte(source) == 0u) return false;
    for (std::uint32_t index = 0u; prefix[index] != '\0'; ++index) {
        const auto byte = runtime.ReadByte(source + index);
        if (byte == 0u) return false;
        const auto expected = static_cast<unsigned char>(prefix[index]);
        if (ignore_case ? std::tolower(byte) != std::tolower(expected) :
                          byte != expected)
            return false;
    }
    return true;
}

void XmlRuntimeText::Advance(
    std::array<std::uint32_t, 4>& position, const std::uint32_t end,
    const int mode, IXmlParserNodeRuntime& runtime) {
    class Adapter final : public IXmlTextCursorRuntime {
    public:
        Adapter(std::array<std::uint32_t, 4>& position,
                IXmlParserNodeRuntime& runtime)
            : position_(position), runtime_(runtime) {}
        std::uint32_t ReadWord(const std::uint32_t address) override {
            return position_[(address - 0xfffffff0u) / 4u];
        }
        void WriteWord(const std::uint32_t address,
                       const std::uint32_t value) override {
            position_[(address - 0xfffffff0u) / 4u] = value;
        }
        std::uint8_t ReadByte(const std::uint32_t address) override {
            return runtime_.ReadByte(address);
        }
    private:
        std::array<std::uint32_t, 4>& position_;
        IXmlParserNodeRuntime& runtime_;
    } adapter{position, runtime};
    XmlTextCursor::Advance(0xfffffff0u, end, mode, adapter);
}

bool XmlRuntimeText::AssignDecoded(
    const std::uint32_t destination, const std::string_view text,
    IXmlNodeFactoryRuntime& runtime) {
    if (text.empty()) {
        (void)runtime.AssignString(destination, 0x8dc780u, 0u);
        return true;
    }
    if (text.size() >= std::numeric_limits<std::uint32_t>::max())
        return false;
    const auto scratch = runtime.Allocate(
        static_cast<std::uint32_t>(text.size() + 1u));
    if (scratch == 0u) return false;
    for (std::size_t index = 0u; index < text.size(); ++index)
        runtime.WriteByte(scratch + static_cast<std::uint32_t>(index),
                          static_cast<std::uint8_t>(text[index]));
    runtime.WriteByte(scratch + static_cast<std::uint32_t>(text.size()), 0u);
    std::uint32_t assigned = 0u;
    try {
        assigned = runtime.AssignString(
            destination, scratch, static_cast<std::uint32_t>(text.size()));
    } catch (...) {
        (void)runtime.Free(scratch);
        throw;
    }
    (void)runtime.Free(scratch);
    return assigned != 0u;
}

std::uint32_t XmlRuntimeText::ParseDecoded(
    const std::uint32_t source, const std::uint32_t destination,
    const char* delimiter, const int mode, IXmlNodeFactoryRuntime& runtime,
    const bool normalize) {
    if (source == 0u) return 0u;
    std::vector<std::uint8_t> bytes;
    auto current = source;
    while (runtime.ReadByte(current) != 0u) {
        bytes.push_back(runtime.ReadByte(current));
        ++current;
    }
    const auto delimiter_length = std::char_traits<char>::length(delimiter);
    bytes.resize(bytes.size() + delimiter_length + 1u, 0u);
    std::string decoded;
    const auto* end = XmlStringParser(
        runtime.ReadByte(0x8c1f98u) != 0u).Parse(
        bytes.data(), decoded, normalize,
        reinterpret_cast<const std::uint8_t*>(delimiter), false, mode);
    if (end == nullptr || !AssignDecoded(destination, decoded, runtime))
        return 0u;
    return source + static_cast<std::uint32_t>(end - bytes.data());
}

}  // namespace pal4
