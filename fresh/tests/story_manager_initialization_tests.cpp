#include "pal4/story_data_table.h"
#include "pal4/story_manager_binding.h"

#include <array>
#include <cstdint>
#include <initializer_list>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct BindingRuntime final : pal4::IStoryManagerBindingRuntime {
    std::vector<std::string> calls;
    std::array<std::uint32_t, 3> found{0x8100, 0x8200, 0x8300};
    std::map<std::uint32_t, std::uint32_t> words;
    std::string error;

    std::uint8_t RegisterHandler(std::uint32_t system,
                                 std::string_view name,
                                 std::uint32_t callback) override {
        calls.push_back("register:" + std::to_string(system) + ":" +
                        std::string(name) + ":" + std::to_string(callback));
        return name == "GameQuestion" ? 0x5a : 0x11;
    }
    std::uint32_t FindTable(std::uint32_t system,
                            std::string_view name) override {
        calls.push_back("find:" + std::to_string(system) + ":" +
                        std::string(name));
        return found[name == "Story" ? 0 : name == "StoryDesc" ? 1 : 2];
    }
    void LogError(std::string_view message) override {
        error = message;
        calls.push_back("log");
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.push_back("read:" + std::to_string(address));
        return words[address];
    }
};

void CheckBinding() {
    BindingRuntime runtime;
    pal4::StoryManagerBinding manager(0x5000);
    Check(manager.RegisterHandlers(runtime) == 0x5a &&
              runtime.calls == std::vector<std::string>{
                  "register:20480:Story:5222384",
                  "register:20480:StoryDesc:5224976",
                  "register:20480:GameQuestion:5226944"},
          "all three table names and factory callbacks are registered in order");
    runtime.calls.clear();
    Check(manager.VerifyTables(runtime) &&
              manager.Handles() ==
                  std::array<std::uint32_t, 3>{0x8100, 0x8200, 0x8300} &&
              runtime.calls == std::vector<std::string>{
                  "find:20480:Story", "find:20480:StoryDesc",
                  "find:20480:GameQuestion"},
          "verification stores all three table handles");

    for (std::size_t missing = 0; missing < 3; ++missing) {
        BindingRuntime failed;
        failed.found[missing] = 0;
        pal4::StoryManagerBinding other(0x5000);
        try {
            static_cast<void>(other.VerifyTables(failed));
            Check(false, "missing table must throw");
        } catch (const std::runtime_error& exception) {
            constexpr std::array<std::string_view, 3> names = {
                "Story", "StoryDesc", "GameQuestion"};
            const auto expected =
                "ERROR [ palDBStoryManager::verifyDataTable < the table " +
                std::string(names[missing]) + "  doesn't exist>]";
            Check(exception.what() == expected && failed.error == expected &&
                      failed.calls.size() == missing + 2 &&
                      failed.calls.back() == "log" &&
                      other.Handles()[missing] == 0,
                  "first missing table logs and throws its exact message");
        }
    }
}

void CheckIndexLookup() {
    BindingRuntime runtime;
    pal4::StoryManagerBinding manager(0x5000);
    runtime.words[0x5004] = 0x7000;
    runtime.words[0x7008] = 0x8000;
    runtime.words[0x700c] = 0x8018;
    runtime.words[0x7060] = 0x9000;
    runtime.words[0x8000] = static_cast<std::uint32_t>(-3);
    runtime.words[0x8004] = 2;
    runtime.words[0x8008] = 5;
    runtime.words[0x800c] = 0;
    runtime.words[0x8010] = 5;
    runtime.words[0x8014] = 1;
    Check(manager.FindStory(-3, runtime) == 0x9000 + 72 &&
              manager.FindStory(5, runtime) == 0x9000 &&
              manager.FindStory(4, runtime) == 0 &&
              manager.FindStory(6, runtime) == 0,
          "signed lower bound finds the first equal index and rejects gaps");
    runtime.words[0x700c] = 0x8000;
    Check(manager.FindStory(5, runtime) == 0,
          "empty index yields no record");
}

struct TableRuntime final : pal4::IStoryDataTableRuntime {
    std::uint32_t allocation = 0x9000;
    std::vector<std::string> calls;
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;

    std::uint32_t Allocate(std::uint32_t size) override {
        calls.push_back("allocate:" + std::to_string(size));
        return allocation;
    }
    void ConstructBase(std::uint32_t object,
                       std::uint32_t name) override {
        calls.push_back("base:" + std::to_string(object) + ":" +
                        std::to_string(name));
    }
    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override {
        calls.push_back("byte:" + std::to_string(address));
        bytes[address] = value;
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        calls.push_back("word:" + std::to_string(address));
        words[address] = value;
    }
};

void CheckTableCreate() {
    TableRuntime runtime;
    Check(pal4::StoryDataTable::Create(0x123456ab, runtime) == 0x9000 &&
              runtime.calls[0] == "allocate:140" &&
              runtime.calls[1] == "base:36864:305419947" &&
              runtime.calls.size() == 15 &&
              runtime.words[0x9000] == 0x84474c,
          "table allocation, base constructor, field initialization and vtable");
    for (const auto offset : {92u, 108u, 124u}) {
        Check(runtime.bytes[0x9000 + offset] == 0xab,
              "array seed byte preserves low name byte");
        for (std::uint32_t word = 4; word <= 12; word += 4)
            Check(runtime.words[0x9000 + offset + word] == 0,
                  "all three pointer triplets are zeroed");
    }
    TableRuntime failed;
    failed.allocation = 0;
    Check(pal4::StoryDataTable::Create(0x1234, failed) == 0 &&
              failed.calls == std::vector<std::string>{"allocate:140"},
          "allocation failure leaves base and fields untouched");
}

}  // namespace

int main() {
    try {
        CheckBinding();
        CheckIndexLookup();
        CheckTableCreate();
        std::cout << "story_manager_initialization_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
