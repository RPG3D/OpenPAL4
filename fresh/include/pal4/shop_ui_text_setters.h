#pragma once

#include <cstdint>

namespace pal4 {

class IShopUiTextRuntime {
public:
    virtual ~IShopUiTextRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    [[nodiscard]] virtual std::uint32_t FrameManager() = 0;
    [[nodiscard]] virtual std::uint32_t FindFrame(
        std::uint32_t manager, std::uint32_t name) = 0;
    virtual void RegisterFrame(std::uint32_t manager,
                               std::uint32_t name) = 0;
    [[nodiscard]] virtual std::uint32_t CStringLength(
        std::uint32_t text) = 0;
    [[noreturn]] virtual void ThrowLength() = 0;
    virtual void SetErrorFlag(std::uint32_t text,
                              std::uint8_t flag) = 0;
    virtual void Reallocate(std::uint32_t text,
                            std::uint32_t length) = 0;
    virtual void Copy(std::uint32_t destination,
                      std::uint32_t source,
                      std::uint32_t length) = 0;
};

class ShopUiTextSetters final {
public:
    static constexpr std::uint32_t kFrameNameSource = 0x8b098cu;

    // IDA 0x486890 and 0x486980.
    [[nodiscard]] static std::uint8_t SetShopId(
        std::uint32_t event, IShopUiTextRuntime& runtime);
    [[nodiscard]] static std::uint8_t SetTradeTaskScript(
        std::uint32_t event, IShopUiTextRuntime& runtime);
};

}  // namespace pal4
