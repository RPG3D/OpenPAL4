#include "pal4/ui_frame_manager_singleton.h"

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

struct Runtime final : pal4::IUiFrameManagerAccessRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> events;
    std::uint32_t allocated = 0xa000u;
    bool throw_construct = false;

    std::uint32_t ReadWord(std::uint32_t address) override {
        Check(address == 0x51a8u, "menu cache read address");
        events.emplace_back("read");
        return words[address];
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        Check(address == 0x51a8u, "menu cache write address");
        events.emplace_back("write");
        words[address] = value;
    }
    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override {
        events.emplace_back("byte");
        bytes[address] = value;
    }
    std::uint32_t Allocate(std::uint32_t size) override {
        Check(size == 0x58u, "menu allocation size");
        events.emplace_back("allocate");
        return allocated;
    }
    std::uint32_t ConstructMenu(std::uint32_t memory) override {
        Check(memory == 0xa000u, "menu construction address");
        events.emplace_back("construct");
        if (throw_construct) throw std::runtime_error("construct failed");
        return 0xb000u;
    }
    void FreeAfterConstructionFailure(std::uint32_t memory) override {
        Check(memory == 0xa000u, "SEH cleanup frees original block");
        events.emplace_back("free");
    }
};

}  // namespace

int main() {
    try {
        Runtime flag;
        Check(pal4::UiFrameManagerSingleton::SetMessageFlag(
                  0x5000u, 0xffu, flag) == 0xffu &&
                  flag.bytes[0x5221u] == 0xffu &&
                  flag.events == std::vector<std::string>{"byte"},
              "message flag write and return");

        Runtime cached;
        cached.words[0x51a8u] = 0x9000u;
        Check(pal4::UiFrameManagerSingleton::GetMenuWindow(
                  0x5000u, cached) == 0x9000u &&
                  cached.events == std::vector<std::string>{"read", "read"},
              "cached menu skips allocation");

        Runtime created;
        Check(pal4::UiFrameManagerSingleton::GetMenuWindow(
                  0x5000u, created) == 0xb000u &&
                  created.words[0x51a8u] == 0xb000u &&
                  created.events == std::vector<std::string>{
                      "read", "allocate", "construct", "write", "read"},
              "new menu is stored and reread");

        Runtime allocation_failed;
        allocation_failed.allocated = 0u;
        Check(pal4::UiFrameManagerSingleton::GetMenuWindow(
                  0x5000u, allocation_failed) == 0u &&
                  allocation_failed.events == std::vector<std::string>{
                      "read", "allocate", "write", "read"},
              "allocation failure caches zero without constructing");

        Runtime construction_failed;
        construction_failed.throw_construct = true;
        bool caught = false;
        try {
            (void)pal4::UiFrameManagerSingleton::GetMenuWindow(
                0x5000u, construction_failed);
        } catch (const std::runtime_error& error) {
            caught = std::string(error.what()) == "construct failed";
        }
        Check(caught && construction_failed.events ==
                            std::vector<std::string>{
                                "read", "allocate", "construct", "free"} &&
                  construction_failed.words[0x51a8u] == 0u,
              "constructor failure frees memory before cache write");
        std::cout << "ui_frame_manager_accessors_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
