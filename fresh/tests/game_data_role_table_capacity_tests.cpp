#include "pal4/game_data_role_table_capacity.h"

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

struct Runtime final : pal4::IGameDataRoleTableCapacityRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;
    std::vector<std::uint32_t> allocations;
    std::size_t next_allocation = 0;

    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
        calls.push_back("write:" + std::to_string(address));
    }
    std::uint32_t Allocate(std::uint32_t bytes) override {
        calls.push_back("allocate:" + std::to_string(bytes));
        return allocations.at(next_allocation++);
    }
    void Free(std::uint32_t address) override {
        calls.push_back("free:" + std::to_string(address));
    }
    void CopyRecord(std::uint32_t destination,
                    std::uint32_t source) override {
        calls.push_back("record:" + std::to_string(destination) + ":" +
                        std::to_string(source));
    }
    void DestroyRecord(std::uint32_t address) override {
        calls.push_back("destroy:" + std::to_string(address));
    }
    void CopyIndex(std::uint32_t destination,
                   std::uint32_t source) override {
        calls.push_back("index:" + std::to_string(destination) + ":" +
                        std::to_string(source));
    }
};

Runtime Base() {
    Runtime runtime;
    runtime.words[0x1060] = 0x2000;
    runtime.words[0x1064] = 0x2000 + 640;
    runtime.words[0x1068] = 0x2000 + 640;
    runtime.words[0x1008] = 0x3000;
    runtime.words[0x100c] = 0x3000 + 16;
    runtime.words[0x1010] = 0x3000 + 16;
    return runtime;
}

std::size_t At(const Runtime& runtime, const std::string& call) {
    const auto it = std::find(runtime.calls.begin(), runtime.calls.end(), call);
    Check(it != runtime.calls.end(), "expected call missing");
    return static_cast<std::size_t>(it - runtime.calls.begin());
}

void CheckNoGrowth() {
    auto runtime = Base();
    Check(pal4::GameDataRoleTableCapacity::Reserve(0x1000, 2, runtime) == 2 &&
              runtime.calls.empty(),
          "matching capacity leaves both arrays untouched and returns count");
}

void CheckRecordGrowth() {
    auto runtime = Base();
    runtime.words[0x1010] = 0x3000 + 32;
    runtime.allocations = {0x4000};
    Check(pal4::GameDataRoleTableCapacity::Reserve(0x1000, 3, runtime) == 4 &&
              runtime.words[0x1060] == 0x4000 &&
              runtime.words[0x1064] == 0x4000 + 640 &&
              runtime.words[0x1068] == 0x4000 + 960 &&
              runtime.words[0x1008] == 0x3000,
          "record-only growth preserves live count and index storage");
    Check(At(runtime, "allocate:960") <
                  At(runtime, "record:16384:8192") &&
              At(runtime, "record:16704:8512") <
                  At(runtime, "destroy:8192") &&
              At(runtime, "destroy:8512") <
                  At(runtime, "free:8192"),
          "deep-copy each record before destroying and freeing old storage");
}

void CheckIndexGrowth() {
    auto runtime = Base();
    runtime.words[0x1068] = 0x2000 + 1280;
    runtime.allocations = {0x5000};
    Check(pal4::GameDataRoleTableCapacity::Reserve(0x1000, 3, runtime) ==
              0x5000 + 16 &&
              runtime.words[0x1008] == 0x5000 &&
              runtime.words[0x100c] == 0x5000 + 16 &&
              runtime.words[0x1010] == 0x5000 + 24 &&
              runtime.words[0x1060] == 0x2000,
          "index-only growth preserves two live pairs");
    Check(At(runtime, "allocate:24") <
                  At(runtime, "index:20480:12288") &&
              At(runtime, "index:20488:12296") <
                  At(runtime, "free:12288"),
          "index entries copied before old buffer is freed");
}

void CheckBothAndEmpty() {
    auto runtime = Base();
    runtime.allocations = {0x4000, 0x5000};
    Check(pal4::GameDataRoleTableCapacity::Reserve(0x1000, 3, runtime) ==
              0x5000 + 16 &&
              runtime.words[0x1060] == 0x4000 &&
              runtime.words[0x1008] == 0x5000 &&
              At(runtime, "free:8192") < At(runtime, "allocate:24"),
          "record capacity grows before index capacity");

    Runtime empty;
    empty.allocations = {0x6000, 0x7000};
    Check(pal4::GameDataRoleTableCapacity::Reserve(0x1000, 1, empty) ==
              0x7000 && empty.words[0x1060] == 0x6000 &&
              empty.words[0x1064] == 0x6000 &&
              empty.words[0x1068] == 0x6000 + 320 &&
              empty.words[0x1008] == 0x7000 &&
              empty.words[0x100c] == 0x7000 &&
              empty.words[0x1010] == 0x7000 + 8 &&
              At(empty, "free:0") < At(empty, "allocate:8"),
          "empty arrays allocate independent first capacities");
}

}  // namespace

int main() {
    try {
        CheckNoGrowth();
        CheckRecordGrowth();
        CheckIndexGrowth();
        CheckBothAndEmpty();
        std::cout << "game_data_role_table_capacity_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
