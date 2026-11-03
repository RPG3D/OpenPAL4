#pragma once

#include <array>
#include <cstdint>
#include <source_location>

namespace pal4 {

class RandomNumberGenerator final {
public:
    static constexpr std::uint32_t kStateCount = 624u;
    static constexpr std::uint32_t kDefaultSeed = 0x1105u;
    static constexpr std::uint32_t kStateAddress = 0x8e0a58u;
    static constexpr std::uint32_t kIndexAddress = 0x8a1780u;

    RandomNumberGenerator();
    void Seed(std::uint32_t value);
    [[nodiscard]] std::uint32_t Next(
        std::source_location source=std::source_location::current());
    [[nodiscard]] std::uint32_t Index() const { return index_; }
    [[nodiscard]] std::uint32_t StateAt(std::uint32_t index) const {
        return state_.at(index);
    }

private:
    void Refresh();

    std::array<std::uint32_t, kStateCount> state_{};
    std::uint32_t index_ = kStateCount + 1u;
};

}  // namespace pal4
