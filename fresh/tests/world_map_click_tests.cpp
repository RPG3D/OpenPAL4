#include "pal4/world_map_click.h"

#include <algorithm>
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

struct Runtime final : pal4::IWorldMapClickRuntime {
    static constexpr std::uint32_t kFrame = 0x5000u;
    static constexpr std::uint32_t kEvent = 0x7100u;
    std::map<std::uint32_t, std::uint32_t> words{
        {kFrame + 0x270u, 0xffffffffu},
        {kFrame + 0x238u, 0x6000u},
        {kEvent + 8u, 0x7200u},
        {0x7200u + 0x118u, 5u},
        {kFrame + 0x1e4u + 5u * 4u, 1u}};
    std::vector<std::string> calls;
    bool visible = true;
    bool fail_dialog = false;
    std::uint32_t callback_allocation = 0x8000u;
    std::uint32_t trigger_text = 0x9000u;
    std::uint32_t expected_trigger = 292u;
    std::uint32_t expected_mode = 3u;

    std::uint32_t ReadWord(const std::uint32_t address) override {
        calls.emplace_back("read");
        return words.at(address);
    }
    void WriteWord(const std::uint32_t address,
                   const std::uint32_t value) override {
        calls.emplace_back("write");
        words[address] = value;
    }
    bool IsVisible(const std::uint32_t window,
                   const bool inherited) override {
        Check(window == 0x6000u && !inherited,
              "highlight visibility arguments");
        calls.emplace_back("visible");
        return visible;
    }
    std::uint32_t GameDatabase() override {
        calls.emplace_back("database");
        return 0xa000u;
    }
    std::uint32_t TriggerText(const std::uint32_t collection,
                              const std::uint32_t trigger) override {
        Check(collection == 0xa034u && trigger == expected_trigger,
              "trigger collection and ID");
        calls.emplace_back("text");
        return trigger_text;
    }
    std::uint32_t EmptyText() override {
        calls.emplace_back("empty text");
        return 0x9001u;
    }
    std::uint32_t FrameManager() override {
        calls.emplace_back("manager");
        return 0xb000u;
    }
    std::uint32_t CreateCommonDialog(const std::uint32_t manager) override {
        Check(manager == 0xb000u, "dialog manager");
        calls.emplace_back("create dialog");
        return 0xc000u;
    }
    void ShowDialog(const std::uint32_t dialog,
                    const std::uint32_t text,
                    const std::uint32_t x_bits,
                    const std::uint32_t y_bits,
                    const std::uint32_t mode,
                    const std::uint32_t callback) override {
        Check(dialog == 0xc000u &&
                  text == (trigger_text == 0u ? 0x9001u : trigger_text) &&
                  x_bits == 0x3f000000u && y_bits == 0x3f000000u &&
                  mode == expected_mode &&
                  callback == (mode == 3u ? callback_allocation : 0u),
              "dialog position, mode and callback");
        calls.emplace_back("show dialog");
        if (fail_dialog) throw std::runtime_error("show dialog");
    }
    std::uint32_t Allocate(const std::uint32_t bytes) override {
        Check(bytes == 24u, "result callback allocation size");
        calls.emplace_back("allocate");
        return callback_allocation;
    }
    void ReleaseCallback(const std::uint32_t callback,
                         const std::uint32_t flags) override {
        Check(callback == callback_allocation && flags == 1u,
              "result callback release");
        calls.emplace_back("release");
    }
};

struct DialogResultRuntime final : pal4::IWorldMapDialogResultRuntime {
    std::vector<std::string> calls;
    void StartFly(const std::uint32_t frame) override {
        Check(frame == Runtime::kFrame, "confirmed flight frame");
        calls.emplace_back("start");
    }
    void WriteWord(const std::uint32_t address,
                   const std::uint32_t value) override {
        Check(address == Runtime::kFrame + 0x270u &&
                  value == 0xffffffffu,
              "cancelled flight index reset");
        calls.emplace_back("reset");
    }
};

void TestGates() {
    Runtime runtime;
    runtime.words[Runtime::kFrame + 0x270u] = 0u;
    Check(pal4::WorldMapClick::OnLocation(Runtime::kFrame,
                                          Runtime::kEvent, runtime) == 0u &&
              runtime.calls == std::vector<std::string>{"read"},
          "active flight rejects location click before UI access");
    runtime = Runtime{};
    runtime.visible = false;
    Check(pal4::WorldMapClick::OnLocation(Runtime::kFrame,
                                          Runtime::kEvent, runtime) == 1u &&
              runtime.calls == std::vector<std::string>{
                  "read", "read", "visible"},
          "hidden role image ignores click after visibility query");
}

void TestDisabledLocation() {
    Runtime runtime;
    runtime.words[Runtime::kFrame + 0x1e4u + 5u * 4u] = 2u;
    runtime.expected_trigger = 246u;
    runtime.expected_mode = 4u;
    Check(pal4::WorldMapClick::OnLocation(Runtime::kFrame,
                                          Runtime::kEvent, runtime) == 1u &&
              runtime.words.at(Runtime::kFrame + 0x270u) == 0xffffffffu &&
              std::find(runtime.calls.begin(), runtime.calls.end(),
                        "allocate") == runtime.calls.end() &&
              runtime.calls.back() == "show dialog",
          "disabled destination shows trigger 246 without callback");
}

void TestTravelDialog() {
    Runtime runtime;
    Check(pal4::WorldMapClick::OnLocation(Runtime::kFrame,
                                          Runtime::kEvent, runtime) == 1u &&
              runtime.words.at(Runtime::kFrame + 0x270u) == 5u &&
              runtime.words.at(0x8000u) == 0x843cc8u &&
              runtime.words.at(0x8008u) == 0x484db0u &&
              runtime.words.at(0x800cu) == 0u &&
              runtime.words.at(0x8010u) == Runtime::kFrame &&
              runtime.calls[runtime.calls.size() - 2u] == "show dialog" &&
              runtime.calls.back() == "release",
          "travel dialog stores index and owns result callback");

    runtime = Runtime{};
    runtime.callback_allocation = 0u;
    (void)pal4::WorldMapClick::OnLocation(Runtime::kFrame,
                                           Runtime::kEvent, runtime);
    Check(runtime.calls.back() == "show dialog" &&
              runtime.words.at(Runtime::kFrame + 0x270u) == 5u,
          "null callback still shows confirmation");

    runtime = Runtime{};
    runtime.trigger_text = 0u;
    (void)pal4::WorldMapClick::OnLocation(Runtime::kFrame,
                                           Runtime::kEvent, runtime);
    Check(std::find(runtime.calls.begin(), runtime.calls.end(),
                    "empty text") != runtime.calls.end(),
          "missing trigger text uses empty string");
}

void TestDialogFailure() {
    Runtime runtime;
    runtime.fail_dialog = true;
    bool caught = false;
    try {
        (void)pal4::WorldMapClick::OnLocation(Runtime::kFrame,
                                               Runtime::kEvent, runtime);
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught && runtime.calls.back() == "release",
          "dialog failure releases result callback");
}

void TestDialogResult() {
    DialogResultRuntime runtime;
    Check(pal4::WorldMapClick::OnDialogResult(Runtime::kFrame, 1u,
                                               runtime) == 1u &&
              runtime.calls == std::vector<std::string>{"start"},
          "confirmed destination starts flight");
    runtime.calls.clear();
    Check(pal4::WorldMapClick::OnDialogResult(Runtime::kFrame, 0u,
                                               runtime) == 1u &&
              pal4::WorldMapClick::OnDialogResult(Runtime::kFrame, 2u,
                                                   runtime) == 1u &&
              runtime.calls == std::vector<std::string>{"reset", "reset"},
          "all other dialog results cancel flight");
}

}  // namespace

int main() {
    TestGates();
    TestDisabledLocation();
    TestTravelDialog();
    TestDialogFailure();
    TestDialogResult();
    return EXIT_SUCCESS;
}
