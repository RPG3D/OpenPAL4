#include "pal4/renderware_vector2.h"
#include "pal4/world_map_click.h"
#include "pal4/world_map_fly_start.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

void Check(const bool value, const char* message) {
    if (!value) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct VectorRuntime final : pal4::IRenderWareVector2 {
    std::vector<std::string> calls;
    double Length(const float x, const float y) override {
        Check(std::abs(x) == 10.0f && std::abs(y) == 10.0f,
              "destination displacement");
        calls.emplace_back("length");
        return 14.0;
    }
    std::pair<float, float> Normalize(const float x,
                                       const float y) override {
        Check(std::abs(x) == 10.0f && std::abs(y) == 10.0f,
              "normalization input");
        calls.emplace_back("normalize");
        return {x / 10.0f, y / 10.0f};
    }
};

struct Runtime final : pal4::IWorldMapFlyStartRuntime {
    static constexpr std::uint32_t kFrame = 0x5000u;
    std::map<std::uint32_t, std::uint32_t> words{
        {kFrame + 0x270u, 5u}, {kFrame + 0x238u, 0x7000u},
        {kFrame + 0x268u, 0x3c23d70au},
        {0x8000u + 0x1a0u, 0x8100u}};
    std::map<std::uint32_t, float> floats;
    std::vector<std::string> calls;
    std::uint32_t disabled = 0u;
    std::uint32_t released_names = 0u;
    std::uint32_t storage = 0xb000u;
    bool fail_timer_add = false;
    bool fail_callback = false;
    std::pair<float, float> target{ -5.0f, -5.0f };
    std::string image_name;

    Runtime() {
        for (std::uint32_t i = 0u; i < 21u; ++i) {
            words[kFrame + 0x40u + i * 4u] = 0x6000u + i;
        }
    }
    std::uint32_t ReadWord(const std::uint32_t address) override {
        calls.emplace_back("read word");
        return words.at(address);
    }
    void WriteWord(const std::uint32_t address,
                   const std::uint32_t value) override {
        calls.emplace_back("write word");
        words[address] = value;
    }
    float ReadFloat(const std::uint32_t address) override {
        calls.emplace_back("read float");
        return floats.at(address);
    }
    void WriteFloat(const std::uint32_t address,
                    const float value) override {
        calls.emplace_back("write float");
        floats[address] = value;
    }
    bool IsVisible(const std::uint32_t window,
                   const bool inherited) override {
        Check(window >= 0x6000u && window <= 0x6014u && !inherited,
              "button visibility query");
        calls.emplace_back("visible");
        return (window - 0x6000u) % 2u == 0u;
    }
    void SetEnabled(const std::uint32_t window,
                    const bool enabled) override {
        Check(window >= 0x6000u && window <= 0x6014u && !enabled,
              "disable visible button only");
        calls.emplace_back("disable");
        ++disabled;
    }
    std::pair<float, float> GetPosition(
        const std::uint32_t window) override {
        calls.emplace_back("position");
        if (window == 0x6005u) return target;
        Check(window == 0x7000u, "marker position window");
        return {5.0f, 5.0f};
    }
    std::uint32_t CopyLocationName(const std::uint32_t record) override {
        Check(record == kFrame + 0x94u + 5u * 0x10u,
              "selected location name record");
        calls.emplace_back("copy name");
        return 0xa000u;
    }
    void ReleaseLocationName(const std::uint32_t name) override {
        Check(name == 0xa000u, "location name release handle");
        calls.emplace_back("release name");
        ++released_names;
    }
    std::uint32_t FrameManager() override {
        calls.emplace_back("manager");
        return 0x8000u;
    }
    std::uint32_t Allocate(const std::uint32_t bytes) override {
        Check(bytes == 32u, "flight callback size");
        calls.emplace_back("allocate");
        return storage;
    }
    std::uint32_t ConstructCallback(
        const std::uint32_t address, const std::uint32_t frame,
        const std::uint32_t function,
        const std::uint32_t name) override {
        Check(address == storage && frame == kFrame &&
                  function == 0x484c10u && name == 0xa000u,
              "flight callback arguments");
        calls.emplace_back("callback");
        if (fail_callback) throw std::runtime_error("callback");
        return 0xb004u;
    }
    void Free(const std::uint32_t address) override {
        Check(address == storage, "failed callback storage free");
        calls.emplace_back("free");
    }
    std::uint32_t ConstructTimerEntry(
        const std::uint32_t name_address,
        const std::uint32_t interval_bits,
        const std::uint32_t callback) override {
        Check(name_address == kFrame + 0x258u &&
                  interval_bits == 0x3c23d70au &&
                  callback == (storage == 0u ? 0u : 0xb004u),
              "timer entry name, interval and callback");
        calls.emplace_back("entry");
        return 0xc000u;
    }
    void AddTimerEntry(const std::uint32_t queue,
                       const std::uint32_t entry) override {
        Check(queue == 0x8100u && entry == 0xc000u,
              "flight timer queue insertion");
        calls.emplace_back("add timer");
        if (fail_timer_add) throw std::runtime_error("timer");
    }
    void DestroyTimerEntry(const std::uint32_t entry) override {
        Check(entry == 0xc000u, "temporary timer entry cleanup");
        calls.emplace_back("destroy entry");
    }
    std::uint32_t SequenceImage(const std::string_view name) override {
        calls.emplace_back("image");
        image_name = name;
        return 0xd000u;
    }
};

void TestDirection(const std::pair<float, float> target,
                   const std::string_view image,
                   const float expected_x,
                   const float expected_y) {
    Runtime runtime;
    runtime.target = target;
    VectorRuntime renderware;
    pal4::WorldMapFlyStart::Start(Runtime::kFrame, runtime, renderware);
    Check(runtime.disabled == 11u &&
              runtime.floats.at(Runtime::kFrame + 0x23cu) == 14.0f &&
              runtime.floats.at(Runtime::kFrame + 0x248u) == 5.0f &&
              runtime.floats.at(Runtime::kFrame + 0x24cu) == 5.0f &&
              runtime.floats.at(Runtime::kFrame + 0x240u) == expected_x &&
              runtime.floats.at(Runtime::kFrame + 0x244u) == expected_y &&
              runtime.words.at(Runtime::kFrame + 0x250u) == 0u &&
              runtime.words.at(Runtime::kFrame + 0x254u) == 0u,
          "flight geometry, speed and traveled reset");
    Check(renderware.calls == std::vector<std::string>{
              "length", "normalize"} &&
              runtime.released_names == 1u &&
              runtime.image_name == image &&
              runtime.words.at(0x7000u + 0x7d4u) == 0xd000u &&
              runtime.calls[runtime.calls.size() - 5u] == "release name" &&
              runtime.calls[runtime.calls.size() - 4u] == "read float" &&
              runtime.calls[runtime.calls.size() - 3u] == "read float" &&
              runtime.calls[runtime.calls.size() - 2u] == "image" &&
              runtime.calls.back() == "write word",
          "timer/name cleanup precedes directional image assignment");
}

void TestGateAndNullCallback() {
    Runtime runtime;
    VectorRuntime renderware;
    runtime.words[Runtime::kFrame + 0x270u] = 0xffffffffu;
    pal4::WorldMapFlyStart::Start(Runtime::kFrame, runtime, renderware);
    Check(runtime.calls == std::vector<std::string>{"read word"} &&
              renderware.calls.empty(),
          "negative destination index skips flight setup");

    runtime = Runtime{};
    runtime.storage = 0u;
    pal4::WorldMapFlyStart::Start(Runtime::kFrame, runtime, renderware);
    Check(runtime.released_names == 1u &&
              runtime.image_name == "flya" &&
              std::find(runtime.calls.begin(), runtime.calls.end(),
                        "callback") == runtime.calls.end(),
          "null callback allocation still queues timer entry");
}

void TestExceptionalCleanup() {
    for (const bool callback_failure : {false, true}) {
        Runtime runtime;
        VectorRuntime renderware;
        runtime.fail_callback = callback_failure;
        runtime.fail_timer_add = !callback_failure;
        bool caught = false;
        try {
            pal4::WorldMapFlyStart::Start(Runtime::kFrame, runtime,
                                          renderware);
        } catch (const std::runtime_error&) {
            caught = true;
        }
        Check(caught && runtime.released_names == 1u &&
                  runtime.calls.back() == "release name" &&
                  std::find(runtime.calls.begin(), runtime.calls.end(),
                            callback_failure ? "free" : "destroy entry") !=
                      runtime.calls.end(),
              "callback and timer failures unwind owned resources");
    }
}

void TestDialogResultComposition() {
    Runtime runtime;
    VectorRuntime renderware;
    Check(pal4::WorldMapClick::OnDialogResult(
              Runtime::kFrame, 1u, runtime, renderware) == 1u &&
              runtime.image_name == "flya" &&
              runtime.disabled == 11u &&
              renderware.calls.size() == 2u,
          "confirmation starts reconstructed flight");

    Runtime cancelled;
    VectorRuntime unused_renderware;
    Check(pal4::WorldMapClick::OnDialogResult(
              Runtime::kFrame, 2u, cancelled, unused_renderware) == 1u &&
              cancelled.words.at(Runtime::kFrame + 0x270u) ==
                  0xffffffffu &&
              cancelled.calls == std::vector<std::string>{"write word"} &&
              unused_renderware.calls.empty(),
          "cancellation resets destination without starting flight");
}

}  // namespace

int main() {
    TestDirection({-5.0f, -5.0f}, "flya", -0.1f, -0.1f);
    TestDirection({15.0f, -5.0f}, "flyb", 0.1f, -0.1f);
    TestDirection({-5.0f, 15.0f}, "flyc", -0.1f, 0.1f);
    TestDirection({15.0f, 15.0f}, "flyd", 0.1f, 0.1f);
    TestGateAndNullCallback();
    TestExceptionalCleanup();
    TestDialogResultComposition();
    return EXIT_SUCCESS;
}
