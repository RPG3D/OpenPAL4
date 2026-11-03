#include "pal4/skill_data_lookup.h"

#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::ISkillDataLookupRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::uint32_t> reads;

    std::uint32_t ReadWord(std::uint32_t address) override {
        reads.push_back(address);
        return words.at(address);
    }
};

void FillTable(Runtime& runtime, std::uint32_t table,
               std::uint32_t index, std::uint32_t records) {
    runtime.words[table + 8] = index;
    runtime.words[table + 12] = index + 4 * 8;
    runtime.words[table + 96] = records;
    const std::int32_t keys[] = {-3, 2, 2, 10};
    const std::uint32_t positions[] = {3, 0, 4, 1};
    for (std::uint32_t i = 0; i < 4; ++i) {
        runtime.words[index + 8 * i] =
            static_cast<std::uint32_t>(keys[i]);
        runtime.words[index + 8 * i + 4] = positions[i];
    }
}

void CheckLookups() {
    Runtime runtime;
    runtime.words[0x1000 + 4] = 0x2000;
    runtime.words[0x1000 + 8] = 0x3000;
    runtime.words[0x1000 + 12] = 0x4000;
    FillTable(runtime, 0x2000, 0x5000, 0x9000);
    FillTable(runtime, 0x3000, 0x6000, 0xa000);
    FillTable(runtime, 0x4000, 0x7000, 0xb000);
    Check(pal4::SkillDataLookup::FindSkill(0x1000, -3, runtime) ==
              0x9000 + 3 * 132 &&
              pal4::SkillDataLookup::FindSkill(0x1000, 2, runtime) ==
                  0x9000 &&
              pal4::SkillDataLookup::FindSkill(0x1000, 1, runtime) == 0 &&
              pal4::SkillDataLookup::HasSkill(0x1000, 10, runtime) &&
              !pal4::SkillDataLookup::HasSkill(0x1000, 11, runtime),
          "Skill uses signed keys, first duplicate and 132-byte stride");
    Check(pal4::SkillDataLookup::FindStunt(0x1000, -3, runtime) ==
              0xa000 + 3 * 208 &&
              pal4::SkillDataLookup::FindStunt(0x1000, 2, runtime) ==
                  0xa000 &&
              pal4::SkillDataLookup::HasStunt(0x1000, 10, runtime) &&
              !pal4::SkillDataLookup::HasStunt(0x1000, 1, runtime),
          "Stunt uses second table and 208-byte stride");
    Check(pal4::SkillDataLookup::FindMagic(0x1000, -3, runtime) ==
              0xb000 + 3 * 152 &&
              pal4::SkillDataLookup::FindMagic(0x1000, 2, runtime) ==
                  0xb000 &&
              pal4::SkillDataLookup::HasMagic(0x1000, 10, runtime) &&
              !pal4::SkillDataLookup::HasMagic(0x1000, 1, runtime),
          "Magic uses third table and 152-byte stride");

    runtime.words[0x2000 + 12] = 0x5000;
    Check(pal4::SkillDataLookup::FindSkill(0x1000, 2, runtime) == 0,
          "empty index does not access an entry");
    runtime.words[0x2000 + 12] = 0x5000 + 4 * 8;
    runtime.words[0x2000 + 96] = 0;
    Check(!pal4::SkillDataLookup::HasSkill(0x1000, 2, runtime),
          "presence is based on computed record address");
}

void CheckRecordPredicates() {
    Runtime runtime;
    runtime.words[0x5004] = 3;
    runtime.words[0x5008] = 14;
    runtime.words[0x500c] = 0xffffffffu;
    Check(pal4::SkillDataLookup::IsSkillRecordValid(0x5000, runtime) &&
              pal4::SkillDataLookup::IsUnsetRecord(0x5000, runtime),
          "upper valid boundaries and minus-one sentinel match");
    runtime.words[0x5004] = 4;
    runtime.reads.clear();
    Check(!pal4::SkillDataLookup::IsSkillRecordValid(0x5000, runtime) &&
              runtime.reads == std::vector<std::uint32_t>{0x5004},
          "invalid first field short-circuits second field read");
    runtime.words[0x5004] = 0xffffffffu;
    Check(!pal4::SkillDataLookup::IsSkillRecordValid(0x5000, runtime),
          "first field is compared unsigned");
    runtime.words[0x5004] = 0;
    runtime.words[0x5008] = 15;
    runtime.words[0x500c] = 0;
    Check(!pal4::SkillDataLookup::IsSkillRecordValid(0x5000, runtime) &&
              !pal4::SkillDataLookup::IsUnsetRecord(0x5000, runtime),
          "second field and sentinel rejection match");
}

}  // namespace

int main() {
    try {
        CheckLookups();
        CheckRecordPredicates();
        std::cout << "skill_data_lookup_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
