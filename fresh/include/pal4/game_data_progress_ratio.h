#pragma once

#include <cstdint>

namespace pal4 {

class IGameDataProgressRatioRuntime {
public:
    virtual ~IGameDataProgressRatioRuntime() = default;
    [[nodiscard]] virtual std::uint32_t GameDatabase() = 0;
    [[nodiscard]] virtual std::uint32_t GlobalData() = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
};

class GameDataProgressRatio final {
public:
    // IDA 0x4c5060: global counter divided by 400-byte GameDB record count.
    [[nodiscard]] static double Calculate(
        IGameDataProgressRatioRuntime& runtime);
    // IDA 0x4c7bb0: global item count divided by 168-byte GameDB records.
    [[nodiscard]] static double CalculateItems(
        IGameDataProgressRatioRuntime& runtime);
};

}  // namespace pal4
