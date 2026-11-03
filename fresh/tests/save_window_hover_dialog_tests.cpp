#include "pal4/save_window_hover_dialog.h"

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

void Check(bool ok, const char* message) {
    if (!ok) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::SaveWindowHoverDialogRuntimeStub {
    std::vector<std::string> calls;
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::uint32_t item{0x7000u};
    std::uint32_t allocation{0xa000u};
    bool fail_show{};

    std::uint32_t HoverItem(std::uint32_t list) override {
        Check(list == 0x5000u, "hover item queried from List");
        calls.emplace_back("hover-item");
        return item;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        Check(address == 0x71ecu, "item occupancy at +0x1ec");
        calls.emplace_back("occupied");
        return words.at(address);
    }
    std::uint32_t AllocateCallback(std::size_t bytes) override {
        Check(bytes == 0x18u, "save callback is 0x18 bytes");
        calls.emplace_back("allocate");
        return allocation;
    }
    void WriteCallbackWord(std::uint32_t address,
                           std::uint32_t value) override {
        words[address] = value;
        calls.emplace_back("write");
    }
    std::string TriggerItemText(std::uint32_t index) override {
        Check(index == (words.at(0x71ecu) == 0u ? 231u : 109u),
              "occupied and empty save prompts use distinct item IDs");
        calls.emplace_back("text");
        return index == 109u ? "overwrite" : "new save";
    }
    std::uint32_t FrameManager() override {
        calls.emplace_back("manager");
        return 0x8000u;
    }
    void CreateCommonDialog(std::uint32_t manager) override {
        Check(manager == 0x8000u, "common dialog uses manager");
        calls.emplace_back("create-dialog");
    }
    void ShowDialog(std::string_view text, float width,
                    float height, std::uint32_t mode,
                    std::uint32_t callback) override {
        Check(text == (words.at(0x71ecu) == 0u ? "new save" : "overwrite") &&
                  width == 0.3f && height == 0.4f && mode == 3u &&
                  callback == allocation,
              "dialog receives prompt, dimensions, mode and callback");
        calls.emplace_back("show-dialog");
        if (fail_show) throw std::runtime_error("show");
    }
    void DeleteCallback(std::uint32_t callback,
                        std::uint32_t flags) override {
        Check(callback == 0xa000u && flags == 1u,
              "local callback deleted with flag one");
        calls.emplace_back("delete");
    }
};

void CheckBranch(std::uint32_t occupied) {
    pal4::SaveWindowState state{};
    state.Write(0x54u, 0x5000u);
    Runtime runtime;
    runtime.words[0x71ecu] = occupied;
    Check(pal4::SaveWindowHoverDialog::Display(
              state, 0x6000u, runtime) == 1u &&
              runtime.words.at(0xa000u) == 0x8437b8u &&
              runtime.words.at(0xa008u) == 0x44d9b0u &&
              runtime.words.at(0xa00cu) == 0u &&
              runtime.words.at(0xa010u) == 0x6000u &&
              runtime.calls == std::vector<std::string>{
                  "hover-item", "occupied", "allocate", "write", "write",
                  "write", "write", "text", "manager", "create-dialog",
                  "show-dialog", "delete"},
          "save dialog branch builds callback and releases local owner");
}

void CheckGatesAndFailure() {
    pal4::SaveWindowState state{};
    state.Write(0x54u, 0x5000u);
    Runtime runtime;
    runtime.item = 0u;
    Check(pal4::SaveWindowHoverDialog::Display(
              state, 0x6000u, runtime) == 1u &&
              runtime.calls == std::vector<std::string>{"hover-item"},
          "no hover item consumes event without creating dialog");
    runtime = Runtime{};
    runtime.words[0x71ecu] = 0u;
    runtime.allocation = 0u;
    Check(pal4::SaveWindowHoverDialog::Display(
              state, 0x6000u, runtime) == 1u &&
              runtime.calls.back() == "show-dialog" &&
              runtime.words.size() == 1u,
          "null callback allocation still opens dialog");
    runtime = Runtime{};
    runtime.words[0x71ecu] = 1u;
    runtime.fail_show = true;
    try {
        (void)pal4::SaveWindowHoverDialog::Display(
            state, 0x6000u, runtime);
        Check(false, "show failure propagates");
    } catch (const std::runtime_error&) {
        Check(runtime.calls.back() == "delete",
              "show failure releases callback");
    }
}

}  // namespace

int main() {
    CheckBranch(0u);
    CheckBranch(1u);
    CheckGatesAndFailure();
    std::cout << "Save window hover dialog tests passed\n";
}
