#pragma once

#include <cstdint>

namespace pal4 {

class IPalMovementCollisionRuntime {
public:
    virtual ~IPalMovementCollisionRuntime() = default;
    [[nodiscard]] virtual std::uint32_t CheckFirst(
        std::uint32_t object, std::uint32_t first,
        std::uint32_t second, std::uint32_t third,
        std::uint32_t& hit, std::uint32_t zero_one,
        std::uint32_t zero_two) = 0;
    [[nodiscard]] virtual std::uint32_t CheckSecond(
        std::uint32_t object, std::uint32_t first,
        std::uint32_t second, std::uint32_t third,
        std::uint32_t& hit, std::uint32_t zero_one,
        std::uint32_t zero_two) = 0;
    [[nodiscard]] virtual std::uint32_t CheckThird(
        std::uint32_t object, std::uint32_t first,
        std::uint32_t second, std::uint32_t extra,
        std::uint32_t third, std::uint32_t& hit,
        std::uint32_t zero_one, std::uint32_t zero_two) = 0;
};

class PalMovementCollisionRuntimeStub final
    : public IPalMovementCollisionRuntime {
public:
    std::uint32_t CheckFirst(std::uint32_t object,
                              std::uint32_t first,
                              std::uint32_t second,
                              std::uint32_t third,
                              std::uint32_t& hit,
                              std::uint32_t zero_one,
                              std::uint32_t zero_two) override;
    std::uint32_t CheckSecond(std::uint32_t object,
                               std::uint32_t first,
                               std::uint32_t second,
                               std::uint32_t third,
                               std::uint32_t& hit,
                               std::uint32_t zero_one,
                               std::uint32_t zero_two) override;
    std::uint32_t CheckThird(std::uint32_t object,
                              std::uint32_t first,
                              std::uint32_t second,
                              std::uint32_t extra,
                              std::uint32_t third,
                              std::uint32_t& hit,
                              std::uint32_t zero_one,
                              std::uint32_t zero_two) override;
};

class PalMovementCollisionRouter final {
public:
    [[nodiscard]] static std::uint32_t CheckStages(
        std::uint32_t object, std::uint32_t enable_first,
        std::uint32_t enable_second, std::uint32_t enable_third,
        std::uint32_t first, std::uint32_t second,
        std::uint32_t third, std::uint32_t extra,
        IPalMovementCollisionRuntime& runtime);
};

}  // namespace pal4
