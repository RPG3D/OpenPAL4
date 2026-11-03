#include "pal4/player_indexed_stats.h"

#include <bit>
#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <utility>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IPlayerIndexedStatsRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::uint32_t> reads;
    std::vector<std::pair<std::uint32_t, std::uint32_t>> writes;

    std::uint32_t ReadWord(std::uint32_t address) override {
        reads.push_back(address);
        return words.at(address);
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        writes.emplace_back(address, value);
        words[address] = value;
    }
};

void CheckIntegerFields() {
    constexpr std::uint32_t player = 0x1000u;
    Runtime runtime;
    for (std::uint32_t i = 0; i < 8u; ++i)
        runtime.words[player + 4u + i * 4u] = 100u + i;
    for (std::uint32_t i = 0; i < 5u; ++i)
        runtime.words[player + 0x7cu + i * 4u] = 200u + i;
    for (std::uint32_t i = 0; i < 6u; ++i)
        runtime.words[player + 0x34u + i * 4u] = 300u + i;

    for (std::int32_t i = 0; i < 5; ++i)
        Check(pal4::PlayerIndexedStats::GetCurrent(player, i, runtime) ==
                  200u + static_cast<std::uint32_t>(i),
              "current stat offset");
    for (std::int32_t i = 0; i < 8; ++i)
        Check(pal4::PlayerIndexedStats::AddBase(player, i, 9u, runtime) ==
                  109u + static_cast<std::uint32_t>(i),
              "base stat addition and return");
    for (std::int32_t i = 0; i < 6; ++i) {
        Check(pal4::PlayerIndexedStats::GetIntegerResistance(
                  player, i, runtime) ==
                  300u + static_cast<std::uint32_t>(i),
              "integer resistance getter offset");
        Check(pal4::PlayerIndexedStats::SetIntegerResistance(
                  player, i, 400u + static_cast<std::uint32_t>(i),
                  runtime) ==
                  400u + static_cast<std::uint32_t>(i),
              "integer resistance setter return");
    }
    Check(runtime.writes.front() == std::pair{player + 4u, 109u} &&
              runtime.writes.back() ==
                  std::pair{player + 0x34u + 5u * 4u, 405u},
          "integer write addresses");

    const auto previous_reads = runtime.reads.size();
    const auto previous_writes = runtime.writes.size();
    Check(pal4::PlayerIndexedStats::GetCurrent(player, -1, runtime) ==
                  0xffffffffu &&
              pal4::PlayerIndexedStats::GetCurrent(player, 5, runtime) == 5u &&
              pal4::PlayerIndexedStats::AddBase(player, -1, 7u,
                                                 runtime) == 0xffffffffu &&
              pal4::PlayerIndexedStats::AddBase(player, 8, 7u,
                                                 runtime) == 8u &&
              pal4::PlayerIndexedStats::GetIntegerResistance(
                  player, 6, runtime) == 6u &&
              pal4::PlayerIndexedStats::SetIntegerResistance(
                  player, -1, 9u, runtime) == 0xffffffffu &&
              runtime.reads.size() == previous_reads &&
              runtime.writes.size() == previous_writes,
          "invalid integer indices return raw index without memory access");

    runtime.words[player + 4u] = 0xffffffffu;
    Check(pal4::PlayerIndexedStats::AddBase(player, 0, 2u, runtime) == 1u,
          "base addition wraps at 32 bits");
}

void CheckFloatFields() {
    constexpr std::uint32_t player = 0x1000u;
    Runtime runtime;
    for (std::uint32_t i = 0; i < 4u; ++i)
        runtime.words[player + 0x24u + i * 4u] =
            std::bit_cast<std::uint32_t>(1.25f + static_cast<float>(i));
    for (std::uint32_t i = 0; i < 6u; ++i)
        runtime.words[player + 0x64u + i * 4u] =
            std::bit_cast<std::uint32_t>(2.5f + static_cast<float>(i));

    for (std::int32_t i = 0; i < 4; ++i) {
        Check(pal4::PlayerIndexedStats::GetAttribute(player, i, runtime) ==
                  static_cast<double>(1.25f + static_cast<float>(i)),
              "attribute getter and float widening");
        Check(pal4::PlayerIndexedStats::SetAttribute(player, i, -3.5f,
                                                     runtime) == -3.5 &&
                  runtime.words.at(player + 0x24u + i * 4u) ==
                      std::bit_cast<std::uint32_t>(-3.5f),
              "attribute setter offset and x87 return");
    }
    for (std::int32_t i = 0; i < 6; ++i) {
        Check(pal4::PlayerIndexedStats::GetSkill(player, i, runtime) ==
                  static_cast<double>(2.5f + static_cast<float>(i)),
              "skill getter offset");
        Check(pal4::PlayerIndexedStats::SetSkill(player, i, 4.25f,
                                                 runtime) == 4.25 &&
                  runtime.words.at(player + 0x64u + i * 4u) ==
                      std::bit_cast<std::uint32_t>(4.25f),
              "skill setter offset and return");
        Check(pal4::PlayerIndexedStats::SetFloatResistance(
                  player, i, -1.5f, runtime) == -1.5 &&
                  runtime.words.at(player + 0x4cu + i * 4u) ==
                      std::bit_cast<std::uint32_t>(-1.5f),
              "float resistance setter has distinct offset");
    }
    const auto previous_reads = runtime.reads.size();
    const auto previous_writes = runtime.writes.size();
    Check(pal4::PlayerIndexedStats::GetAttribute(player, -1,
                                                  runtime) == -1.0 &&
              pal4::PlayerIndexedStats::SetAttribute(player, 4, 9.0f,
                                                      runtime) == 4.0 &&
              pal4::PlayerIndexedStats::GetSkill(player, 6,
                                                  runtime) == 6.0 &&
              pal4::PlayerIndexedStats::SetSkill(player, -1, 9.0f,
                                                  runtime) == -1.0 &&
              pal4::PlayerIndexedStats::SetFloatResistance(
                  player, 6, 9.0f, runtime) == 6.0 &&
              runtime.reads.size() == previous_reads &&
              runtime.writes.size() == previous_writes,
          "invalid float indices return signed index without memory access");
}

}  // namespace

int main() {
    try {
        CheckIntegerFields();
        CheckFloatFields();
        std::cout << "player_indexed_stats_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
