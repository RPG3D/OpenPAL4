#include "pal4/ui_frame_manager_singleton.h"

#include <array>
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

struct Runtime final : pal4::IUiFrameManagerCleanupRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> events;
    std::vector<std::uint32_t> destroyed;
    std::vector<std::uint32_t> freed;
    std::uint32_t end_reads = 0;
    bool extend_frames = false;
    bool replace_collection = false;
    bool throw_on_destroy = false;

    std::uint32_t ReadWord(std::uint32_t address) override {
        if (address == 0x5190u) {
            ++end_reads;
            if (extend_frames && end_reads == 2u)
                words[address] = 0x9008u;
        }
        return words[address];
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
    }
    void DisableEffects(std::uint32_t manager) override {
        Check(manager == 0x5000u, "effect disable receiver");
        events.emplace_back("disable");
    }
    void DestroyFrame(std::uint32_t frame,
                      std::uint32_t delete_flag) override {
        Check(delete_flag == 1u, "virtual destructor delete flag");
        events.emplace_back("frame");
        destroyed.push_back(frame);
        if (throw_on_destroy) throw std::runtime_error("destroy failed");
    }
    void CleanupCollectionElements(std::uint32_t collection) override {
        Check(collection == 0xb000u, "initial collection for element cleanup");
        events.emplace_back("elements");
    }
    void CleanupGameObjects(std::uint32_t collection) override {
        Check(collection == 0xb000u, "collection reread before object cleanup");
        events.emplace_back("objects");
        if (replace_collection) words[0x519cu] = 0xb001u;
    }
    void DestructCollection(std::uint32_t collection) override {
        Check(collection == (replace_collection ? 0xb001u : 0xb000u),
              "collection reread after object cleanup");
        events.emplace_back("collection");
    }
    void CleanupTimedEffects(std::uint32_t effects) override {
        Check(effects == 0xc000u, "timed-effect pointer");
        events.emplace_back("effects");
    }
    void DestructEmbeddedTree(std::uint32_t tree) override {
        Check(tree == 0x5114u, "embedded-tree address");
        events.emplace_back("tree");
    }
    std::uint32_t ClearSpinnerPool() override {
        events.emplace_back("spinners");
        return 0xcafeu;
    }
    void DestructGui(std::uint32_t gui) override {
        Check(gui == 0xd000u, "CEGUI pointer");
        events.emplace_back("gui");
    }
    std::uint32_t Free(std::uint32_t address) override {
        freed.push_back(address);
        events.emplace_back("free");
        return address == 0xd000u ? 0xd00du : 0u;
    }
};

void CheckFullCleanup() {
    Runtime runtime;
    constexpr std::array<std::uint32_t, 11> offsets{
        0x1c0u, 0x1c4u, 0x1a8u, 0x1acu, 0x1b0u, 0x1ccu,
        0x1d0u, 0x1b4u, 0x1b8u, 0x1bcu, 0x1c8u};
    for (std::uint32_t i = 0; i < offsets.size(); ++i)
        runtime.words[0x5000u + offsets[i]] = 0x6000u + i;
    runtime.words[0x518cu] = 0x9000u;
    runtime.words[0x5190u] = 0x9004u;
    runtime.words[0x9000u] = 0x7000u;
    runtime.words[0x9004u] = 0x7001u;
    runtime.words[0x519cu] = 0xb000u;
    runtime.words[0x51a0u] = 0xc000u;
    runtime.words[0x5198u] = 0xd000u;
    runtime.extend_frames = true;
    runtime.replace_collection = true;

    Check(pal4::UiFrameManagerSingleton::Cleanup(0x5000u, runtime) ==
              0xd00du, "final CEGUI free result is returned");
    std::vector<std::uint32_t> expected;
    for (std::uint32_t i = 0; i < offsets.size(); ++i) {
        expected.push_back(0x6000u + i);
        Check(runtime.words[0x5000u + offsets[i]] == 0u,
              "dedicated frame pointer cleared");
    }
    expected.push_back(0x7000u);
    expected.push_back(0x7001u);
    Check(runtime.destroyed == expected && runtime.end_reads >= 3u,
          "dedicated order and changing frame-array end");
    Check(runtime.words[0x5190u] == 0x9000u &&
              runtime.words[0x519cu] == 0u &&
              runtime.words[0x51a0u] == 0u &&
              runtime.words[0x5198u] == 0u,
          "array and owned pointers reset");
    Check(runtime.freed ==
              std::vector<std::uint32_t>{0xb001u, 0xc000u, 0xd000u},
          "reloaded collection, effects and GUI freed in order");
    Check(runtime.events.front() == "disable" &&
              runtime.events.back() == "free" &&
              runtime.events[runtime.events.size() - 3u] == "spinners" &&
              runtime.events[runtime.events.size() - 2u] == "gui",
          "effect disable and final shutdown order");
}

void CheckEmptyAndException() {
    Runtime empty;
    Check(pal4::UiFrameManagerSingleton::Cleanup(0x5000u, empty) ==
              0xcafeu, "empty path returns spinner-pool result");
    Check(empty.destroyed.empty() && empty.freed.empty() &&
              empty.events ==
                  std::vector<std::string>{"disable", "tree", "spinners"},
          "empty resources skip destruction and free");

    Runtime throwing;
    throwing.words[0x51c0u] = 0x6000u;
    throwing.throw_on_destroy = true;
    bool caught = false;
    try {
        (void)pal4::UiFrameManagerSingleton::Cleanup(0x5000u, throwing);
    } catch (const std::runtime_error& error) {
        caught = std::string(error.what()) == "destroy failed";
    }
    Check(caught && throwing.events ==
                        std::vector<std::string>{"disable", "frame"} &&
              throwing.words[0x51c0u] == 0x6000u,
          "destructor failure stops cleanup before clearing its field");
}

}  // namespace

int main() {
    try {
        CheckFullCleanup();
        CheckEmptyAndException();
        std::cout << "ui_frame_manager_cleanup_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
