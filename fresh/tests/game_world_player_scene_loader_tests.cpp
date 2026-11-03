#include "pal4/game_world_player_scene_loader.h"
#include "pal4/game_arena_script_scene_callbacks.h"

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

struct Event {
    std::string name;
    std::uint32_t first = 0u;
    std::uint32_t second = 0u;
    bool operator==(const Event&) const = default;
};

struct Runtime final : pal4::IGameWorldPlayerSceneRuntime,
                       pal4::IGameArenaScriptSceneRuntime {
    std::uint32_t player = 0x6000u;
    std::uint32_t control = 0x7000u;
    std::map<std::uint32_t, std::string> texts;
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<pal4::GameArenaSceneRecord> records;
    std::vector<Event> events;

    void ResetPlayerData() override { events.push_back({"reset"}); }
    std::uint32_t CurrentPlayer() override {
        events.push_back({"player"});
        return player;
    }
    void SetPlayerStateFlag(std::uint32_t actor,
                            std::uint32_t value) override {
        words[actor + 0x1f0u] = value;
        events.push_back({"state", actor, value});
    }
    void SelectPlayerAnimation(std::uint32_t member,
                               std::uint32_t action_name_slot) override {
        events.push_back({"animation", member, action_name_slot});
    }
    void AddPlayerToScene(std::uint32_t world,
                          std::uint32_t actor) override {
        (void)pal4::GameArenaScriptSceneCallbacks::AddObject(
            world, actor, *this);
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        const auto found = words.find(address);
        Check(found != words.end(), "scene add reads a known object field");
        return found->second;
    }
    std::uint32_t Call(pal4::GameArenaScriptSceneCall operation,
                       std::uint32_t first, std::uint32_t second,
                       std::uint32_t third) override {
        using Call = pal4::GameArenaScriptSceneCall;
        switch (operation) {
        case Call::AddClump:
            events.push_back({"scene-clump", first, second});
            break;
        case Call::SetAnimationFlag:
            events.push_back({"scene-animation", first, second});
            break;
        case Call::RenderCamera:
            Check(third == 0x9000u, "scene add passes world resource");
            events.push_back({"scene-camera", first, second});
            break;
        case Call::ProcessClumpAtomics:
            events.push_back({"scene-atomics", first, second});
            break;
        default:
            Check(false, "unexpected scene add operation");
        }
        return 1u;
    }
    std::uint32_t ActorControl() override {
        events.push_back({"control"});
        return control;
    }
    void SetActor(std::uint32_t owner,
                  std::uint32_t actor) override {
        events.push_back({"actor", owner, actor});
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        events.push_back({"write", address, value});
    }
    std::string_view ReadText(std::uint32_t address) override {
        events.push_back({"text", address});
        return texts[address];
    }
    std::span<const pal4::GameArenaSceneRecord> SceneRecords() override {
        events.push_back({"records"});
        return records;
    }
    void SetActorWord28(std::uint32_t owner,
                        bool value) override {
        events.push_back({"word28", owner, value ? 1u : 0u});
    }
};

void SetDefaults(Runtime& runtime) {
    runtime.texts[0x5008u] = "world";
    runtime.texts[0x500cu] = "scene";
    runtime.words[0x5030u] = 1u;
    runtime.words[0x5010u] = 0x9000u;
    runtime.words[0x5194u] = 0u;
    runtime.words[0x6018u] = 0u;
    runtime.words[0x6014u] = 0x8000u;
}

void MissingPlayer() {
    Runtime runtime;
    runtime.player = 0u;
    Check(!pal4::GameWorldPlayerSceneLoader::Load(
              0x5000u, runtime) &&
              runtime.events ==
                  std::vector<Event>{{"reset"}, {"player"}},
          "missing player exits after reset and lookup");
}

void FirstMatchingRecord() {
    Runtime runtime;
    SetDefaults(runtime);
    runtime.records = {
        {0x100u, "WORLD", "OTHER", 1u},
        {0x200u, "WORLD", "SCENE", 1u},
        {0x300u, "WORLD", "SCENE", 2u},
    };
    Check(pal4::GameWorldPlayerSceneLoader::Load(
              0x5000u, runtime),
          "scene setup completes");
    Check(runtime.events == std::vector<Event>{
              {"reset"},
              {"player"},
              {"state", 0x6000u, 1u},
              {"animation", 0x6170u,
               pal4::GameWorldPlayerSceneLoader::kInitialAnimationStringSlot},
              {"scene-clump", 0x5000u, 0x8000u},
              {"scene-animation", 0x6170u, 1u},
              {"scene-camera", 0x6000u, 1u},
              {"scene-atomics", 0x8000u, 0x512cu},
              {"control"},
              {"actor", 0x7000u, 0x6000u},
              {"control"},
              {"write", 0x7110u, 0u},
              {"text", 0x500cu},
              {"text", 0x5008u},
              {"records"},
              {"control"},
              {"word28", 0x7000u, 0u},
          },
          "player setup order, C01 animation slot and first matching type follow IDA");
}

void OtherTypesAndNoMatch() {
    Runtime other;
    SetDefaults(other);
    other.records = {{0x200u, "WORLD", "SCENE", 7u}};
    Check(pal4::GameWorldPlayerSceneLoader::Load(
              0x5000u, other) &&
              other.events.back() == Event{"word28", 0x7000u, 1u},
          "non-one actor type sets control word");

    Runtime missing;
    SetDefaults(missing);
    missing.records = {{0x200u, "World", "SCENE", 1u}};
    Check(pal4::GameWorldPlayerSceneLoader::Load(
              0x5000u, missing) &&
              missing.events.back() == Event{"records"},
          "record comparison is exact after uppercasing lookup text");

    Runtime empty;
    SetDefaults(empty);
    Check(pal4::GameWorldPlayerSceneLoader::Load(
              0x5000u, empty) &&
              empty.events.back() == Event{"records"},
          "empty GameDB table succeeds without changing actor word");
}

void CStringInputs() {
    Runtime runtime;
    SetDefaults(runtime);
    runtime.texts[0x5008u] = std::string("world\0tail", 10);
    runtime.texts[0x500cu] = std::string("scene\0tail", 10);
    runtime.records = {{0x200u, "WORLD", "SCENE", 2u}};
    Check(pal4::GameWorldPlayerSceneLoader::Load(
              0x5000u, runtime) &&
              runtime.events.back() == Event{"word28", 0x7000u, 1u},
          "source C strings stop at first NUL before record lookup");
}

}  // namespace

int main() {
    try {
        MissingPlayer();
        FirstMatchingRecord();
        OtherTypesAndNoMatch();
        CStringInputs();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
