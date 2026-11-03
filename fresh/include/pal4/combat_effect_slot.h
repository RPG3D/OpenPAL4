#pragma once

#include <cstdint>

namespace pal4 {

class ICombatEffectSlotRuntime {
public:
    virtual ~ICombatEffectSlotRuntime() = default;
    virtual void ConstructNameNode(std::uint32_t slot) = 0;
    virtual void WriteByte(std::uint32_t address,
                           std::uint8_t value) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
};

class CombatEffectSlot final {
public:
    static constexpr std::uint32_t kSize = 24u;

    // IDA 0x5659b0: construct one effect record in a task array.
    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t slot, ICombatEffectSlotRuntime& runtime);
};

}  // namespace pal4
