#include "pal4/world_map_visibility.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(const bool value, const char* message) {
    if (!value) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IWorldMapVisibilityRuntime {
    static constexpr std::uint32_t kFrame = 0x5000u;
    std::map<std::uint32_t, std::uint32_t> words{
        {0x8b1c70u, 0x9000u}, {kFrame + 0x24u, 0x5100u},
        {kFrame + 0x238u, 0x5200u}, {kFrame + 0x26cu, 42u},
        {kFrame + 0x270u, 42u},
        {0x6000u + 0x30u, 1u}, {0x6000u + 8u, 0x7000u}};
    std::map<std::uint32_t, std::string> strings{{0x7000u, "m17"}};
    std::vector<std::string> calls;
    std::uint32_t temp_name = 0xa000u;
    bool throw_refresh = false;

    Runtime() {
        for (std::uint32_t i = 0; i < 21u; ++i) {
            words[kFrame + 0x98u + i * 0x10u] = 0x8000u + i;
            words[kFrame + 0x40u + i * 4u] = 0x5300u + i;
            strings[0x8000u + i] = "Q" + std::to_string(i);
        }
        strings[0x8005u] = "M17";
    }
    void SetParent(const std::uint32_t frame, const std::uint8_t visible,
                   const std::uint32_t parent) override {
        Check(frame == kFrame && parent == 0x1201u && visible <= 2u,
              "parent arguments");
        calls.emplace_back("parent");
    }
    std::uint32_t FrameManager() override {
        calls.emplace_back("manager");
        return 0x4000u;
    }
    std::uint32_t ReadWord(const std::uint32_t address) override {
        calls.emplace_back("read");
        return words.at(address);
    }
    void SetCursor(const std::uint32_t slot,
                   const std::uint32_t cursor) override {
        Check(slot == 0x4114u && cursor == 0x9000u,
              "default cursor slot and pointer");
        calls.emplace_back("cursor");
    }
    void InitializeButtons(const std::uint32_t frame) override {
        Check(frame == kFrame, "button frame");
        calls.emplace_back("buttons");
    }
    void SetModal(const std::uint32_t window,
                  const bool modal) override {
        Check(window == 0x5100u, "modal root");
        calls.emplace_back(modal ? "modal on" : "modal off");
    }
    void SetVisible(const std::uint32_t window,
                    const bool visible) override {
        Check(window == 0x5200u, "highlight window");
        calls.emplace_back(visible ? "highlight on" : "highlight off");
    }
    std::uint32_t ArenaManager() override {
        calls.emplace_back("arena");
        return 0x6000u;
    }
    std::uint32_t CopyArenaName(const std::uint32_t source,
                                const std::uint8_t flag) override {
        Check(source == 0x7000u && flag == 1u,
              "arena name and initial text flag");
        calls.emplace_back("copy name");
        if (temp_name != 0u) strings[temp_name] = strings.at(source);
        return temp_name;
    }
    bool NamesEqualIgnoreCase(const std::uint32_t first,
                              const std::uint32_t second) override {
        calls.emplace_back("compare");
        auto left = first == 0u ? std::string{} : strings.at(first);
        auto right = second == 0u ? std::string{} : strings.at(second);
        std::transform(left.begin(), left.end(), left.begin(),
                       [](unsigned char c) { return std::tolower(c); });
        std::transform(right.begin(), right.end(), right.begin(),
                       [](unsigned char c) { return std::tolower(c); });
        return left == right;
    }
    std::uint32_t GetPosition(const std::uint32_t window) override {
        Check(window == 0x5305u, "matched location window");
        calls.emplace_back("get position");
        return 0xb000u;
    }
    void SetPosition(const std::uint32_t window,
                     const std::uint32_t position) override {
        Check(window == 0x5200u && position == 0xb000u,
              "highlight position");
        calls.emplace_back("set position");
    }
    void RefreshMouse(const std::uint32_t mouse) override {
        Check(mouse == kFrame + 0x28u, "mouse helper address");
        calls.emplace_back("refresh mouse");
        if (throw_refresh) throw std::runtime_error("refresh mouse");
    }
    void ReleaseArenaName(const std::uint32_t name) override {
        Check(name == temp_name, "temporary name release");
        calls.emplace_back("release name");
    }
    void WriteWord(const std::uint32_t address,
                   const std::uint32_t value) override {
        calls.emplace_back("write");
        words[address] = value;
    }
};

void TestHide() {
    Runtime runtime;
    pal4::WorldMapVisibility::Update(Runtime::kFrame, 0u, 0x1201u,
                                      runtime);
    Check(runtime.calls == std::vector<std::string>{
              "parent", "manager", "read", "cursor", "read",
              "modal off", "write"} &&
              runtime.words.at(Runtime::kFrame + 0x270u) == 0xffffffffu,
          "hide resets modal state and trailing index");
}

void TestMissingArena() {
    Runtime runtime;
    runtime.words[0x6030u] = 0u;
    pal4::WorldMapVisibility::Update(Runtime::kFrame, 1u, 0x1201u,
                                      runtime);
    Check(runtime.words.at(Runtime::kFrame + 0x270u) == 42u &&
              runtime.calls.back() == "read" &&
              std::find(runtime.calls.begin(), runtime.calls.end(),
                        "copy name") == runtime.calls.end(),
          "missing arena returns before name and trailing reset");
}

void TestMatchedLocation() {
    Runtime runtime;
    pal4::WorldMapVisibility::Update(Runtime::kFrame, 2u, 0x1201u,
                                      runtime);
    Check(runtime.words.at(Runtime::kFrame + 0x26cu) == 5u &&
              runtime.words.at(Runtime::kFrame + 0x270u) == 0xffffffffu &&
              std::count(runtime.calls.begin(), runtime.calls.end(),
                         "compare") == 6 &&
              runtime.calls[runtime.calls.size() - 3u] == "refresh mouse" &&
              runtime.calls[runtime.calls.size() - 2u] == "release name" &&
              runtime.calls.back() == "write",
          "matched location highlights, refreshes and releases");
}

void TestNoMatchAndAllocationFailure() {
    Runtime runtime;
    runtime.strings[0x7000u] = "unknown";
    pal4::WorldMapVisibility::Update(Runtime::kFrame, 1u, 0x1201u,
                                      runtime);
    Check(std::count(runtime.calls.begin(), runtime.calls.end(),
                     "compare") == 21 &&
              runtime.words.at(Runtime::kFrame + 0x26cu) == 42u &&
              std::find(runtime.calls.begin(), runtime.calls.end(),
                        "highlight on") == runtime.calls.end(),
          "unmatched arena scans all names without selecting");

    Runtime failed;
    failed.temp_name = 0u;
    pal4::WorldMapVisibility::Update(Runtime::kFrame, 1u, 0x1201u,
                                      failed);
    Check(std::count(failed.calls.begin(), failed.calls.end(),
                     "compare") == 21 &&
              failed.calls[failed.calls.size() - 2u] == "release name",
          "failed temporary allocation compares empty name");
}

void TestExceptionCleanup() {
    Runtime runtime;
    runtime.throw_refresh = true;
    bool caught = false;
    try {
        pal4::WorldMapVisibility::Update(Runtime::kFrame, 1u,
                                          0x1201u, runtime);
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught && runtime.calls.back() == "release name" &&
              runtime.words.at(Runtime::kFrame + 0x270u) == 42u,
          "refresh failure releases temporary name before rethrow");
}

}  // namespace

int main() {
    TestHide();
    TestMissingArena();
    TestMatchedLocation();
    TestNoMatchAndAllocationFailure();
    TestExceptionCleanup();
    return EXIT_SUCCESS;
}
