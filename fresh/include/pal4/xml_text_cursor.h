#pragma once

#include <cstdint>

namespace pal4 {

class IXmlTextCursorRuntime {
public:
    virtual ~IXmlTextCursorRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
};

class XmlTextCursorRuntimeStub final : public IXmlTextCursorRuntime {
public:
    std::uint32_t ReadWord(std::uint32_t) override { return 0u; }
    void WriteWord(std::uint32_t, std::uint32_t) override {}
    std::uint8_t ReadByte(std::uint32_t) override { return 0u; }
};

class XmlTextCursor final {
public:
    // IDA 0x5f1c00.
    static void Advance(std::uint32_t state, std::uint32_t end,
                        int utf8_mode, IXmlTextCursorRuntime& runtime);
    // IDA 0x5f1df0.
    [[nodiscard]] static const std::uint8_t* SkipWhitespaceAndBom(
        const std::uint8_t* text, int utf8_mode);
};

}  // namespace pal4
