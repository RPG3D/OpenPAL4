#pragma once

#include <cstdint>

namespace pal4 {

class IGameDataShopGoodsObjectRuntime {
public:
    virtual ~IGameDataShopGoodsObjectRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    virtual void ConstructBase(std::uint32_t object, std::uint32_t name) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    virtual void ClearEffectData(std::uint32_t object) = 0;
    virtual void DestroyRecordText(std::uint32_t address) = 0;
    virtual void ProcessElements(std::uint32_t address) = 0;
    virtual void DestroySecondaryRecord(std::uint32_t address) = 0;
    virtual void Free(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t DestroyBase(
        std::uint32_t object) = 0;
};

class GameDataShopGoodsObject final {
public:
    static constexpr std::uint32_t kBytes = 140;
    static constexpr std::uint32_t kVtable = 0x8446ccu;

    // IDA 0x4efd30: allocate and construct the Good table object.
    [[nodiscard]] static std::uint32_t Create(
        std::uint32_t name, IGameDataShopGoodsObjectRuntime& runtime);
    // IDA 0x4efe00: release owned records and the Effect base.
    [[nodiscard]] static std::uint32_t Destroy(
        std::uint32_t object, IGameDataShopGoodsObjectRuntime& runtime);
};

}  // namespace pal4
