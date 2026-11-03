#include "pal4/jigsaw_window_result.h"

#include <cstdint>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IJigsawWindowResultRuntime {
    static constexpr std::uint32_t frame = 0x1000u;
    static constexpr std::uint32_t manager = 0x2000u;
    static constexpr std::uint32_t text_window = 0x3000u;
    static constexpr std::uint32_t overlay = 0x4000u;
    static constexpr std::uint32_t callback = 0x5000u;
    static constexpr std::uint32_t queue = 0x6000u;

    std::vector<std::string> calls;
    std::unordered_map<std::uint32_t, std::uint32_t> words{
        {frame + 0x188u, text_window}, {manager + 0x1a0u, queue}};
    std::unordered_map<std::uint32_t, std::uint8_t> bytes;
    std::int32_t counter = 4;
    bool has_format = true;
    bool fail_overlay = false;
    bool fail_callback = false;
    bool throw_configure = false;
    bool throw_queue = false;
    std::uint32_t expected_trigger_id = 0xf5u;
    std::string image;
    std::string sound;
    std::string used_format;
    std::int32_t formatted_value = 0;
    std::u32string text;
    std::uint32_t queued_callback = 0xffffffffu;
    bool owns_callback = false;
    std::uint32_t released_overlay = 0u;
    std::uint32_t released_callback = 0u;
    std::string released_text;

    std::int32_t ReadCounter(std::uint32_t owner) override {
        Check(owner == frame, "counter frame");
        calls.emplace_back("counter");
        return counter;
    }
    std::optional<std::string_view> TriggerFormat(
        std::uint32_t id) override {
        Check(id == expected_trigger_id, "trigger ID");
        calls.emplace_back("trigger");
        if (!has_format) return std::nullopt;
        return "steps: %d";
    }
    std::string FormatText(std::string_view format,
                           std::int32_t value) override {
        used_format = format;
        formatted_value = value;
        calls.emplace_back("format");
        return "formatted";
    }
    std::string ConvertToUtf8(std::string_view source) override {
        Check(source == "formatted", "text conversion input");
        calls.emplace_back("utf8");
        return "\xe6\x88\x90\xe5\x8a\x9f";
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.emplace_back("read");
        return words.at(address);
    }
    void SetText(std::uint32_t window,
                 std::u32string_view value) override {
        Check(window == text_window, "step text target");
        text = value;
        calls.emplace_back("set-text");
    }
    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override {
        bytes[address] = value;
        calls.emplace_back("write-byte");
    }
    std::uint32_t FrameManager() override {
        calls.emplace_back("manager");
        return manager;
    }
    void PlaySound(std::string_view file,
                   std::uint32_t mode) override {
        Check(mode == 0u, "result sound mode");
        sound = file;
        calls.emplace_back("sound");
    }
    std::uint32_t CreateOverlay(std::uint32_t bytes,
                                std::uint32_t vtable) override {
        Check(bytes == 0x48u &&
                  vtable == pal4::JigsawWindowResult::kOverlayVtable,
              "overlay allocation layout");
        calls.emplace_back("create-overlay");
        return fail_overlay ? 0u : overlay;
    }
    void ConfigureOverlay(
        std::uint32_t target,
        const pal4::JigsawResultOverlaySpec& spec) override {
        Check(target == overlay && spec.width_bits == 0x43c80000u &&
                  spec.height_bits == 0x43960000u &&
                  spec.imageset == "zi0" &&
                  spec.image_scale_bits == 0x41a00000u &&
                  spec.font_field_bits == 0x42700000u &&
                  spec.font_name.empty() && spec.font_group == "system" &&
                  spec.font_size_bits == 0x3f800000u &&
                  spec.font_x_bits == 0x40000000u &&
                  spec.font_y_bits == 0x40400000u,
              "overlay constants");
        image = spec.image;
        calls.emplace_back("configure-overlay");
        if (throw_configure) throw std::runtime_error("configure failed");
    }
    void AddOverlay(std::uint32_t list,
                    std::uint8_t active,
                    std::uint32_t target) override {
        Check(list == manager + 0x1dcu && active == 1u &&
                  target == overlay, "overlay list insertion");
        calls.emplace_back("add-overlay");
    }
    void ReleaseOverlay(std::uint32_t target) override {
        released_overlay = target;
        calls.emplace_back("release-overlay");
    }
    std::uint32_t AllocateCallback(std::uint32_t bytes) override {
        Check(bytes == 0x18u, "close callback size");
        calls.emplace_back("allocate-callback");
        return fail_callback ? 0u : callback;
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
        calls.emplace_back("write-word");
    }
    void ReleaseCallback(std::uint32_t target) override {
        released_callback = target;
        calls.emplace_back("release-callback");
    }
    void QueueCloseEvent(std::uint32_t target,
                         std::string_view name,
                         std::uint32_t period,
                         std::uint32_t callback_value,
                         bool owns) override {
        Check(target == queue + 0xcu && name == "closeGame" &&
                  period == 0x40800000u, "close event layout");
        queued_callback = callback_value;
        owns_callback = owns;
        calls.emplace_back("queue");
        if (throw_queue) throw std::runtime_error("queue failed");
    }
    std::uint32_t ReleaseFormatted(std::string& source) override {
        released_text = source;
        calls.emplace_back("release-text");
        return 0x98765432u;
    }
};

void CheckResultBranches() {
    for (const auto success : {0u, 1u, 2u}) {
        Runtime runtime;
        runtime.has_format = success != 2u;
        const auto result = pal4::JigsawWindowResult::Show(
            Runtime::frame, static_cast<std::uint8_t>(success), runtime);
        Check(result == 0x98765432u &&
                  runtime.text == U"\u6210\u529f" &&
                  runtime.formatted_value == 4 &&
                  runtime.used_format == (success == 2u ? "" : "steps: %d") &&
                  runtime.bytes.at(Runtime::frame + 0x18cu) == 1u &&
                  runtime.bytes.at(Runtime::manager + 0x222u) == success &&
                  runtime.image == (success == 0u ? "gamelose" : "gamewin") &&
                  runtime.sound == (success == 0u ? "WA24.wav" : "WA23.wav") &&
                  runtime.released_text == "formatted",
              "result text, state, image and sound");
        Check(runtime.words.at(Runtime::callback) ==
                  pal4::JigsawWindowResult::kCallbackVtable &&
                  runtime.words.at(Runtime::callback + 4u) == Runtime::frame &&
                  runtime.words.at(Runtime::callback + 8u) ==
                      pal4::JigsawWindowResult::kCloseFunction &&
                  runtime.words.at(Runtime::callback + 0xcu) == 0u &&
                  runtime.bytes.at(Runtime::callback + 0x10u) == 0u &&
                  runtime.queued_callback == Runtime::callback &&
                  runtime.owns_callback &&
                  runtime.calls.back() == "release-text",
              "close callback fields and ownership transfer");
    }
}

void CheckFailures() {
    Runtime no_callback;
    no_callback.fail_callback = true;
    Check(pal4::JigsawWindowResult::Show(
              Runtime::frame, 1u, no_callback) == 0x98765432u &&
              no_callback.queued_callback == 0u &&
              !no_callback.owns_callback,
          "callback allocation failure still queues event");

    Runtime no_overlay;
    no_overlay.fail_overlay = true;
    bool caught = false;
    try {
        (void)pal4::JigsawWindowResult::Show(Runtime::frame, 0u, no_overlay);
    } catch (const std::bad_alloc&) {
        caught = true;
    }
    Check(caught && no_overlay.calls.back() == "release-text" &&
              no_overlay.queued_callback == 0xffffffffu,
          "overlay allocation failure releases text");

    Runtime bad_overlay;
    bad_overlay.throw_configure = true;
    caught = false;
    try {
        (void)pal4::JigsawWindowResult::Show(Runtime::frame, 0u, bad_overlay);
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught && bad_overlay.released_overlay == Runtime::overlay &&
              bad_overlay.calls.back() == "release-text",
          "overlay configuration failure releases both resources");

    Runtime bad_queue;
    bad_queue.throw_queue = true;
    caught = false;
    try {
        (void)pal4::JigsawWindowResult::Show(Runtime::frame, 1u, bad_queue);
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught && bad_queue.released_callback == Runtime::callback &&
              bad_queue.calls.back() == "release-text",
          "event insertion failure releases callback and text");
}

void CheckPuzzleResult() {
    for (const auto success : {0u, 1u}) {
        Runtime runtime;
        runtime.expected_trigger_id = 0xe9u;
        Check(pal4::PuzzleWindowResult::Show(
                  Runtime::frame, static_cast<std::uint8_t>(success),
                  runtime) == 0x98765432u &&
                  runtime.bytes.at(Runtime::frame + 0x190u) == 1u &&
                  !runtime.bytes.contains(Runtime::frame + 0x18cu) &&
                  runtime.bytes.at(Runtime::manager + 0x222u) == success &&
                  runtime.words.at(Runtime::callback) ==
                      pal4::PuzzleWindowResult::kCallbackVtable &&
                  runtime.words.at(Runtime::callback + 4u) == Runtime::frame &&
                  runtime.words.at(Runtime::callback + 8u) ==
                      pal4::JigsawWindowResult::kCloseFunction &&
                  runtime.image == (success == 0u ? "gamelose" : "gamewin") &&
                  runtime.sound == (success == 0u ? "WA24.wav" : "WA23.wav") &&
                  runtime.queued_callback == Runtime::callback &&
                  runtime.owns_callback,
              "puzzle result uses its own trigger, gate and callback vtable");
    }
    Runtime failing;
    failing.expected_trigger_id = 0xe9u;
    failing.throw_queue = true;
    bool caught = false;
    try {
        (void)pal4::PuzzleWindowResult::Show(Runtime::frame, 1u, failing);
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught && failing.released_callback == Runtime::callback &&
              failing.calls.back() == "release-text",
          "puzzle result releases callback and text on queue failure");
}

struct OverlayRuntime final : pal4::IJigsawResultOverlayRuntime {
    static constexpr std::uint32_t object = 0x7000u;
    std::unordered_map<std::uint32_t, float> floats{
        {object + 0x28u, 3.0f}, {object + 0x44u, 2.0f}};
    std::vector<std::string> calls;
    std::uint16_t status = 0u;
    double compared = 0.0;

    std::uint32_t UpdateAnimationProgress(float delta) override {
        Check(delta == 0.5f, "animation delta");
        calls.emplace_back("base");
        return 0xabcd1234u;
    }
    float ReadFloat(std::uint32_t address) override {
        calls.emplace_back("read");
        return floats.at(address);
    }
    void WriteFloat(std::uint32_t address, float value) override {
        floats[address] = value;
        calls.emplace_back("write");
    }
    std::uint16_t CompareStatusWord(double value) override {
        compared = value;
        calls.emplace_back("compare");
        return status;
    }
};

void CheckOverlayAnimation() {
    for (const auto status : {0u, 0x0100u, 0x4000u, 0x4500u}) {
        OverlayRuntime runtime;
        runtime.status = static_cast<std::uint16_t>(status);
        const auto result = pal4::JigsawResultOverlay::UpdateAnimation(
            OverlayRuntime::object, 0.5f, runtime);
        const auto clamp = status == 0x0100u || status == 0x4000u;
        std::vector<std::string> expected{
            "base", "read", "read", "write", "compare"};
        if (clamp) expected.emplace_back("write");
        expected.emplace_back("read");
        expected.emplace_back("write");
        Check(result == (0xabcd0000u | status) &&
                  runtime.compared == 2.0 &&
                  runtime.floats[OverlayRuntime::object + 0x28u] ==
                      (clamp ? 1.0f : 2.0f) &&
                  runtime.floats[OverlayRuntime::object + 0x24u] ==
                      (clamp ? 1.0f : 2.0f) &&
                  runtime.calls == expected,
              "overlay x87 comparison and low return word");
    }
}

}  // namespace

int main() {
    try {
        CheckResultBranches();
        CheckFailures();
        CheckPuzzleResult();
        CheckOverlayAnimation();
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
