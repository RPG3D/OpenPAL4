#include "pal4/combat_console.h"

#include <bit>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

constexpr std::uint32_t kConsole = 0x5000u;
constexpr std::uint32_t kEvent = 0x6000u;
constexpr std::uint32_t kActor = 0x7000u;
constexpr std::uint32_t kManager = 0xb000u;
constexpr std::uint32_t kQueue = 0xc000u;
constexpr std::uint32_t kHead = 0xd000u;

struct Runtime final : pal4::ICombatConsoleDialogEventRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words{
        {kActor + 804u, 2u}, {kConsole + 236u + 5u * 4u, 0x8500u},
        {kConsole + 236u + 2u * 4u, 0x8200u},
        {kConsole + 224u + 2u * 4u, 0x7200u},
        {kConsole + 260u, 0x9000u}, {kConsole + 308u, 0u},
        {kManager + 416u, kQueue}, {kQueue + 16u, kHead},
        {kQueue + 20u, 0u}, {kHead + 4u, kHead}};
    std::vector<std::string> calls;
    std::int32_t actor_index = 5;
    bool update_effect = true;
    std::uint32_t hidden_window = 0u;
    std::uint32_t attached_frame = 0u;
    std::uint32_t attached_child = 0u;
    std::uint32_t copied_name = 0u;
    pal4::TimedQueueRecord copied_record{};
    std::uint32_t alpha_window = 0u;
    float alpha_value = 0.0f;

    std::uint32_t Allocate(std::uint32_t bytes) override {
        calls.push_back("allocate-" + std::to_string(bytes));
        if (bytes == 88u) return 0xe000u;
        if (bytes == 16u) return 0xe100u;
        Check(bytes == 36u, "dialog queue allocation size");
        return 0xe200u;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.push_back("read");
        return words.at(address);
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        calls.push_back("write");
        words[address] = value;
    }
    void InitializeName(pal4::TimedQueueRecord& record,
                        std::uint32_t source) override {
        Check(source == kConsole + 276u, "timer name field");
        calls.push_back("initialize-name");
        record.name_data = 0xf000u;
    }
    void CopyRecord(std::uint32_t destination,
                    pal4::TimedQueueRecord& source) override {
        Check(destination == 0xe200u + 8u, "queue record position");
        calls.push_back("copy-record");
        copied_record = source;
        source.owns_callback = 0u;
    }
    void DeleteCallback(std::uint32_t callback,
                        std::uint32_t flag) override {
        Check(callback == 0xe100u && flag == 1u,
              "temporary callback delete");
        calls.push_back("delete-callback");
    }
    void ReleaseName(pal4::TimedQueueRecord& record) override {
        calls.push_back("release-name");
        copied_name = record.name_data;
    }
    std::uint32_t ActorFromEvent(std::uint32_t event) override {
        Check(event == kEvent, "combat dialog event");
        calls.push_back("actor");
        return kActor;
    }
    std::int32_t ActorIndex(std::uint32_t actor) override {
        Check(actor == kActor, "dialog actor index");
        calls.push_back("index");
        return actor_index;
    }
    std::string ActorImageName(std::uint32_t actor) override {
        Check(actor == kActor, "dialog actor image");
        calls.push_back("image-name");
        return "portrait";
    }
    void SetVisible(std::uint32_t window, bool visible) override {
        Check(!visible, "dialog event only hides windows");
        calls.push_back("hide");
        hidden_window = window;
    }
    std::uint32_t ConstructVisualElement(std::uint32_t memory) override {
        Check(memory == 0xe000u, "visual element allocation");
        calls.push_back("construct");
        return memory;
    }
    void SetElementPosition(std::uint32_t element, float x,
                            float y) override {
        Check(element == 0xe000u && x == 750.0f && y == 50.0f,
              "visual element position");
        calls.push_back("element-position");
    }
    void SetElementImage(std::uint32_t element,
                         std::string_view imageset,
                         std::string_view image, float scale) override {
        Check(element == 0xe000u &&
                  imageset == "zhangdouxiaotouxiang0" &&
                  image == "portrait" && scale == 1.0f,
              "visual element image and scale");
        calls.push_back("element-image");
    }
    void SetElementFont(std::uint32_t element, float x,
                        float y) override {
        Check(element == 0xe000u && x == 0.4f && y == 0.4f,
              "visual element font parameters");
        calls.push_back("element-font");
    }
    void SetElementMotion(std::uint32_t element, float x,
                          float y, float width, float height) override {
        Check(element == 0xe000u && x == 5.0f && y == 0.2f &&
                  width == 5.0f && height == 0.2f,
              "visual element motion parameters");
        calls.push_back("element-motion");
    }
    void AddElement(std::uint32_t element) override {
        Check(element == 0xe000u, "inserted visual element");
        calls.push_back("add-element");
    }
    pal4::CombatConsolePosition TransformFrameCoordinates(
        pal4::CombatConsolePosition position) override {
        Check(position.x == 750.0f && position.y == 110.0f,
              "effect anchor");
        calls.push_back("transform-frame");
        return {15.0f, 25.0f};
    }
    std::uint32_t CurrentCombatCamera() override {
        calls.push_back("camera");
        return 0xf100u;
    }
    std::array<float, 3> TransformEffectPosition(
        std::uint32_t camera, pal4::CombatConsolePosition position,
        float depth) override {
        Check(camera == 0xf100u && position.x == 15.0f &&
                  position.y == 25.0f && depth == 300.1f,
              "effect position transform");
        calls.push_back("transform-effect");
        return {1.0f, 2.0f, 3.0f};
    }
    std::uint32_t CreateEffect(std::string_view name,
                               const std::array<float, 3>& position) override {
        Check(name == "jdg" && position ==
                  std::array<float, 3>{1.0f, 2.0f, 3.0f},
              "JDG effect name and position");
        calls.push_back("create-effect");
        return 0xf200u;
    }
    bool EffectNeedsUpdate(std::uint32_t effect) override {
        Check(effect == 0xf200u, "effect handle");
        calls.push_back("needs-update");
        return update_effect;
    }
    std::uint32_t UpdateEffectProperties(std::uint32_t effect) override {
        Check(effect == 0xf200u, "effect update handle");
        calls.push_back("update-effect");
        return 0xf300u;
    }
    std::uint32_t CombatCameraFrame() override {
        calls.push_back("camera-frame");
        return 0xf400u;
    }
    void RwFrameAddChild(std::uint32_t frame,
                         std::uint32_t child) override {
        calls.push_back("renderware-add-child");
        attached_frame = frame;
        attached_child = child;
    }
    std::uint32_t UiFrameManager() override {
        calls.push_back("frame-manager");
        return kManager;
    }
    std::uint32_t ResetEffect(std::uint32_t effect) override {
        Check(effect == 0xf200u, "effect reset handle");
        calls.push_back("reset-effect");
        return 0x12345678u;
    }
    void SetAlpha(std::uint32_t window, float value) override {
        calls.push_back("alpha");
        alpha_window = window;
        alpha_value = value;
    }
};

std::size_t First(const Runtime& runtime, const std::string& call) {
    const auto found = std::find(runtime.calls.begin(), runtime.calls.end(),
                                 call);
    Check(found != runtime.calls.end(), "missing dialog event call");
    return static_cast<std::size_t>(found - runtime.calls.begin());
}

void HandleAndQueue() {
    Runtime runtime;
    Check(pal4::CombatConsoleWindow::HandleCombatDialog(
              kConsole, kEvent, runtime) == 0x12345678u &&
              runtime.hidden_window == 0x8500u,
          "type-two actor hides monster slot five and resets effect");
    Check(runtime.attached_frame == 0xf400u &&
              runtime.attached_child == 0xf300u,
          "effect update reaches RenderWare boundary");
    Check(runtime.words.at(0xe100u) == 0x84553cu &&
              runtime.words.at(0xe100u + 4u) == kConsole &&
              runtime.words.at(0xe100u + 8u) == 0x54cac0u &&
              runtime.words.at(0xe100u + 12u) == 0u,
          "16-byte refresh callback fields");
    Check(runtime.copied_record.callback == 0xe100u &&
              runtime.copied_record.delay ==
                  std::bit_cast<std::uint32_t>(0.4f) &&
              runtime.copied_record.owns_callback == 1u &&
              runtime.words.at(kQueue + 20u) == 1u &&
              runtime.copied_name == 0xf000u,
          "timer record and list count");
    Check(First(runtime, "add-element") < First(runtime, "create-effect") &&
              First(runtime, "renderware-add-child") <
                  First(runtime, "initialize-name") &&
              First(runtime, "copy-record") < First(runtime, "release-name") &&
              First(runtime, "release-name") < First(runtime, "reset-effect"),
          "element, effect, timer and cleanup order");
}

void IndexAndEffectGates() {
    Runtime role_limit;
    role_limit.words[kActor + 804u] = 1u;
    role_limit.actor_index = 3;
    Check(pal4::CombatConsoleWindow::HandleCombatDialog(
              kConsole, kEvent, role_limit) == 3u &&
              std::count(role_limit.calls.begin(), role_limit.calls.end(),
                         "hide") == 0,
          "non-type-two index three skips effect");

    Runtime monster_limit;
    monster_limit.actor_index = 6;
    Check(pal4::CombatConsoleWindow::HandleCombatDialog(
              kConsole, kEvent, monster_limit) == 6u &&
              std::count(monster_limit.calls.begin(), monster_limit.calls.end(),
                         "allocate-88") == 0,
          "type-two index six skips allocation");

    Runtime no_update;
    no_update.update_effect = false;
    Check(pal4::CombatConsoleWindow::HandleCombatDialog(
              kConsole, kEvent, no_update) == 0x12345678u &&
              std::count(no_update.calls.begin(), no_update.calls.end(),
                         "renderware-add-child") == 0,
          "effect without update skips RenderWare attachment");
}

void TargetAndPanelMethods() {
    Runtime role;
    role.words[kActor + 804u] = 1u;
    role.actor_index = 2;
    Check(pal4::CombatConsoleWindow::SetDialogTargetWindow(
              kConsole, kEvent, role) == 2u &&
              role.words.at(kConsole + 308u) == 0x7200u,
          "role target uses +224 family");
    pal4::CombatConsoleWindow::ClearDialogTargetWindow(
        kConsole, 0xdeadbeefu, role);
    Check(role.words.at(kConsole + 308u) == 0u &&
              role.alpha_window == 0x7200u && role.alpha_value == 1.0f,
          "target clear restores alpha and nulls field");
    role.calls.clear();
    pal4::CombatConsoleWindow::ClearDialogTargetWindow(
        kConsole, 0xdeadbeefu, role);
    Check(std::count(role.calls.begin(), role.calls.end(), "alpha") == 0,
          "null target skips alpha reset");

    Runtime monster;
    monster.actor_index = 5;
    Check(pal4::CombatConsoleWindow::SetDialogTargetWindow(
              kConsole, kEvent, monster) == 5u &&
              monster.words.at(kConsole + 308u) == 0x8500u,
          "other actor types use +236 family");
    monster.actor_index = 6;
    Check(pal4::CombatConsoleWindow::SetDialogTargetWindow(
              kConsole, kEvent, monster) == 6u &&
              monster.words.at(kConsole + 308u) == 0x8500u,
          "out-of-range target preserves previous pointer");
    pal4::CombatConsoleWindow::HideDialogPanel(
        kConsole, 0xdeadbeefu, monster);
    Check(monster.hidden_window == 0x9000u,
          "dialog panel hide uses +260 control");
}

}  // namespace

int main() {
    try {
        HandleAndQueue();
        IndexAndEffectGates();
        TargetAndPanelMethods();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
