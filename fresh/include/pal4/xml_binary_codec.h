#pragma once

#include <cstdint>

namespace pal4 {

class IXmlBinaryCodecRuntime {
public:
    virtual ~IXmlBinaryCodecRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
};

class XmlBinaryCodec final {
public:
    // IDA 0x5f7440. Capacity is a signed word in the source address space.
    [[nodiscard]] static std::int32_t DecodeHex(
        std::uint32_t source, std::uint32_t destination,
        std::uint32_t capacity_address, IXmlBinaryCodecRuntime& runtime);
};

}  // namespace pal4
