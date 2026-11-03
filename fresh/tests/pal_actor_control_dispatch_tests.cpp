#include "pal4/pal_actor_control_dispatch.h"

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(1);
    }
}

struct Runtime : pal4::IActorControlDispatchRuntime {
    pal4::PalActorControlState* state = nullptr;
    std::vector<std::string> events;
    bool interaction = false;
    bool motion = false;
    bool priority = false;
    bool database = false;

    bool HandlePlayerInteraction(pal4::PalActorControlState& value,
                                 std::uint32_t camera) override {
        Check(&value == state && camera == 0x300u,
              "interaction arguments");
        events.emplace_back("interaction");
        return interaction;
    }
    bool TryMotion(std::uint32_t* object, std::uint32_t actor,
                   float delta) override {
        Check(object == state->words.data() + 165u &&
                  actor == 0x100u && delta == 0.25f,
              "motion arguments");
        events.emplace_back("motion");
        return motion;
    }
    bool DatabaseHasPriority(std::uint32_t* object) override {
        Check(object == state->words.data() + 35u,
              "database priority offset");
        events.emplace_back("priority");
        return priority;
    }
    bool TryDatabase(std::uint32_t* object, std::uint32_t actor,
                     float delta) override {
        Check(object == state->words.data() + 35u &&
                  actor == 0x100u && delta == 0.25f,
              "database arguments");
        events.emplace_back("database");
        return database;
    }
    void BindDispatchSubobject(std::uint32_t* object,
                               std::uint32_t actor) override {
        const auto index = object - state->words.data();
        Check((index == 23 || index == 35 || index == 165) &&
                  actor == 0x100u, "bind arguments");
        events.emplace_back("bind " + std::to_string(index));
    }
    std::uint32_t SubobjectAddress(pal4::PalActorControlState& value,
                                    std::uint32_t index) override {
        Check(&value == state && (index == 35u || index == 165u),
              "subobject address arguments");
        events.emplace_back("address " + std::to_string(index));
        return 0x400000u + index * 4u;
    }
};

void TestEarlyExit() {
    pal4::PalActorControlState state{};
    state.words[1] = 0x100u;
    state.words[183] = 0xdeadbeefu;
    Runtime runtime;
    runtime.state = &state;
    Check(!pal4::PalActorControlDispatch::Select(
              state, 0.25f, 0u, 0x300u, runtime) &&
              state.words[183] == 0xdeadbeefu && runtime.events.empty(),
          "null actor preserves active subobject");
    Check(pal4::PalActorControlDispatch::Select(
              state, 0.25f, 0x100u, 0x300u, runtime) &&
              state.words[183] == 0xdeadbeefu && runtime.events.empty(),
          "active subobject returns before other calls");
    state.words[183] = 0u;
    runtime.interaction = true;
    Check(pal4::PalActorControlDispatch::Select(
              state, 0.25f, 0x100u, 0x300u, runtime) &&
              state.words[183] == 0u &&
              runtime.events == std::vector<std::string>{"interaction"},
          "interaction success leaves active pointer unchanged");
}

void TestMotionAndDatabase() {
    pal4::PalActorControlState state{};
    state.words[1] = 0x100u;
    Runtime runtime;
    runtime.state = &state;
    runtime.motion = true;
    Check(pal4::PalActorControlDispatch::Select(
              state, 0.25f, 0x100u, 0x300u, runtime) &&
              state.words[183] == 0x400000u + 165u * 4u &&
              runtime.events == std::vector<std::string>{
                  "interaction", "motion", "bind 35", "bind 23",
                  "address 165"},
          "motion wins and binds the other subobjects in order");
    state.words[183] = 0u;
    runtime.events.clear();
    runtime.motion = false;
    runtime.database = true;
    Check(pal4::PalActorControlDispatch::Select(
              state, 0.25f, 0x100u, 0x300u, runtime) &&
              state.words[183] == 0x400000u + 35u * 4u &&
              runtime.events == std::vector<std::string>{
                  "interaction", "motion", "priority", "database",
                  "bind 165", "bind 23", "address 35"},
          "database success binds motion and component in order");
}

void TestFailureAndPriority() {
    pal4::PalActorControlState state{};
    state.words[1] = 0x100u;
    Runtime runtime;
    runtime.state = &state;
    runtime.priority = true;
    runtime.database = true;
    Check(!pal4::PalActorControlDispatch::Select(
              state, 0.25f, 0x100u, 0x300u, runtime) &&
              state.words[183] == 0u &&
              runtime.events == std::vector<std::string>{
                  "interaction", "motion", "priority"},
          "database priority flag skips database call");
    runtime.events.clear();
    runtime.priority = false;
    runtime.database = false;
    Check(!pal4::PalActorControlDispatch::Select(
              state, 0.25f, 0x100u, 0x300u, runtime) &&
              state.words[183] == 0u &&
              runtime.events == std::vector<std::string>{
                  "interaction", "motion", "priority", "database"},
          "all failures clear active pointer");
}

}  // namespace

int main() {
    TestEarlyExit();
    TestMotionAndDatabase();
    TestFailureAndPriority();
}
