#pragma once

#include <cstdint>

namespace pal4 {

class IGameDataShopPropertyObjectRuntime {
public:
    virtual ~IGameDataShopPropertyObjectRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    virtual void ConstructBase(std::uint32_t object,
                               std::uint32_t name) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    virtual void ClearEffectData(std::uint32_t object) = 0;
    virtual void DestroyText(std::uint32_t address) = 0;
    virtual void ProcessElements(std::uint32_t address) = 0;
    virtual void DestroySecondaryRecord(std::uint32_t address) = 0;
    virtual void Free(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t DestroyBase(
        std::uint32_t object) = 0;
};

class GameDataShopPropertyObject final {
public:
    static constexpr std::uint32_t kBytes = 140;
    static constexpr std::uint32_t kVtable = 0x8446acu;

    // IDA 0x4ef1a0: allocate and initialize a shop property object.
    [[nodiscard]] static std::uint32_t Create(
        std::uint32_t name, IGameDataShopPropertyObjectRuntime& runtime);
    // IDA 0x4ef270: release owned record ranges and the Effect base.
    [[nodiscard]] static std::uint32_t Destroy(
        std::uint32_t object, IGameDataShopPropertyObjectRuntime& runtime);
};

}  // namespace pal4
