#include "pal4/combat_console_number_events.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace {

constexpr std::uint32_t kEvent = 0x1000u;
constexpr std::uint32_t kActor = 0x2000u;
constexpr std::uint32_t kQueue = kActor + 0x2d8u;
constexpr std::uint32_t kHead = 0x3000u;
constexpr std::uint32_t kElement = 0x4000u;
constexpr std::uint32_t kNode = 0x5000u;

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::ICombatConsoleNumberRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::unordered_map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> calls;
    std::string displayed_text;
    std::string displayed_font;
    std::uint32_t attribute = 16u;
    std::uint32_t dialog_calls = 0u;
    bool fail_allocation = false;
    bool fail_constructor = false;
    bool fail_number_font = false;
    bool fail_queue = false;
    bool fail_formatted_cleanup = false;

    Runtime() {
        words[kEvent + 4u] = 7u;
        words[kEvent + 8u] = 10u;
        words[kEvent + 12u] = 12u;
        words[kActor + 0x324u] = 1u;
        words[kQueue + 4u] = kHead;
        words[kHead] = kHead;
        words[kHead + 4u] = kHead;
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        return bytes[address];
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        if (address == kActor + 0x324u) calls.emplace_back("actor-kind");
        return words[address];
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        bytes[address] = value;
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
    }
    std::uint32_t DialogData(std::uint32_t event) override {
        Check(event == kEvent, "dialog event");
        ++dialog_calls;
        calls.emplace_back("dialog");
        return kActor;
    }
    std::uint32_t ActorAttribute(std::uint32_t actor,
                                 std::uint32_t property) override {
        Check(actor == kActor && (property == 7u || property == 9u),
              "actor property query");
        calls.emplace_back("attribute");
        return attribute;
    }
    void EnsureCombatSystem() override {
        calls.emplace_back("combat-system");
    }
    std::uint32_t CameraManager() override {
        calls.emplace_back("camera-manager");
        return 0x6100u;
    }
    std::uint32_t FindCamera(std::uint32_t manager,
                             std::string_view name) override {
        Check(manager == 0x6100u && name == "CombatCamera",
              "combat camera lookup");
        calls.emplace_back("find-camera");
        return 0x6200u;
    }
    std::uint32_t InternalCamera(std::uint32_t camera) override {
        Check(camera == 0x6200u, "camera object");
        calls.emplace_back("internal-camera");
        return 0x6300u;
    }
    void SetPosition(std::uint32_t, std::array<float, 3>,
                     std::uint32_t) override {}
    void SetDistance(std::uint32_t, float, std::uint32_t) override {}
    std::uint32_t SetAngles(std::uint32_t, std::array<float, 3>,
                            std::uint32_t) override { return 0u; }
    std::uint32_t ActorScreenPosition(
        std::uint32_t actor, std::uint32_t camera,
        std::array<std::uint32_t, 2>& position) override {
        Check(actor == kActor && camera == 0x6300u, "projection inputs");
        calls.emplace_back("project");
        position = {0x42c80000u, 0x43480000u};
        return 0xabcdef01u;
    }
    std::uint32_t Allocate(std::uint32_t size) override {
        calls.emplace_back("allocate:" + std::to_string(size));
        if (size == 92u) return fail_allocation ? 0u : kElement;
        Check(size == 16u, "queue node size");
        if (fail_queue) throw std::runtime_error("queue failure");
        return kNode;
    }
    std::uint32_t Free(std::uint32_t address) override {
        calls.emplace_back("free:" + std::to_string(address));
        return address;
    }
    void DeleteAnimation(std::uint32_t) override {}
    void AdvanceAnimation(std::uint32_t, float) override {}
    void CallAnimationEvent(std::uint32_t, std::uint32_t) override {}
    std::uint32_t AnimationFinished(std::uint32_t) override { return 0u; }
    void CopyText(std::uint32_t destination,
                  std::string_view text) override {
        Check(destination == 0x7000u, "text allocation");
        displayed_text = text;
        calls.emplace_back("copy-text");
    }
    void SetTextErrorFlag(std::uint32_t, std::uint8_t) override {}
    void ReallocateText(std::uint32_t record,
                        std::uint32_t length) override {
        Check(record == kElement + 4u && length > 0u,
              "text record allocation");
        words[kElement + 8u] = 0x7000u;
    }
    std::uint32_t CreateCeguiString(std::u32string_view text) override {
        calls.emplace_back("cegui-string");
        displayed_font.clear();
        if (text != U"system") {
            for (const auto character : text)
                displayed_font.push_back(static_cast<char>(character));
        }
        return 0x8000u;
    }
    std::uint32_t GetFont(std::uint32_t string) override {
        Check(string == 0x8000u, "font string handle");
        calls.emplace_back("get-font");
        if (fail_constructor) throw std::runtime_error("font failure");
        if (fail_number_font && !displayed_font.empty())
            throw std::runtime_error("number font failure");
        return 0x9000u;
    }
    std::uint32_t GetImage(std::uint32_t,
                            std::string_view) override { return 0u; }
    void DestroyCeguiString(std::uint32_t string) override {
        Check(string == 0x8000u, "destroy string handle");
    }
    void CleanupTextBuffer(std::uint32_t address) override {
        Check(address == kElement + 4u, "constructor buffer cleanup");
        calls.emplace_back("cleanup-buffer");
    }
    std::uint32_t FreeTextAllocation(std::uint32_t address) override {
        Check(address == 0x6fffu, "number text allocation header");
        calls.emplace_back("free-number-text");
        return 0u;
    }
    std::uint32_t RenderWidth() override { return 0u; }
    std::uint32_t RenderHeight() override { return 0u; }
    std::string ConvertToUtf8(std::uint32_t) override { return {}; }
    float MeasureText(std::uint32_t, std::string_view, float) override {
        return 0.0f;
    }
    void DrawImage(std::uint32_t, const pal4::UiOverlayRect&,
                   const pal4::UiOverlayRect&, float) override {}
    void DrawOverlayText(std::uint32_t, std::string_view,
                  const pal4::UiOverlayRect&, const pal4::UiOverlayRect&,
                  float, float) override {}
    std::uint32_t CleanupFormattedText(std::string_view text) override {
        Check(text == displayed_text, "temporary text contents");
        calls.emplace_back("cleanup-formatted");
        if (fail_formatted_cleanup) throw std::runtime_error("formatted cleanup failure");
        return 0xbeefu;
    }
};

std::size_t Position(const Runtime& runtime, std::string_view call) {
    const auto found = std::find(runtime.calls.begin(), runtime.calls.end(),
                                 call);
    Check(found != runtime.calls.end(), "expected call missing");
    return static_cast<std::size_t>(found - runtime.calls.begin());
}

void CheckFeedbackFields(const Runtime& runtime) {
    Check(runtime.words.at(kElement) ==
              pal4::CombatConsoleNumberEvents::kVtable &&
              runtime.words.at(kElement + 24u) == 0u &&
              runtime.words.at(kElement + 28u) == 0x42c80000u &&
              runtime.words.at(kElement + 32u) == 0x43480000u &&
              runtime.words.at(kElement + 36u) == 0x3f800000u &&
              runtime.words.at(kElement + 56u) == 0x3f000000u &&
              runtime.words.at(kElement + 60u) == 0x3fc00000u &&
              runtime.words.at(kElement + 68u) == 0x42a00000u &&
              runtime.words.at(kElement + 72u) == 0x40a00000u &&
              runtime.words.at(kElement + 76u) == 0x40a00000u &&
              runtime.words.at(kElement + 80u) == 0x3dcccccdu &&
              runtime.words.at(kElement + 84u) == 0x3dcccccdu &&
              runtime.words.at(kElement + 88u) == 0u,
          "92-byte feedback fields");
    Check(runtime.words.at(kQueue + 8u) == 1u &&
              runtime.words.at(kNode + 12u) == kElement &&
              runtime.bytes.at(kNode + 8u) == 1u,
          "owned feedback queued");
}

void NoDisplayBranches() {
    Runtime other;
    other.words[kEvent + 4u] = 5u;
    Check(pal4::CombatConsoleNumberEvents::Display(kEvent, other) == 5u &&
              other.dialog_calls == 1u && other.calls.size() == 1u,
          "other property returns id after dialog lookup");
    Runtime no_mp_change;
    no_mp_change.words[kEvent + 4u] = 9u;
    no_mp_change.words[kEvent + 12u] = 10u;
    Check(pal4::CombatConsoleNumberEvents::Display(kEvent,
                                                    no_mp_change) == 9u &&
              no_mp_change.calls.size() == 1u,
          "nonpositive MP delta returns property id");
    Runtime no_hp_gain;
    no_hp_gain.attribute = 10u;
    Check(pal4::CombatConsoleNumberEvents::Display(kEvent,
                                                    no_hp_gain) ==
              0xabcdef01u &&
              no_hp_gain.dialog_calls == 2u &&
              Position(no_hp_gain, "attribute") <
                  Position(no_hp_gain, "combat-system") &&
              Position(no_hp_gain, "project") ==
                  no_hp_gain.calls.size() - 1u,
          "nonpositive HP result preserves projection return");
}

void HpIncreaseAndDamage() {
    Runtime increase;
    Check(pal4::CombatConsoleNumberEvents::Display(kEvent, increase) ==
              0xbeefu &&
              increase.displayed_text == "+6" &&
              increase.displayed_font == "HpInc" &&
              increase.dialog_calls == 3u &&
              Position(increase, "project") <
                  Position(increase, "allocate:92") &&
              Position(increase, "allocate:16") <
                  Position(increase, "cleanup-formatted"),
          "HP increase presentation and cleanup");
    CheckFeedbackFields(increase);

    Runtime damage;
    damage.words[kEvent + 12u] = 6u;
    Check(pal4::CombatConsoleNumberEvents::Display(kEvent, damage) ==
              0xbeefu &&
              damage.displayed_text == "-4" &&
              damage.displayed_font == "RoleHpDec" &&
              damage.dialog_calls == 2u &&
              std::find(damage.calls.begin(), damage.calls.end(),
                        "attribute") == damage.calls.end() &&
              Position(damage, "allocate:92") <
                  Position(damage, "actor-kind"),
          "role damage flow");
    CheckFeedbackFields(damage);

    Runtime monster;
    monster.words[kEvent + 12u] = 10u;
    monster.words[kActor + 0x324u] = 2u;
    Check(pal4::CombatConsoleNumberEvents::Display(kEvent, monster) ==
              0xbeefu &&
              monster.displayed_text == "-0" &&
              monster.displayed_font == "MonsterHpDec",
          "zero delta uses monster decrease branch");
}

void MpIncreaseAndDelayedQueue() {
    Runtime runtime;
    runtime.words[kEvent + 4u] = 9u;
    runtime.attribute = 8u;
    runtime.words[kHead] = 0x5100u;
    runtime.words[kHead + 4u] = 0x5100u;
    runtime.words[0x5100u] = kHead;
    runtime.words[0x5100u + 4u] = kHead;
    runtime.words[0x5100u + 12u] = 0x5200u;
    runtime.words[0x5200u + 48u] = 0x3e800000u;
    runtime.words[kQueue + 8u] = 1u;
    runtime.words[kQueue + 60u] = 0x3e4ccccdu;
    Check(pal4::CombatConsoleNumberEvents::Display(kEvent, runtime) ==
              0xbeefu &&
              runtime.displayed_text == "+-2" &&
              runtime.displayed_font == "MpInc" &&
              runtime.dialog_calls == 2u &&
              runtime.words[kQueue + 8u] == 2u &&
              runtime.words[kElement + 52u] == 0xbd4cccccu,
          "MP feedback queues even with negative queried amount");
}

void FailurePaths() {
    Runtime missing;
    missing.fail_allocation = true;
    Check(pal4::CombatConsoleNumberEvents::Display(kEvent, missing) == 0u &&
              missing.dialog_calls == 2u &&
              std::find(missing.calls.begin(), missing.calls.end(),
                        "cleanup-formatted") == missing.calls.end(),
          "safe allocation failure before temporary string");

    Runtime throwing;
    throwing.fail_constructor = true;
    bool caught = false;
    try {
        (void)pal4::CombatConsoleNumberEvents::Display(kEvent, throwing);
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught && Position(throwing, "cleanup-buffer") <
                        Position(throwing, "free:16384") &&
              std::find(throwing.calls.begin(), throwing.calls.end(),
                        "allocate:16") == throwing.calls.end(),
          "constructor unwinding frees 92-byte allocation");
}

void PostConstructorFailureRetiresTextAndObject() {
    for (const bool queue_failure : {false, true}) {
        Runtime runtime;
        runtime.fail_number_font = !queue_failure;
        runtime.fail_queue = queue_failure;
        std::string failure;
        try {
            (void)pal4::CombatConsoleNumberEvents::Display(kEvent, runtime);
        } catch (const std::runtime_error& error) { failure = error.what(); }
        Check(failure == (queue_failure ? "queue failure" : "number font failure"),
              "original number presentation failure propagates");
        Check(std::count(runtime.calls.begin(), runtime.calls.end(), "free:16384") == 1 &&
                  std::count(runtime.calls.begin(), runtime.calls.end(), "free-number-text") == 1 &&
                  std::count(runtime.calls.begin(), runtime.calls.end(), "cleanup-formatted") == 1 &&
                  Position(runtime, "cleanup-formatted") < Position(runtime, "free-number-text") &&
                  Position(runtime, "free-number-text") < Position(runtime, "free:16384") &&
                  runtime.words[kElement + 8u] == 0u && runtime.words[kQueue + 8u] == 0u &&
                  runtime.words[kHead] == kHead && runtime.words[kHead + 4u] == kHead,
              "failed number creation releases host text and constructed object exactly once");
    }
}

void QueueOwnsActivityAfterFormattedCleanupFailure() {
    Runtime runtime;
    runtime.fail_formatted_cleanup = true;
    std::string failure;
    try {
        (void)pal4::CombatConsoleNumberEvents::Display(kEvent, runtime);
    } catch (const std::runtime_error& error) { failure = error.what(); }
    Check(failure == "formatted cleanup failure" &&
              runtime.words[kQueue + 8u] == 1u && runtime.words[kNode + 12u] == kElement &&
              runtime.words[kElement] == pal4::CombatConsoleNumberEvents::kVtable &&
              std::count(runtime.calls.begin(), runtime.calls.end(), "cleanup-formatted") == 1 &&
              std::count(runtime.calls.begin(), runtime.calls.end(), "free:16384") == 0 &&
              std::count(runtime.calls.begin(), runtime.calls.end(), "free-number-text") == 0,
          "committed actor queue retains activity if later formatted cleanup throws");
}

}  // namespace

int main() {
    try {
        NoDisplayBranches();
        HpIncreaseAndDamage();
        MpIncreaseAndDelayedQueue();
        FailurePaths();
        PostConstructorFailureRetiresTextAndObject();
        QueueOwnsActivityAfterFormattedCleanupFailure();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
