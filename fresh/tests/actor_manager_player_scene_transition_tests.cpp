#include "pal4/actor_manager_player_scene_transition.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

constexpr std::uint32_t actor = 0x1000u;

struct Runtime final : pal4::IActorManagerPlayerSceneRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::map<std::uint32_t, std::string> names;
    std::vector<std::uint32_t> actors;
    std::vector<std::string> trace;
    std::uint32_t iterator_index = 0u;
    std::uint32_t iterator_handle = 0x9000u;
    std::uint32_t actor_slot = 0u;
    std::uint32_t state = 0u;
    std::uint32_t refresh = 0u;
    std::uint32_t selected = 0u;
    std::uint32_t state_calls = 0u;
    std::uint32_t refresh_calls = 0u;
    std::uint32_t move_result = 0x55u;
    bool ready = true;
    bool throw_name = false;
    std::string match = "idle";

    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        return bytes[address];
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        trace.emplace_back("byte:" + std::to_string(address) + ":" +
                           std::to_string(value));
        bytes[address] = value;
    }
    bool IsActorReady(std::uint32_t actual_actor) override {
        Check(actual_actor == actor, "ready actor");
        trace.emplace_back("ready");
        return ready;
    }
    std::uint32_t ResolveAnimationTransform(std::uint32_t actual_actor,
                                            std::uint32_t mode) override {
        Check(actual_actor == actor && mode == 0u, "transform request");
        trace.emplace_back("resolve");
        return 0x7000u;
    }
    void SetAnimationTransform(std::uint32_t animation,
                               std::uint32_t transform) override {
        Check(animation == 0x6000u && transform == 0x7000u,
              "animation transform");
        trace.emplace_back("transform");
    }
    std::uint32_t ActorSlot(std::uint32_t actual_actor) override {
        Check(actual_actor == actor, "actor kind receiver");
        trace.emplace_back("kind");
        return actor_slot;
    }
    std::uint32_t MoveChild(std::uint32_t actual_actor,
                            const std::array<float, 3>& position,
                            std::uint32_t mode) override {
        Check(actual_actor == actor && position[0] == 0.0f &&
                  position[2] == 0.0f, "child position axes");
        trace.emplace_back("move:" + std::to_string(position[1]) + ":" +
                           std::to_string(mode));
        return move_result;
    }
    std::int32_t QueryCurrentStat(std::uint32_t actual_actor,
                      std::uint32_t mode) override {
        Check(actual_actor == actor && mode == 7u, "current HP query index seven");
        trace.emplace_back("hp-query");
        return 123;
    }
    std::uint32_t SecondaryState(std::uint32_t actual_actor) override {
        Check(actual_actor == actor, "refresh receiver");
        ++refresh_calls;
        trace.emplace_back("refresh");
        return refresh;
    }
    std::uint32_t PrimaryState(std::uint32_t actual_actor) override {
        Check(actual_actor == actor, "state receiver");
        ++state_calls;
        trace.emplace_back("state");
        return state;
    }
    void SelectAnimation(std::uint32_t actual_actor,
                         std::uint32_t descriptor,
                         std::uint32_t enabled) override {
        Check(actual_actor == actor && enabled == 1u,
              "animation selection arguments");
        selected = descriptor;
        trace.emplace_back("select");
    }
    std::uint32_t CreateActorIterator(std::uint32_t actual_actor) override {
        Check(actual_actor == actor, "iterator owner");
        trace.emplace_back("create");
        return iterator_handle;
    }
    bool IteratorDone(std::uint32_t iterator) override {
        Check(iterator == iterator_handle, "iterator done receiver");
        return iterator_index >= actors.size();
    }
    std::uint32_t IteratorActor(std::uint32_t iterator) override {
        Check(iterator == iterator_handle, "iterator actor receiver");
        return actors.at(iterator_index);
    }
    void AdvanceIterator(std::uint32_t iterator) override {
        Check(iterator == iterator_handle, "iterator advance receiver");
        ++iterator_index;
        trace.emplace_back("advance");
    }
    void DestroyIterator(std::uint32_t iterator) override {
        Check(iterator == iterator_handle, "iterator destroy receiver");
        trace.emplace_back("destroy");
    }
    std::string_view MatchAnimationName() override { return match; }
    std::string_view AnimationName(std::uint32_t animation) override {
        if (throw_name) throw std::runtime_error("name read failed");
        return names.at(animation);
    }
    void ActivateActor(std::uint32_t actual_actor,
                       std::uint32_t active) override {
        Check(active == 1u, "actor activation flag");
        trace.emplace_back("activate:" + std::to_string(actual_actor));
    }
};

std::size_t Position(const std::vector<std::string>& trace,
                     const std::string& event) {
    const auto it = std::find(trace.begin(), trace.end(), event);
    Check(it != trace.end(), "scene transition operation missing");
    return static_cast<std::size_t>(it - trace.begin());
}

void BeginSetsFlagAfterMove() {
    Runtime runtime;
    Check(pal4::ActorManagerPlayerSceneTransition::Begin(actor, runtime) ==
              runtime.move_result &&
              runtime.bytes.at(actor + 1308u) == 1u &&
              runtime.trace == std::vector<std::string>{
                  "activate:4096", "move:180.000000:1", "byte:5404:1"},
          "begin activates, rotates 180 degrees and sets pending flag");
}

void UpdateGatesAndPrimary() {
    Runtime empty;
    pal4::ActorManagerPlayerSceneTransition::Update(actor, 1u, empty);
    Check(empty.trace.empty(), "null animation skips ready and pose calls");

    Runtime not_ready;
    not_ready.words[actor + 808u] = 0x6000u;
    not_ready.ready = false;
    pal4::ActorManagerPlayerSceneTransition::Update(actor, 1u, not_ready);
    Check(not_ready.trace == std::vector<std::string>{"ready"},
          "unready actor skips pose calls");

    Runtime primary;
    primary.words[actor + 808u] = 0x6000u;
    primary.actor_slot = 2u;
    primary.state = 4u;
    pal4::ActorManagerPlayerSceneTransition::Update(actor, 0x101u, primary);
    Check(primary.selected ==
              pal4::ActorManagerPlayerSceneTransition::kPrimaryAnimation &&
              primary.state_calls == 1u && primary.refresh_calls == 1u &&
              Position(primary.trace, "resolve") <
                  Position(primary.trace, "transform") &&
              Position(primary.trace, "transform") <
                  Position(primary.trace, "move:206.000000:0") &&
              Position(primary.trace, "move:206.000000:0") <
                  Position(primary.trace, "hp-query") &&
              Position(primary.trace, "hp-query") <
                  Position(primary.trace, "select"),
          "low flag byte syncs current, slot two adds 26 and state four selects primary");

    Runtime state_one;
    state_one.words[actor + 808u] = 0x6000u;
    state_one.state = 1u;
    pal4::ActorManagerPlayerSceneTransition::Update(actor, 0x100u,
                                                     state_one);
    Check(state_one.selected ==
              pal4::ActorManagerPlayerSceneTransition::kPrimaryAnimation &&
              state_one.state_calls == 2u &&
              std::find(state_one.trace.begin(), state_one.trace.end(),
                        "resolve") == state_one.trace.end(),
          "state one uses primary after second query and high flag byte does not sync");
}

void UpdateSecondaryAndDefault() {
    Runtime secondary;
    secondary.words[actor + 808u] = 0x6000u;
    secondary.refresh = 2u;
    pal4::ActorManagerPlayerSceneTransition::Update(actor, 0u,
                                                     secondary);
    Check(secondary.selected ==
              pal4::ActorManagerPlayerSceneTransition::kSecondaryAnimation &&
              secondary.refresh_calls == 2u &&
              secondary.state_calls == 2u &&
              Position(secondary.trace, "move:180.000000:0") <
                  Position(secondary.trace, "select"),
          "refresh result two selects secondary animation");

    Runtime fallback;
    fallback.words[actor + 808u] = 0x6000u;
    pal4::ActorManagerPlayerSceneTransition::Update(actor, 0u, fallback);
    Check(fallback.selected ==
              pal4::ActorManagerPlayerSceneTransition::kDefaultAnimation &&
              fallback.refresh_calls == 2u,
          "other state and refresh result select default animation");
}

void PendingIterationAndCleanup() {
    Runtime runtime;
    runtime.bytes[actor + 1308u] = 1u;
    runtime.actors = {0x2000u, 0x3000u, 0x4000u};
    runtime.words[0x2000u + 808u] = 0x7000u;
    runtime.words[0x3000u + 808u] = 0u;
    runtime.words[0x4000u + 808u] = 0x7100u;
    runtime.bytes[0x2000u + 1308u] = 1u;
    runtime.names[0x7000u] = "IDLE";
    runtime.names[0x7100u] = "walk";
    pal4::ActorManagerPlayerSceneTransition::Update(actor, 0u, runtime);
    Check(runtime.bytes.at(actor + 1308u) == 0u &&
              runtime.bytes.at(0x2000u + 1308u) == 0u &&
              runtime.trace.front() == "byte:5404:0" &&
              Position(runtime.trace, "create") <
                  Position(runtime.trace, "activate:8192") &&
              runtime.trace.back() == "destroy" &&
              runtime.iterator_index == 3u &&
              std::count(runtime.trace.begin(), runtime.trace.end(),
                         "activate:8192") == 1 &&
              std::find(runtime.trace.begin(), runtime.trace.end(),
                        "activate:16384") == runtime.trace.end(),
          "pending flag scans all actors, matches case insensitively and destroys iterator");

    Runtime throwing;
    throwing.bytes[actor + 1308u] = 1u;
    throwing.actors = {0x2000u};
    throwing.words[0x2000u + 808u] = 0x7000u;
    throwing.throw_name = true;
    bool caught = false;
    try {
        pal4::ActorManagerPlayerSceneTransition::Update(actor, 0u,
                                                         throwing);
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught && throwing.trace.back() == "destroy",
          "iterator is destroyed during exceptional name lookup");
}

}  // namespace

int main() {
    try {
        BeginSetsFlagAfterMove();
        UpdateGatesAndPrimary();
        UpdateSecondaryAndDefault();
        PendingIterationAndCleanup();
        std::cout << "actor_manager_player_scene_transition_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
