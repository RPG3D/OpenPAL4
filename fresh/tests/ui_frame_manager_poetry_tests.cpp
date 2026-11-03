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

struct Runtime final : pal4::IUiFrameManagerPoetryRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> events;
    std::uint32_t allocation = 0xa000u;
    bool throw_format = false;
    bool throw_construct = false;

    std::uint32_t ReadWord(std::uint32_t address) override {
        Check(address == 0x51d0u, "poetry cache read address");
        events.emplace_back("read");
        return words[address];
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        Check(address == 0x51d0u, "poetry cache write address");
        events.emplace_back("write");
        words[address] = value;
    }
    void DestroyPoetry(std::uint32_t poetry,
                       std::uint32_t delete_flag) override {
        Check(poetry == 0x9000u && delete_flag == 1u,
              "old poetry window virtual destructor");
        events.emplace_back("destroy");
    }
    std::uint32_t InitializeName() override {
        events.emplace_back("initialize name");
        return 0x1100u;
    }
    std::uint32_t FormatName(std::uint32_t buffer,
                             const char* format,
                             std::int32_t index) override {
        Check(buffer == 0x1100u &&
                  std::string(format) == "PoetryWindow%d" &&
                  index == -257,
              "format uses signed full-width index and exact pattern");
        events.emplace_back("format name");
        if (throw_format) throw std::runtime_error("format failed");
        return 0x1200u;
    }
    void CleanupName(std::uint32_t buffer) override {
        Check(buffer == 0x1100u, "temporary name cleanup");
        events.emplace_back("cleanup name");
    }
    std::uint32_t Allocate(std::uint32_t bytes) override {
        Check(bytes == 0xb8u, "poetry object size");
        events.emplace_back("allocate");
        return allocation;
    }
    std::uint32_t ConstructPoetry(std::uint32_t memory,
                                  std::uint32_t name) override {
        Check(memory == 0xa000u && name == 0x1200u,
              "poetry constructor arguments");
        events.emplace_back("construct");
        if (throw_construct) throw std::runtime_error("construct failed");
        return 0xb000u;
    }
    void FreeAfterConstructionFailure(std::uint32_t memory) override {
        Check(memory == 0xa000u, "constructor SEH frees allocation");
        events.emplace_back("free");
    }
    std::uint32_t HidePoetry(std::uint32_t poetry) override {
        Check(poetry == 0xb000u || poetry == 0u,
              "hide receives create result even when null");
        events.emplace_back("hide");
        return 0xc000u;
    }
};

void CheckReplaceAndHide() {
    Runtime runtime;
    runtime.words[0x51d0u] = 0x9000u;
    Check(pal4::UiFrameManagerSingleton::CreateAndHidePoetryWindow(
              0x5000u, -257, runtime) == 0xc000u &&
              runtime.words[0x51d0u] == 0xb000u &&
              runtime.events == std::vector<std::string>{
                  "read", "destroy", "write", "initialize name",
                  "format name", "allocate", "construct", "write",
                  "cleanup name", "hide"},
          "old window replaced before formatting; hide follows cleanup");
    runtime.events.clear();
    Check(pal4::UiFrameManagerSingleton::GetPoetryWindow(
              0x5000u, runtime) == 0xb000u &&
              runtime.events == std::vector<std::string>{"read"},
          "getter reads the poetry cache only");
}

void CheckFailurePaths() {
    Runtime no_memory;
    no_memory.allocation = 0u;
    Check(pal4::UiFrameManagerSingleton::CreateAndHidePoetryWindow(
              0x5000u, -257, no_memory) == 0xc000u &&
              no_memory.events == std::vector<std::string>{
                  "read", "initialize name", "format name", "allocate",
                  "write", "cleanup name", "hide"},
          "failed allocation stores null and still hides null");

    Runtime format_failure;
    format_failure.throw_format = true;
    bool caught = false;
    try {
        (void)pal4::UiFrameManagerSingleton::CreatePoetryWindow(
            0x5000u, -257, format_failure);
    } catch (const std::runtime_error& error) {
        caught = std::string(error.what()) == "format failed";
    }
    Check(caught && format_failure.events == std::vector<std::string>{
              "read", "initialize name", "format name", "cleanup name"},
          "format exception releases initialized name");

    Runtime construction_failure;
    construction_failure.words[0x51d0u] = 0x9000u;
    construction_failure.throw_construct = true;
    caught = false;
    try {
        (void)pal4::UiFrameManagerSingleton::CreatePoetryWindow(
            0x5000u, -257, construction_failure);
    } catch (const std::runtime_error& error) {
        caught = std::string(error.what()) == "construct failed";
    }
    Check(caught && construction_failure.words[0x51d0u] == 0u &&
              construction_failure.events == std::vector<std::string>{
                  "read", "destroy", "write", "initialize name",
                  "format name", "allocate", "construct", "free",
                  "cleanup name"},
          "constructor failure frees allocation and temporary name");
}

}  // namespace

int main() {
    try {
        CheckReplaceAndHide();
        CheckFailurePaths();
        std::cout << "ui_frame_manager_poetry_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
