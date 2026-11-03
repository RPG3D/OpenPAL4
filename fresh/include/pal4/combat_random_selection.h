#pragma once

#include <cstdint>
#include <vector>

namespace pal4 {

class ICombatRandomSelectionRuntime {
public:
    virtual ~ICombatRandomSelectionRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t RandomNumber() = 0;
    [[nodiscard]] virtual std::int32_t FloatToInt(float value) = 0;
    virtual void ApplySelected(std::uint32_t target,
                               std::int32_t selected,
                               std::uint32_t context) = 0;
    virtual void ApplySelectedActorEvent(std::uint32_t target,
                                         std::int32_t selected,
                                         std::uint32_t context) = 0;
    [[nodiscard]] virtual std::int32_t ReleaseCollection(
        const std::vector<std::int32_t>& values) = 0;
};

class CombatRandomSelection final {
public:
    // IDA 0x580ba0. An empty accepted collection throws instead of native div/0.
    [[nodiscard]] static std::int32_t Process(
        std::uint32_t action, std::uint32_t context,
        std::uint32_t target, ICombatRandomSelectionRuntime& runtime);
    // IDA 0x581280. The second native argument is unused.
    [[nodiscard]] static std::int32_t ProcessActorEvent(
        std::uint32_t action, std::uint32_t target,
        std::uint32_t context, ICombatRandomSelectionRuntime& runtime);
};

}  // namespace pal4
