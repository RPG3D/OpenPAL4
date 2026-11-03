#pragma once

#include <array>
#include <cstdint>

namespace pal4 {

class GameDataRaceStats final {
public:
    static constexpr std::size_t kWordCount = 36;
    using Words = std::array<std::uint32_t, kWordCount>;

    // IDA 0x4cf1e0: constructor wrapper returns its own object.
    [[nodiscard]] GameDataRaceStats& Construct();
    // IDA 0x4cf290: set tag 7 and clear all remaining 140 bytes.
    [[nodiscard]] std::uint32_t Reset();
    // IDA 0x4cf1f0: add signed-independent DWORDs and three float ranges.
    [[nodiscard]] std::uint32_t AddFrom(const GameDataRaceStats& source,
                                        std::uint32_t destination_address,
                                        std::uint32_t source_address);

    [[nodiscard]] Words& Data() { return words_; }
    [[nodiscard]] const Words& Data() const { return words_; }

private:
    Words words_{};
};

class GameDataRaceDescriptorHeader final {
public:
    static constexpr std::uint32_t kBaseVtable = 0x8444ac;
    using Words = std::array<std::uint32_t, 8>;

    // IDA 0x4cf2f0: initialize only the first three words of a 32-byte object.
    [[nodiscard]] GameDataRaceDescriptorHeader& Initialize(
        std::uint32_t source_address, std::uint32_t kind);
    [[nodiscard]] Words& Data() { return words_; }
    [[nodiscard]] const Words& Data() const { return words_; }

private:
    Words words_{};
};

}  // namespace pal4
