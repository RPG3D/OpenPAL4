#include "pal4/game_data_race_stats.h"

#include <bit>

namespace pal4 {

GameDataRaceStats& GameDataRaceStats::Construct() {
    (void)Reset();
    return *this;
}

std::uint32_t GameDataRaceStats::Reset() {
    words_.fill(0);
    words_[0] = 7;
    return 0;
}

std::uint32_t GameDataRaceStats::AddFrom(
    const GameDataRaceStats& source, std::uint32_t destination_address,
    std::uint32_t source_address) {
    const auto add_words = [&](std::size_t first, std::size_t count) {
        for (std::size_t i = first; i < first + count; ++i)
            words_[i] += source.words_[i];
    };
    const auto add_floats = [&](std::size_t first, std::size_t count) {
        for (std::size_t i = first; i < first + count; ++i) {
            const auto left = std::bit_cast<float>(words_[i]);
            const auto right = std::bit_cast<float>(source.words_[i]);
            words_[i] = std::bit_cast<std::uint32_t>(left + right);
        }
    };
    add_words(1, 8);
    add_floats(9, 4);
    add_words(13, 6);
    add_floats(25, 6);
    add_floats(19, 6);
    add_words(31, 5);
    return source_address - destination_address;
}

GameDataRaceDescriptorHeader& GameDataRaceDescriptorHeader::Initialize(
    std::uint32_t source_address, std::uint32_t kind) {
    words_[0] = kBaseVtable;
    words_[1] = source_address;
    words_[2] = kind;
    return *this;
}

}  // namespace pal4
