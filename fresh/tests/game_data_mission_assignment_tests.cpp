#include "pal4/game_data_mission_assignment.h"

#include <cstdint>
#include <iostream>
#include <map>
#include <new>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

struct Runtime final : pal4::IGameDataMissionAssignmentRuntime {
    std::map<std::uint32_t, std::uint32_t> words{
        {0x5004, 0x7000}, {0x7060, 0x9000}, {0x7064, 0x9000},
        {0x4158, 0xa000}, {0x415c, 0xa008}};
    std::vector<std::size_t> allocations;
    std::vector<std::uint32_t> freed;
    std::vector<std::uint32_t> released;
    std::uint32_t next_address = 0xc000;
    std::size_t fail_allocation_number = 0;
    std::uint32_t game_state_calls = 0;
    bool throw_on_level = false;

    std::uint32_t ReadWord(std::uint32_t address) override {
        Check(words.contains(address), "Mission assignment reads known memory");
        return words.at(address);
    }
    std::uint32_t AllocatePointerStorage(std::size_t capacity) override {
        allocations.push_back(capacity);
        if (allocations.size() == fail_allocation_number) return 0;
        const auto address = next_address;
        next_address += 0x1000;
        return address;
    }
    void FreePointerStorage(std::uint32_t address) override {
        freed.push_back(address);
    }
    std::uint32_t GameState() override {
        ++game_state_calls;
        return 0x4000;
    }
    std::int32_t CurrentLevel(std::uint32_t game_state,
                              std::uint32_t category) override {
        Check(game_state == 0x4000, "level lookup uses current PALGameIV");
        if (throw_on_level) throw std::runtime_error("level failed");
        return category < 2 ? 4 : -1;
    }
    int ReleaseTemporaryStorage(std::uint32_t address) override {
        released.push_back(address);
        return 77;
    }
    void SetRecords(const std::vector<std::pair<std::uint32_t,
                                                 std::int32_t>>& records) {
        words[0x7060] = 0x9000;
        words[0x7064] = 0x9000 +
                        static_cast<std::uint32_t>(records.size() * 68);
        for (std::size_t i = 0; i < records.size(); ++i) {
            const auto address = 0x9000 + static_cast<std::uint32_t>(i * 68);
            words[address + 4] = static_cast<std::uint32_t>(records[i].second);
            words[address + 8] = records[i].first;
        }
    }
};

void CheckSelectionAndGrowth() {
    Runtime runtime;
    runtime.SetRecords({{0, 1}, {0, 3}, {0, 5},
                        {1, 2}, {1, 4}, {2, 1}});
    pal4::GameDataMissionPointerArray output{0xb000, 1, {0xdead}};
    Check(pal4::GameDataMissionAssignment::Refresh(
              0x5000, output, runtime) == 77,
          "assignment returns temporary allocator result");
    Check(output.records == std::vector<std::uint32_t>{
                                0x9000 + 68, 0x9000 + 4 * 68, 0xdead} &&
              output.capacity == 3 && output.begin_address != 0xb000,
          "highest unlocked records fill new front slots and preserve tail");
    Check(runtime.allocations.back() == 3 &&
              runtime.freed[runtime.freed.size() - 1] == 0xb000 &&
              runtime.released.size() == 1 &&
              runtime.game_state_calls == 7,
          "output growth frees old storage and queries PALGameIV per record");
}

void CheckReusableAndEmpty() {
    Runtime runtime;
    runtime.SetRecords({{0, 5}, {1, 2}});
    pal4::GameDataMissionPointerArray output{0xb000, 8, {0xdead}};
    Check(pal4::GameDataMissionAssignment::Refresh(
              0x5000, output, runtime) == 77 &&
              output.begin_address == 0xb000 && output.capacity == 8 &&
              output.records == std::vector<std::uint32_t>{
                                    0, 0x9000 + 68, 0xdead},
          "enough capacity inserts zero slots and leaves locked records out");
    Check(runtime.freed.empty() || runtime.freed.back() != 0xb000,
          "reused output storage is not freed");

    Runtime empty;
    empty.words[0x4158] = 0;
    pal4::GameDataMissionPointerArray retained{0xb000, 3, {0xdead}};
    Check(pal4::GameDataMissionAssignment::Refresh(
              0x5000, retained, empty) == 77 &&
              retained.records == std::vector<std::uint32_t>{0xdead} &&
              retained.begin_address == 0xb000 &&
              empty.released == std::vector<std::uint32_t>{0},
          "no PALGameIV slots preserves output and releases empty temporary");
}

void CheckFailureCleanup() {
    Runtime allocation;
    allocation.SetRecords({{0, 1}});
    allocation.fail_allocation_number = 2;
    pal4::GameDataMissionPointerArray output;
    bool threw = false;
    try {
        (void)pal4::GameDataMissionAssignment::Refresh(
            0x5000, output, allocation);
    } catch (const std::bad_alloc&) {
        threw = true;
    }
    Check(threw && output.records.empty() && output.begin_address == 0 &&
              allocation.released.size() == 1 &&
              allocation.released[0] != 0,
          "output allocation failure releases collected temporary pointers");

    Runtime level;
    level.SetRecords({{0, 1}});
    level.throw_on_level = true;
    pal4::GameDataMissionPointerArray existing{0xb000, 4, {0xdead}};
    threw = false;
    try {
        (void)pal4::GameDataMissionAssignment::Refresh(
            0x5000, existing, level);
    } catch (const std::runtime_error& error) {
        threw = std::string(error.what()) == "level failed";
    }
    Check(threw && level.released.size() == 1 &&
              existing.records == std::vector<std::uint32_t>{0, 0, 0xdead},
          "level exception releases temporary storage after slot insertion");
}

}  // namespace

int main() {
    try {
        CheckSelectionAndGrowth();
        CheckReusableAndEmpty();
        CheckFailureCleanup();
        std::cout << "game_data_mission_assignment_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
