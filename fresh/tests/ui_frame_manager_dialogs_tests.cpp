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

struct Runtime final : pal4::IUiFrameManagerDialogRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> events;
    std::uint32_t allocation = 0xa000u;
    bool throw_construct = false;
    bool mutate_on_release = false;

    std::uint32_t ReadWord(std::uint32_t address) override {
        events.emplace_back("read");
        return words[address];
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        events.emplace_back("write");
        words[address] = value;
    }
    std::uint32_t Allocate(std::uint32_t bytes) override {
        Check(bytes == 0x60u || bytes == 0x50u || bytes == 0xa4u,
              "one of the three IDA allocation sizes");
        events.emplace_back("allocate " + std::to_string(bytes));
        return allocation;
    }
    std::uint32_t Construct(pal4::UiCachedFrameKind kind,
                            std::uint32_t memory,
                            const char* name) override {
        Check(memory == 0xa000u, "dialog allocation passed to constructor");
        if (kind == pal4::UiCachedFrameKind::Common)
            Check(name && std::string(name) == "CommonDialog",
                  "common dialog resource name");
        if (kind == pal4::UiCachedFrameKind::YesNo)
            Check(name && std::string(name) == "CommonDialog2",
                  "yes/no dialog resource name");
        if (kind == pal4::UiCachedFrameKind::Select)
            Check(name == nullptr, "select dialog has no name argument");
        events.emplace_back("construct");
        if (throw_construct) throw std::runtime_error("construct failed");
        return 0xb000u;
    }
    void FreeAfterConstructionFailure(std::uint32_t memory) override {
        Check(memory == 0xa000u, "SEH cleanup frees the allocation");
        events.emplace_back("free");
    }
    void ReleaseCommonResources(std::uint32_t dialog) override {
        events.emplace_back("release");
        if (mutate_on_release) words[0x51acu] = dialog + 1u;
    }
    void CleanupSelectCallback(std::uint32_t dialog) override {
        events.emplace_back("callback");
        if (mutate_on_release) words[0x51ccu] = dialog + 1u;
    }
};

void CheckCommon() {
    Runtime created;
    created.mutate_on_release = true;
    Check(pal4::UiFrameManagerSingleton::CreateCommonDialog(
              0x5000u, created) == 0xb001u &&
              created.events == std::vector<std::string>{
                  "read", "allocate 96", "construct", "write", "read",
                  "release", "read"},
          "common dialog constructs, releases and rereads cache");

    Runtime cached;
    cached.words[0x51acu] = 0xc000u;
    Check(pal4::UiFrameManagerSingleton::CreateCommonDialog(
              0x5000u, cached) == 0xc000u &&
              cached.events == std::vector<std::string>{
                  "read", "read", "release", "read"},
          "cached common dialog still releases resources");

    Runtime missing;
    missing.allocation = 0u;
    Check(pal4::UiFrameManagerSingleton::CreateCommonDialog(
              0x5000u, missing) == 0u &&
              missing.events == std::vector<std::string>{
                  "read", "allocate 96", "write", "read", "release", "read"},
          "common resource release is unconditional on allocation failure");
}

void CheckYesNoAndSelect() {
    Runtime yes_no;
    Check(pal4::UiFrameManagerSingleton::CreateYesNoDialog(
              0x5000u, yes_no) == 0xb000u &&
              yes_no.words[0x51b0u] == 0xb000u &&
              yes_no.events == std::vector<std::string>{
                  "read", "allocate 80", "construct", "write", "read"},
          "yes/no dialog size, name and cache");

    Runtime select;
    select.mutate_on_release = true;
    Check(pal4::UiFrameManagerSingleton::CreateSelectDialog(
              0x5000u, select) == 0xb001u &&
              select.events == std::vector<std::string>{
                  "read", "allocate 164", "construct", "write", "read",
                  "callback", "read"},
          "select dialog cleans callback and rereads cache");

    Runtime empty_select;
    empty_select.allocation = 0u;
    Check(pal4::UiFrameManagerSingleton::CreateSelectDialog(
              0x5000u, empty_select) == 0u &&
              empty_select.events == std::vector<std::string>{
                  "read", "allocate 164", "write", "read", "callback", "read"},
          "select callback cleanup is unconditional");
}

void CheckConstructionFailure() {
    Runtime failed;
    failed.throw_construct = true;
    bool caught = false;
    try {
        (void)pal4::UiFrameManagerSingleton::CreateYesNoDialog(
            0x5000u, failed);
    } catch (const std::runtime_error& error) {
        caught = std::string(error.what()) == "construct failed";
    }
    Check(caught && failed.events == std::vector<std::string>{
              "read", "allocate 80", "construct", "free"} &&
              failed.words[0x51b0u] == 0u,
          "constructor exception frees memory before cache write");
}

}  // namespace

int main() {
    try {
        CheckCommon();
        CheckYesNoAndSelect();
        CheckConstructionFailure();
        std::cout << "ui_frame_manager_dialogs_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
