#pragma once

#include <cstdint>

namespace pal4 {

class ICombatPropertyWriterRuntime {
public:
    virtual ~ICombatPropertyWriterRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void SetString(std::uint32_t node, std::uint32_t key,
                           std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint8_t WriteInt(
        std::uint32_t node, std::uint32_t key, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t CreateNode(
        std::uint32_t document, std::uint32_t name) = 0;
    [[nodiscard]] virtual std::uint32_t CombatSystemInstance() = 0;
    [[nodiscard]] virtual std::uint32_t SetProperty110(
        std::uint32_t system, std::uint32_t value) = 0;
};

class CombatPropertyWriter final {
public:
    static constexpr std::uint32_t kEmptyText = 0x841964u;

    // IDA 0x579360.
    [[nodiscard]] static std::uint32_t SetCombatProperties(
        std::uint32_t node, std::uint32_t scene,
        std::uint32_t area, std::uint32_t start_type,
        std::uint32_t property110, ICombatPropertyWriterRuntime& runtime);

    // IDA 0x5793e0.
    [[nodiscard]] static std::uint8_t SetMonsterProperties(
        std::uint32_t document, std::uint32_t monster_id,
        std::uint32_t position, std::uint32_t abnormal_state,
        ICombatPropertyWriterRuntime& runtime);

private:
    [[nodiscard]] static std::uint32_t Key(
        std::uint32_t record, ICombatPropertyWriterRuntime& runtime);
};

}  // namespace pal4
