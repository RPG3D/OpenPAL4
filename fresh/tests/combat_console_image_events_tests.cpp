#include "pal4/combat_console_image_events.h"
#include "pal4/ui_text_image_animation.h"

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

struct Runtime final : pal4::ICombatConsoleVisualRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::unordered_map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> calls;
    std::uint32_t camera_object = 0x6000u;
    bool fail_element_allocation = false;
    bool fail_font = false;
    bool fail_image = false;
    bool fail_queue = false;
    std::string requested_image;
    std::uint32_t dialog_calls = 0u;

    Runtime() {
        words[kEvent + 4u] = kActor;
        words[kQueue + 4u] = kHead;
        words[kHead] = kHead;
        words[kHead + 4u] = kHead;
    }

    std::uint8_t ReadByte(std::uint32_t address) override {
        return bytes[address];
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        if (address == kEvent + 4u) calls.emplace_back("actor");
        return words[address];
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        bytes[address] = value;
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
    }
    void EnsureCombatSystem() override {
        calls.emplace_back("combat-system");
    }
    std::uint32_t CameraManager() override {
        calls.emplace_back("camera-manager");
        return 0x8000u;
    }
    std::uint32_t FindCamera(std::uint32_t manager,
                             std::string_view name) override {
        Check(manager == 0x8000u && name == "CombatCamera",
              "camera lookup arguments");
        calls.emplace_back("find-camera");
        return camera_object;
    }
    std::uint32_t InternalCamera(std::uint32_t object) override {
        Check(object == camera_object, "camera object");
        calls.emplace_back("internal-camera");
        return 0x9000u;
    }
    void SetPosition(std::uint32_t, std::array<float, 3>,
                     std::uint32_t) override {}
    void SetDistance(std::uint32_t, float, std::uint32_t) override {}
    std::uint32_t SetAngles(std::uint32_t, std::array<float, 3>,
                            std::uint32_t) override { return 0u; }
    std::uint32_t ActorScreenPosition(
        std::uint32_t actor, std::uint32_t camera,
        std::array<std::uint32_t, 2>& position) override {
        Check(actor == kActor && camera == (camera_object ? 0x9000u : 0u),
              "actor projection arguments");
        calls.emplace_back("project");
        position = {0x42c80000u, 0x43480000u};
        return 0u;
    }
    std::uint32_t DialogData(std::uint32_t argument) override {
        Check(argument == kEvent, "dialog callback argument");
        ++dialog_calls;
        calls.emplace_back("dialog-data");
        return kActor;
    }
    std::uint32_t Allocate(std::uint32_t size) override {
        calls.emplace_back("allocate:" + std::to_string(size));
        if (size == 72u) return fail_element_allocation ? 0u : kElement;
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
    void CopyText(std::uint32_t, std::string_view) override {}
    void SetTextErrorFlag(std::uint32_t, std::uint8_t) override {}
    void ReallocateText(std::uint32_t, std::uint32_t) override {}
    std::uint32_t CreateCeguiString(std::u32string_view text) override {
        Check(text == U"system" || text == U"zi0", "CEGUI name");
        calls.emplace_back(text == U"system" ? "string:system" :
                           "string:zi0");
        return 0xa000u;
    }
    std::uint32_t GetFont(std::uint32_t name) override {
        Check(name == 0xa000u, "font name handle");
        calls.emplace_back("font");
        if (fail_font) throw std::runtime_error("font failure");
        return 0xb000u;
    }
    std::uint32_t GetImage(std::uint32_t name,
                            std::string_view image_name) override {
        Check(name == 0xa000u &&
                  (image_name == "luokong" || image_name == "mianyi" ||
                   image_name == "fangji"),
              "image resource identity");
        requested_image = image_name;
        calls.emplace_back("image:" + requested_image);
        if (fail_image) throw std::runtime_error("image failure");
        return 0xc000u;
    }
    void DestroyCeguiString(std::uint32_t name) override {
        Check(name == 0xa000u, "CEGUI string destruction");
        calls.emplace_back("destroy-string");
    }
    void CleanupTextBuffer(std::uint32_t address) override {
        Check(address == kElement + 4u, "constructor text cleanup");
        calls.emplace_back("cleanup-text");
    }
    std::uint32_t FreeTextAllocation(std::uint32_t) override { return 0u; }
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
};

std::size_t Position(const Runtime& runtime, std::string_view call) {
    const auto found = std::find(runtime.calls.begin(), runtime.calls.end(),
                                 call);
    Check(found != runtime.calls.end(), "expected call missing");
    return static_cast<std::size_t>(found - runtime.calls.begin());
}

void CreatesQueuedMissImage() {
    Runtime runtime;
    Check(pal4::CombatConsoleImageEvents::CreateMissImage(kEvent, runtime) ==
              kNode + 8u, "queue insertion result");
    Check(Position(runtime, "combat-system") <
              Position(runtime, "camera-manager") &&
              Position(runtime, "internal-camera") <
              Position(runtime, "actor") &&
              Position(runtime, "actor") < Position(runtime, "project") &&
              Position(runtime, "project") <
              Position(runtime, "allocate:72") &&
              Position(runtime, "string:system") <
              Position(runtime, "string:zi0") &&
              Position(runtime, "image:luokong") <
              Position(runtime, "allocate:16"),
          "IDA call sequence");
    Check(runtime.words[kElement] ==
              pal4::UiTextImageAnimation::kMotionVtable &&
              runtime.words[kElement + 20u] == 0xb000u &&
              runtime.words[kElement + 24u] == 0xc000u &&
              runtime.words[kElement + 28u] == 0x42c80000u &&
              runtime.words[kElement + 32u] == 0x43480000u &&
              runtime.words[kElement + 40u] == 0x3f800000u &&
              runtime.words[kElement + 56u] == 0x3f800000u &&
              runtime.words[kElement + 60u] == 0x40000000u &&
              runtime.words[kElement + 68u] == 0x3f800000u,
          "motion image fields");
    Check(runtime.words[kQueue + 8u] == 1u &&
              runtime.words[kHead] == kNode &&
              runtime.words[kHead + 4u] == kNode &&
              runtime.words[kNode] == kHead &&
              runtime.words[kNode + 4u] == kHead &&
              runtime.bytes[kNode + 8u] == 1u &&
              runtime.words[kNode + 12u] == kElement &&
              runtime.dialog_calls == 0u,
          "owned image linked in actor queue");
}

void CreatesQueuedImmuneImage() {
    Runtime runtime;
    Check(pal4::CombatConsoleImageEvents::CreateImmuneImage(
              kEvent, runtime) == kNode + 8u,
          "immune queue result");
    Check(runtime.dialog_calls == 2u &&
              runtime.requested_image == "mianyi" &&
              Position(runtime, "internal-camera") <
                  Position(runtime, "dialog-data") &&
              Position(runtime, "dialog-data") <
                  Position(runtime, "project") &&
              Position(runtime, "image:mianyi") <
                  Position(runtime, "allocate:16") &&
              runtime.words[kNode + 12u] == kElement,
          "immune image reads dialog actor twice and owns animation");
}

void CreatesQueuedCounterImage() {
    Runtime runtime;
    Check(pal4::CombatConsoleImageEvents::CreateCounterImage(
              kEvent, runtime) == kNode + 8u &&
              runtime.dialog_calls == 2u &&
              runtime.requested_image == "fangji" &&
              runtime.words[kNode + 12u] == kElement &&
              runtime.words[kElement + 60u] == 0x40000000u,
          "counter image shares dialog actor path and timing");
}

void MissingCameraStillProjects() {
    Runtime runtime;
    runtime.camera_object = 0u;
    Check(pal4::CombatConsoleImageEvents::CreateMissImage(kEvent, runtime) ==
              kNode + 8u, "missing camera result");
    Check(Position(runtime, "find-camera") < Position(runtime, "project") &&
              std::find(runtime.calls.begin(), runtime.calls.end(),
                        "internal-camera") == runtime.calls.end(),
          "missing camera forwarded as zero");
}

void AllocationFailureStopsBeforeConstruction() {
    Runtime runtime;
    runtime.fail_element_allocation = true;
    Check(pal4::CombatConsoleImageEvents::CreateMissImage(kEvent, runtime) ==
              0u, "safe allocation failure");
    Check(runtime.words[kQueue + 8u] == 0u &&
              std::find(runtime.calls.begin(), runtime.calls.end(),
                        "string:system") == runtime.calls.end() &&
              std::find(runtime.calls.begin(), runtime.calls.end(),
                        "allocate:16") == runtime.calls.end(),
          "failure leaves queue unchanged");
}

void ConstructorFailureFreesElement() {
    Runtime runtime;
    runtime.fail_font = true;
    bool threw = false;
    try {
        (void)pal4::CombatConsoleImageEvents::CreateMissImage(kEvent,
                                                               runtime);
    } catch (const std::runtime_error&) {
        threw = true;
    }
    Check(threw && Position(runtime, "cleanup-text") <
                       Position(runtime, "free:16384") &&
              runtime.words[kQueue + 8u] == 0u,
          "constructor exception cleanup");
}

void PostConstructorFailureFreesElement() {
    for (const bool queue_failure : {false, true}) {
        Runtime runtime;
        runtime.fail_image = !queue_failure;
        runtime.fail_queue = queue_failure;
        std::string failure;
        try {
            (void)pal4::CombatConsoleImageEvents::CreateImmuneImage(kEvent, runtime);
        } catch (const std::runtime_error& error) { failure = error.what(); }
        Check(failure == (queue_failure ? "queue failure" : "image failure"),
              "original image/queue failure propagates");
        Check(std::count(runtime.calls.begin(), runtime.calls.end(), "free:16384") == 1 &&
                  runtime.words[kElement] == pal4::UiTextImageOverlay::kVtable &&
                  runtime.words[kElement + 8u] == 0u &&
                  runtime.words[kQueue + 8u] == 0u &&
                  runtime.words[kHead] == kHead && runtime.words[kHead + 4u] == kHead,
              "failed image creation destroys constructed overlay once and leaves actor queue empty");
        Check(std::count(runtime.calls.begin(), runtime.calls.end(), "destroy-string") == 2,
              "failed image creation retires both temporary DLL strings");
    }
}

}  // namespace

int main() {
    try {
        CreatesQueuedMissImage();
        CreatesQueuedImmuneImage();
        CreatesQueuedCounterImage();
        MissingCameraStillProjects();
        AllocationFailureStopsBeforeConstruction();
        ConstructorFailureFreesElement();
        PostConstructorFailureFreesElement();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
