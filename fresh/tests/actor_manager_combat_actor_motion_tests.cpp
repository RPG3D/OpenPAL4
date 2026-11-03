#include "pal4/actor_manager_combat_actor_motion.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <map>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

using pal4::ActorManagerCombatActorMotion;
using pal4::ActorManagerVec3;
constexpr std::uint32_t actor = 0x1000u;
constexpr std::uint32_t child = 0x2000u;
constexpr std::uint32_t iterator = 0x3000u;

void Check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}
bool Near(float first, float second) {
    return std::fabs(first - second) < 0.01f;
}

struct Runtime final : pal4::IActorManagerCombatMotionRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::map<std::uint32_t, std::string> animation_names;
    std::vector<std::uint32_t> members;
    std::vector<std::string> trace;
    std::optional<std::string_view> global_name;
    ActorManagerVec3 position{};
    ActorManagerVec3 last_angle{};
    std::size_t index = 0;
    unsigned position_reads = 0;
    unsigned normalizations = 0;
    unsigned acos_calls = 0;
    bool ready = true;
    bool throw_during_iteration = false;
    std::uint32_t iterator_value = iterator;
    std::uint32_t last_mode = 99u;

    std::uint32_t ReadWord(std::uint32_t address) override {
        trace.emplace_back("read_word:" + std::to_string(address));
        return words[address];
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        trace.emplace_back("read_byte:" + std::to_string(address));
        return bytes[address];
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        trace.emplace_back("write_byte:" + std::to_string(address));
        bytes[address] = value;
    }
    bool IsActorReady(std::uint32_t address) override {
        Check(address == actor, "readiness actor");
        trace.emplace_back("ready");
        return ready;
    }
    std::uint32_t ActorSlot444(std::uint32_t address,
                               std::uint32_t argument) override {
        Check(address == actor && argument == 0u, "slot 444 arguments");
        trace.emplace_back("slot444");
        return 0x777u;
    }
    void ChildSlot4(std::uint32_t address, std::uint32_t value) override {
        Check(address == child && value == 0x777u, "child slot 4 payload");
        trace.emplace_back("child4");
    }
    void Normalize(ActorManagerVec3& vector) override {
        ++normalizations;
        trace.emplace_back("normalize");
        const auto length = std::sqrt(vector.x * vector.x +
                                      vector.y * vector.y +
                                      vector.z * vector.z);
        if (length > 0.0f) {
            vector.x /= length;
            vector.y /= length;
            vector.z /= length;
        }
    }
    ActorManagerVec3 ChildPosition(std::uint32_t address) override {
        Check(address == actor, "position actor");
        ++position_reads;
        trace.emplace_back("position");
        return position;
    }
    double Acos(float cosine) override {
        ++acos_calls;
        trace.emplace_back("acos");
        return std::acos(cosine);
    }
    std::uint32_t ChildSlot8(std::uint32_t address,
                             const ActorManagerVec3& angle,
                             std::uint32_t mode) override {
        Check(address == actor, "child slot 8 actor");
        last_angle = angle;
        last_mode = mode;
        trace.emplace_back("child8");
        return 0x888u;
    }
    void ActorSlot564(std::uint32_t address, std::uint32_t global,
                      std::uint32_t mode) override {
        Check(address == actor && global == 0x8fa3a0u && mode == 1u,
              "slot 564 global and mode");
        trace.emplace_back("slot564");
    }
    void ActorSlot396(std::uint32_t address, std::uint32_t mode) override {
        Check(mode == 1u, "slot 396 mode");
        trace.emplace_back("slot396:" + std::to_string(address));
    }
    std::uint32_t ActorSlot464(std::uint32_t address) override {
        Check(address == actor, "slot 464 actor");
        trace.emplace_back("slot464");
        return iterator_value;
    }
    bool IteratorAtEnd(std::uint32_t value) override {
        Check(value == iterator, "iterator end receiver");
        trace.emplace_back("end");
        return index == members.size();
    }
    std::uint32_t IteratorActor(std::uint32_t value) override {
        Check(value == iterator, "iterator actor receiver");
        trace.emplace_back("current");
        if (throw_during_iteration) throw std::runtime_error("iterator failure");
        return members[index];
    }
    void IteratorNext(std::uint32_t value) override {
        Check(value == iterator, "iterator next receiver");
        trace.emplace_back("next");
        ++index;
    }
    void DeleteIterator(std::uint32_t value, std::uint32_t flags) override {
        Check(value == iterator && flags == 1u, "iterator delete flags");
        trace.emplace_back("delete");
    }
    std::optional<std::string_view> GlobalAnimationName() override {
        trace.emplace_back("global_name");
        return global_name;
    }
    std::string_view AnimationName(std::uint32_t address) override {
        trace.emplace_back("animation_name");
        return animation_names[address];
    }
    bool EqualIgnoreCase(std::string_view first,
                         std::string_view second) override {
        trace.emplace_back("compare");
        if (first.size() != second.size()) return false;
        for (std::size_t i = 0; i < first.size(); ++i) {
            const auto lower = [](char value) {
                return static_cast<char>(std::tolower(
                    static_cast<unsigned char>(value)));
            };
            if (lower(first[i]) != lower(second[i])) return false;
        }
        return true;
    }
};

void GatesAndMotion() {
    Runtime absent;
    ActorManagerCombatActorMotion::Update(actor, 1u, absent);
    Check(absent.trace == std::vector<std::string>{
              "read_word:4904", "read_byte:5256"},
          "absent child skips readiness and motion");

    Runtime unready;
    unready.words[actor + 808u] = child;
    unready.ready = false;
    ActorManagerCombatActorMotion::Update(actor, 1u, unready);
    Check(unready.normalizations == 0u && unready.acos_calls == 0u &&
              unready.trace == std::vector<std::string>{
                  "read_word:4904", "ready", "read_byte:5256"},
          "unready actor skips motion");

    Runtime zero_flag;
    zero_flag.words[actor + 808u] = child;
    ActorManagerCombatActorMotion::Update(actor, 0x100u, zero_flag);
    Check(std::find(zero_flag.trace.begin(), zero_flag.trace.end(),
                    "slot444") == zero_flag.trace.end() &&
              zero_flag.position_reads == 3u &&
              zero_flag.normalizations == 3u &&
              zero_flag.acos_calls == 1u && zero_flag.last_mode == 0u &&
              Near(zero_flag.last_angle.x, 0.0f) &&
              Near(zero_flag.last_angle.z, 0.0f) &&
              zero_flag.last_angle.y < 0.0f,
          "low-byte flag gate and negative angle");
    Check(std::find(zero_flag.trace.begin(), zero_flag.trace.end(),
                    "slot564") != zero_flag.trace.end(),
          "motion finishes with actor slot 564");

    Runtime positive;
    positive.words[actor + 808u] = child;
    positive.position.x = -1000.0f;
    ActorManagerCombatActorMotion::Update(actor, 1u, positive);
    Check(positive.trace[2] == "slot444" && positive.trace[3] == "child4" &&
              positive.last_angle.y > 0.0f,
          "flag calls child before positive-angle motion");
}

void Propagation() {
    Runtime runtime;
    runtime.bytes[actor + 1160u] = 1u;
    runtime.members = {0x4000u, 0x5000u, 0x6000u};
    runtime.words[0x4000u + 808u] = 0x7000u;
    runtime.words[0x6000u + 808u] = 0x8000u;
    runtime.animation_names[0x7000u + 368u] = "ATTACK";
    runtime.animation_names[0x8000u + 368u] = "idle";
    runtime.global_name = "attack";
    runtime.bytes[0x4000u + 1160u] = 1u;
    runtime.bytes[0x6000u + 1160u] = 1u;
    ActorManagerCombatActorMotion::Update(actor, 0u, runtime);
    Check(runtime.bytes[actor + 1160u] == 0u &&
              runtime.bytes[0x4000u + 1160u] == 0u &&
              runtime.bytes[0x6000u + 1160u] == 1u &&
              runtime.index == 3u && runtime.trace.back() == "delete",
          "matching member clears state, all members advance and iterator deletes");
    Check(std::count(runtime.trace.begin(), runtime.trace.end(),
                     "slot396:16384") == 1 &&
              std::count(runtime.trace.begin(), runtime.trace.end(),
                         "global_name") == 2,
          "only matching children receive callback and name lookup");
    Check(runtime.trace[2] == "write_byte:5256" &&
              runtime.trace[3] == "slot464",
          "owner state clears before iterator creation");

    Runtime empty_name;
    empty_name.bytes[actor + 1160u] = 1u;
    empty_name.members = {0x4000u};
    empty_name.words[0x4000u + 808u] = 0x7000u;
    empty_name.animation_names[0x7000u + 368u] = "";
    ActorManagerCombatActorMotion::Update(actor, 0u, empty_name);
    Check(std::count(empty_name.trace.begin(), empty_name.trace.end(),
                     "slot396:16384") == 1,
          "null global animation name uses empty fallback");

    Runtime no_iterator;
    no_iterator.bytes[actor + 1160u] = 1u;
    no_iterator.iterator_value = 0u;
    ActorManagerCombatActorMotion::Update(actor, 0u, no_iterator);
    Check(no_iterator.trace.back() == "slot464" &&
              no_iterator.bytes[actor + 1160u] == 0u,
          "null iterator leaves state cleared");

    Runtime throwing;
    throwing.bytes[actor + 1160u] = 1u;
    throwing.members = {0x4000u};
    throwing.throw_during_iteration = true;
    try {
        ActorManagerCombatActorMotion::Update(actor, 0u, throwing);
        Check(false, "iterator exception expected");
    } catch (const std::runtime_error&) {
        Check(throwing.trace.back() == "delete",
              "iterator released during unwind");
    }
}

void StartAnimation() {
    Runtime runtime;
    Check(ActorManagerCombatActorMotion::StartAnimation(actor, runtime) ==
              0x888u &&
              runtime.trace == std::vector<std::string>{
                  "slot396:4096", "child8", "write_byte:5256"} &&
              runtime.last_mode == 1u &&
              Near(runtime.last_angle.x, 0.0f) &&
              Near(runtime.last_angle.y, 180.0f) &&
              Near(runtime.last_angle.z, 0.0f) &&
              runtime.bytes[actor + 1160u] == 1u,
          "start animation sets 180 degree child angle then pending state");
}

}  // namespace

int main() {
    try {
        GatesAndMotion();
        Propagation();
        StartAnimation();
        std::cout << "actor_manager_combat_actor_motion_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
