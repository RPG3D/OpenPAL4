#pragma once

#include <cstdint>

namespace pal4 {

class IGameDataShopGoodsRecordTextRuntime {
public:
    virtual ~IGameDataShopGoodsRecordTextRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    [[nodiscard]] virtual std::uint32_t Free(std::uint32_t address) = 0;
};

class GameDataShopGoodsRecordText final {
public:
    // IDA 0x4f07d0: drop a shared text buffer and clear its pointer triplet.
    [[nodiscard]] static std::uint32_t Release(
        std::uint32_t record,
        IGameDataShopGoodsRecordTextRuntime& runtime);
};

}  // namespace pal4
