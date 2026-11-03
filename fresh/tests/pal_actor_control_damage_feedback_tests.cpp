#include "pal4/pal_actor_control_damage_feedback.h"

#include <algorithm>
#include <bit>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(1);
    }
}

std::uint32_t Bits(float value) {
    return std::bit_cast<std::uint32_t>(value);
}

struct Runtime final : pal4::IActorControlDamageFeedbackRuntime {
    std::vector<std::string> calls;
    std::vector<std::pair<std::uint32_t, std::uint32_t>> writes;
    std::uint32_t player{0x2000u};
    std::uint32_t allocation{0x9000u};
    std::int32_t health{10};
    std::int32_t base{200};
    std::int32_t conversion{2};
    std::int32_t changed_by{};
    std::int32_t formatted_amount{};
    long double conversion_input{};
    bool throw_on_insert{};
    std::uint32_t cleanup_count{};

    std::uint32_t PlayerManager() override {
        calls.emplace_back("manager"); return 0x1000u;
    }
    std::uint32_t FindPlayer(std::uint32_t manager,
                             std::uint32_t index) override {
        Check(manager == 0x1000u && index == 2u, "player lookup args");
        calls.emplace_back("player"); return player;
    }
    std::int32_t ReadAttribute(std::uint32_t address,
                               std::uint32_t slot,
                               std::uint32_t property) override {
        Check(address == player + 0x654u, "attribute subobject offset");
        calls.emplace_back("read" + std::to_string(slot) + ":" +
                           std::to_string(property));
        if (slot == 0x30u && property == 0u) return base;
        Check(slot == 0x34u && property == 7u,
              "attribute virtual slot/property");
        return health;
    }
    void ChangeAttribute(std::uint32_t address, std::uint32_t property,
                         std::int32_t amount) override {
        Check(address == player + 0x654u && property == 7u,
              "change attribute args");
        calls.emplace_back("change" + std::to_string(amount));
        changed_by += amount;
        health += amount;
    }
    std::int32_t FloatToLong(long double value) override {
        calls.emplace_back("ftol"); conversion_input = value;
        return conversion;
    }
    pal4::DamageWorldPoint PlayerPosition(std::uint32_t address) override {
        Check(address == player + 0xacu, "player position offset");
        calls.emplace_back("position"); return {3.0f, 4.0f, 5.0f};
    }
    float PlayerHeight(std::uint32_t address) override {
        Check(address == player + 0xf4u, "player height offset");
        calls.emplace_back("height"); return 6.0f;
    }
    std::uint32_t CameraSubsystem() override {
        calls.emplace_back("camera_subsystem");
        return ++camera_count == 1u ? 0xa000u : 0xb000u;
    }
    std::uint32_t camera_count{};
    std::uint32_t ActiveCamera(std::uint32_t subsystem) override {
        calls.emplace_back("active" + std::to_string(subsystem));
        return subsystem + 1u;
    }
    pal4::DamageScreenPoint Project(
        std::uint32_t camera,
        const pal4::DamageWorldPoint& world) override {
        Check(world == pal4::DamageWorldPoint{3.0f, 30.0f, 5.0f},
              "raised world point");
        calls.emplace_back("project" + std::to_string(camera));
        return camera == 0xb001u ?
            pal4::DamageScreenPoint{11.0f, 22.0f} :
            pal4::DamageScreenPoint{33.0f, 44.0f};
    }
    std::uint32_t UiFrameManager() override {
        calls.emplace_back("ui_manager"); return 0xc000u;
    }
    pal4::DamageScreenPoint ConvertCoordinates(
        std::uint32_t manager,
        const pal4::DamageScreenPoint& point) override {
        Check(manager == 0xc000u &&
                  point == pal4::DamageScreenPoint{33.0f, 22.0f},
              "mixed projection components");
        calls.emplace_back("convert"); return {55.0f, 66.0f};
    }
    std::uint32_t Allocate(std::uint32_t size) override {
        Check(size == 92u, "feedback allocation size");
        calls.emplace_back("allocate"); return allocation;
    }
    void InitializeFeedback(std::uint32_t object) override {
        Check(object == allocation, "feedback constructor address");
        calls.emplace_back("initialize");
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        calls.emplace_back("write"); writes.emplace_back(address, value);
    }
    std::string FormatDamage(std::int32_t amount) override {
        calls.emplace_back("format"); formatted_amount = amount;
        return std::to_string(amount);
    }
    void InitializeFont(std::uint32_t object, std::string_view text,
                        std::string_view font, float scale) override {
        Check(object == allocation &&
                  text == std::to_string(formatted_amount) &&
                  font == "RoleHpDec" && scale == 1.0f,
              "formatted damage font args");
        calls.emplace_back("font");
    }
    void SetPosition(std::uint32_t object,
                     const pal4::DamageScreenPoint& point) override {
        Check(object == allocation &&
                  point == pal4::DamageScreenPoint{55.0f, 66.0f},
              "feedback UI position");
        calls.emplace_back("set_position");
    }
    void SetFontProperties(std::uint32_t object, float x,
                           float y) override {
        Check(object == allocation && x == 0.5f && y == 0.5f,
              "font properties");
        calls.emplace_back("font_properties");
    }
    void AddFeedback(std::uint32_t list, std::uint8_t kind,
                     std::uint32_t object) override {
        Check(list == player + 0xaa8u && kind == 1u &&
                  object == allocation, "feedback list args");
        calls.emplace_back("insert");
        if (throw_on_insert) throw std::runtime_error("insert");
    }
    std::uint32_t CleanupText(std::string& text) override {
        Check(text == std::to_string(formatted_amount),
              "cleanup text contents");
        calls.emplace_back("cleanup"); ++cleanup_count; return 0x12345678u;
    }
};

}  // namespace

int main() {
    pal4::PalActorControlState state{};
    Runtime runtime;
    state.words[19u] = 0xffffffffu;
    Check(pal4::PalActorControlDamageFeedback::Apply(state, 2.0f, runtime) ==
              0xffffffffu && runtime.calls.empty(),
          "negative player index returns unchanged");
    state.words[19u] = 4u;
    Check(pal4::PalActorControlDamageFeedback::Apply(state, 2.0f, runtime) ==
              4u && runtime.calls.empty(), "high player index returns unchanged");

    state.words[19u] = 2u;
    runtime.player = 0u;
    Check(pal4::PalActorControlDamageFeedback::Apply(state, 2.0f, runtime) ==
              0u && runtime.calls == std::vector<std::string>{"manager", "player"},
          "missing player returns zero");
    runtime.player = 0x2000u;
    runtime.calls.clear();
    runtime.health = 1;
    Check(pal4::PalActorControlDamageFeedback::Apply(state, 2.0f, runtime) ==
              1u && runtime.calls ==
                  std::vector<std::string>{"manager", "player", "read52:7"},
          "one health short circuits");
    runtime.calls.clear();
    runtime.health = -3;
    Check(pal4::PalActorControlDamageFeedback::Apply(state, 2.0f, runtime) ==
              std::bit_cast<std::uint32_t>(-3) && runtime.calls.size() == 3u,
          "negative health preserves EAX bits");

    runtime = Runtime{};
    runtime.health = 2;
    runtime.allocation = 0u;
    // A null allocator result is still passed into the original downstream calls.
    runtime.conversion = 2;
    Check(pal4::PalActorControlDamageFeedback::Apply(state, 2.0f, runtime) ==
              0x12345678u && runtime.health == 1 &&
              runtime.changed_by == -1 && runtime.cleanup_count == 1u,
          "damage clamps health and retains cleanup return");
    Check(runtime.conversion_input > 3.9L && runtime.conversion_input < 4.1L,
          "damage uses attribute times percent times input");
    Check(runtime.camera_count == 2u, "two camera initializations");
    const std::vector<std::string> camera_calls{
        "camera_subsystem", "camera_subsystem", "active45056",
        "project45057", "active40960", "project40961", "ui_manager",
        "convert", "allocate", "format", "font", "set_position",
        "font_properties"};
    const auto camera_start = std::find(runtime.calls.begin(), runtime.calls.end(),
                                        "camera_subsystem");
    Check(camera_start != runtime.calls.end() &&
              std::equal(camera_calls.begin(), camera_calls.end(), camera_start),
          "camera, allocation and UI call order");
    Check(runtime.writes == std::vector<std::pair<std::uint32_t,
              std::uint32_t>>{
                  {0x50u, Bits(0.1f)}, {0x54u, Bits(0.1f)},
                  {0x48u, Bits(5.0f)}, {0x4cu, Bits(5.0f)},
                  {0x44u, Bits(80.0f)}, {0x58u, 0u}},
          "null allocation skips constructor but continues field writes");

    runtime = Runtime{};
    Check(pal4::PalActorControlDamageFeedback::Apply(state, 2.0f, runtime) ==
              0x12345678u, "full feedback path");
    Check(runtime.writes == std::vector<std::pair<std::uint32_t,
              std::uint32_t>>{
                  {0x9000u, 0x842698u}, {0x9058u, 0u},
                  {0x9050u, 0u}, {0x9054u, 0u},
                  {0x9048u, 0u}, {0x904cu, 0u},
                  {0x9050u, Bits(0.1f)}, {0x9054u, Bits(0.1f)},
                  {0x9048u, Bits(5.0f)}, {0x904cu, Bits(5.0f)},
                  {0x9044u, Bits(80.0f)}, {0x9058u, 0u}},
          "constructor and feedback field order");
    Check(runtime.calls.back() == "cleanup", "cleanup follows list insertion");

    runtime = Runtime{};
    runtime.conversion = 0;
    (void)pal4::PalActorControlDamageFeedback::Apply(state, -1.0f, runtime);
    Check(runtime.changed_by == -1 && runtime.health == 9 &&
              std::find(runtime.calls.begin(), runtime.calls.end(),
                        "change-1") != runtime.calls.end(),
          "nonpositive conversion uses minimum damage one");

    runtime = Runtime{};
    runtime.throw_on_insert = true;
    try {
        (void)pal4::PalActorControlDamageFeedback::Apply(state, 2.0f, runtime);
        Check(false, "insertion exception propagates");
    } catch (const std::runtime_error&) {
        Check(runtime.cleanup_count == 1u &&
                  runtime.calls.back() == "cleanup",
              "exception cleans temporary text once");
    }
}
