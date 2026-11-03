#include "pal4/xml_binary_codec.h"

namespace pal4 {
namespace {

std::uint8_t Nibble(const std::uint8_t character) {
    if (character >= '0' && character <= '9')
        return static_cast<std::uint8_t>(character - '0');
    if (character >= 'a' && character <= 'f')
        return static_cast<std::uint8_t>(character - 'a' + 10u);
    return 0u;
}

}  // namespace

std::int32_t XmlBinaryCodec::DecodeHex(
    const std::uint32_t source, const std::uint32_t destination,
    const std::uint32_t capacity_address, IXmlBinaryCodecRuntime& runtime) {
    const auto capacity =
        static_cast<std::int32_t>(runtime.ReadWord(capacity_address));
    if (source == 0u) return 0;
    for (std::int32_t index = 0; index < capacity; ++index)
        runtime.WriteByte(destination + static_cast<std::uint32_t>(index), 0u);
    if (runtime.ReadByte(source) == 0u) return 0;

    std::int32_t produced = 0;
    while (produced < capacity) {
        const auto index = static_cast<std::uint32_t>(produced);
        const auto high = Nibble(runtime.ReadByte(source + index * 2u));
        const auto low = Nibble(runtime.ReadByte(source + index * 2u + 1u));
        runtime.WriteByte(destination + index,
                          static_cast<std::uint8_t>((high << 4u) + low));
        ++produced;
        if (runtime.ReadByte(source + index + 1u) == 0u) break;
    }
    return produced;
}

}  // namespace pal4
