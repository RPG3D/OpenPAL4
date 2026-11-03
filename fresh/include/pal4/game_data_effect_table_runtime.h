#pragma once

#include <cstdint>

namespace pal4 {

class IGameDataEffectTableRuntime {
public:
    virtual ~IGameDataEffectTableRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t size) = 0;
    virtual void ConstructEffect(std::uint32_t object,
                                 std::uint32_t name) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    virtual void ClearEffectData(std::uint32_t object) = 0;
    virtual void CleanupTextRecords(std::uint32_t record) = 0;
    virtual void ProcessElements(std::uint32_t elements) = 0;
    virtual void DestroyRecord(std::uint32_t record) = 0;
    virtual void DestroyAuxRecord(std::uint32_t record) = 0;
    virtual void Free(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t DestructEffect(
        std::uint32_t object) = 0;
};

}  // namespace pal4
