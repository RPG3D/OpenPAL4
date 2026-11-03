#include "pal4/game_data_record_access.h"

#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <utility>
#include <vector>

namespace {

void Check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

struct Runtime final : pal4::IGameDataRecordAccessRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::uint32_t> reads;
    std::vector<std::pair<std::uint32_t, std::uint32_t>> effect_calls;
    std::uint32_t effect_result = 0xface;

    std::uint32_t ReadWord(std::uint32_t address) override {
        reads.push_back(address);
        return words[address];
    }
    std::uint32_t FindEffectEntry(std::uint32_t collection,
                                  std::uint32_t key) override {
        effect_calls.emplace_back(collection, key);
        return effect_result;
    }
};

void CheckAccessorsAndSelector() {
    Runtime runtime;
    runtime.words[0x1010] = 0x1234;
    Check(pal4::GameDataRecordAccess::MemberAt16(0x1000, runtime) ==
              0x1234 && runtime.reads.back() == 0x1010,
          "fourth DWORD accessor uses object offset 16");
    runtime.words[0x100c] = 0x7000;
    Check(pal4::GameDataRecordAccess::FindEffect(
              0x1000, 0x22, runtime) == 0xface &&
              runtime.effect_calls ==
                  std::vector<std::pair<std::uint32_t, std::uint32_t>>{
                      {0x7000, 0x22}},
          "Effect lookup receives member +12 and key");

    runtime.words[0x1004] = 0x5000;
    runtime.words[0x5060] = 0x6000;
    runtime.words[0x5064] = 0x6030;
    for (std::uint32_t i = 0; i < 4; ++i) {
        runtime.words[0x6000 + i * 12 + 4] = 100 + i;
        runtime.words[0x6000 + i * 12 + 8] = i == 1 ? 8 : 7;
    }
    Check(pal4::GameDataRecordAccess::FirstMatchingRecordValue(
              0x1000, 7, runtime) == 100,
          "selector returns second word of first matching 12-byte record");
    Check(pal4::GameDataRecordAccess::FirstMatchingRecordValue(
              0x1000, 8, runtime) == 101,
          "selector skips nonmatching prefix");
    runtime.words[0x6004] = 0;
    Check(pal4::GameDataRecordAccess::FirstMatchingRecordValue(
              0x1000, 7, runtime) == 0,
          "first matching value zero is not the no-match sentinel five");
    Check(pal4::GameDataRecordAccess::FirstMatchingRecordValue(
              0x1000, 9, runtime) == 5,
          "selector returns five for no matching record");
    runtime.words[0x5060] = 0;
    Check(pal4::GameDataRecordAccess::FirstMatchingRecordValue(
              0x1000, 7, runtime) == 5,
          "null collection begin is empty");
}

void CheckSignedSearch() {
    Runtime runtime;
    runtime.words[0x1008] = 0x3000;
    runtime.words[0x3008] = 0x2000;
    runtime.words[0x300c] = 0x2018;
    runtime.words[0x3060] = 0x4000;
    runtime.words[0x2000] = 0xfffffffd;
    runtime.words[0x2004] = 2;
    runtime.words[0x2008] = 4;
    runtime.words[0x200c] = 0;
    runtime.words[0x2010] = 9;
    runtime.words[0x2014] = 1;
    Check(pal4::GameDataRecordAccess::FindRecord(
              0x1000, -3, runtime) == 0x4320 &&
              pal4::GameDataRecordAccess::HasRecord(
                  0x1000, -3, runtime),
          "signed negative key finds index two with 400-byte stride");
    Check(pal4::GameDataRecordAccess::FindRecord(
              0x1000, 4, runtime) == 0x4000 &&
              pal4::GameDataRecordAccess::FindRecord(
                  0x1000, 9, runtime) == 0x4190,
          "middle and last keys use their stored indices");
    for (const auto missing : {-4, 0, 5, 10})
        Check(pal4::GameDataRecordAccess::FindRecord(
                  0x1000, missing, runtime) == 0 &&
                  !pal4::GameDataRecordAccess::HasRecord(
                      0x1000, missing, runtime),
              "lower-bound gaps and ends return no record");

    runtime.words[0x3060] = 0;
    Check(pal4::GameDataRecordAccess::FindRecord(
              0x1000, 4, runtime) == 0 &&
              !pal4::GameDataRecordAccess::HasRecord(
                  0x1000, 4, runtime),
          "boolean lookup tests the computed address itself");
    runtime.words[0x300c] = 0x2000;
    Check(pal4::GameDataRecordAccess::FindRecord(
              0x1000, 4, runtime) == 0,
          "empty key array returns zero");
}

}  // namespace

int main() {
    try {
        CheckAccessorsAndSelector();
        CheckSignedSearch();
        std::cout << "game_data_record_access_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
