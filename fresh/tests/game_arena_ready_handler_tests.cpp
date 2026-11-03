#include "pal4/game_arena_ready_handler.h"

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IGameArenaReadyRuntime {
    std::uint32_t actor_ready = 1;
    std::uint32_t arena_ready = 0;
    std::uint32_t restore_flag = 0;
    std::uint32_t current_actor = 0x8000u;
    std::uint32_t hint_window = 0x4000u;
    std::string arena = "q14";
    std::string area = "q14b";
    float y = 500.0f;
    float z = 0.0f;
    std::vector<pal4::ArenaReadyHintRecord> records;
    std::vector<std::string> calls;
    std::string throw_at;

    void Log(std::string name) {
        calls.push_back(name);
        if (name == throw_at) throw std::runtime_error("injected failure");
    }
    std::uint32_t ActorControl() override {
        Log("actor");
        return 0x1000u;
    }
    std::uint32_t ArenaManager() override {
        Log("arena");
        return 0x2000u;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        Log("read:" + std::to_string(address));
        if (address == 0x1000u) return actor_ready;
        if (address == 0x20c4u) return arena_ready;
        if (address == 0x21bcu) return restore_flag;
        if (address == 0x1004u) return current_actor;
        if (address == 0x7574u) return 456u;
        throw std::runtime_error("unexpected read address");
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        Log("write:" + std::to_string(address) + ":" +
            std::to_string(value));
    }
    std::string_view ReadText(std::uint32_t address) override {
        Log("text:" + std::to_string(address));
        if (address == 0x2008u) return arena;
        if (address == 0x20e0u) return area;
        throw std::runtime_error("unexpected text address");
    }
    std::uint32_t GameDatabase() override {
        Log("database");
        return 0x3000u;
    }
    std::span<const pal4::ArenaReadyHintRecord> HintRecords(
        std::uint32_t member) override {
        Check(member == 0x3034u, "database member offset");
        Log("hint-records");
        return records;
    }
    std::uint32_t CreateHintWindow() override {
        Log("create-hint");
        return hint_window;
    }
    void ShowHint(std::uint32_t window, std::string_view markup,
                  float x, float hint_y, float scale,
                  std::uint32_t first_flag,
                  std::uint32_t second_flag) override {
        Check(window == 0x4000u && x == 0.5f &&
              hint_y > 0.149f && hint_y < 0.151f && scale == 1.0f &&
              first_flag == 0u && second_flag == 1u,
              "hint geometry and flags");
        Log("show:" + std::string(markup));
    }
    std::int32_t ReleaseHintMatches() override {
        Log("release-hints");
        return -53;
    }
    void ProcessActorInput(std::uint32_t actor) override {
        Check(actor == 0x1000u, "actor input object");
        Log("process-actor");
    }
    void ProcessActorInputAndClearVector(std::uint32_t actor) override {
        Check(actor == 0x1000u, "actor vector object");
        Log("clear-vector");
    }
    std::uint32_t InputManager() override {
        Log("input");
        return 0x5000u;
    }
    void InitializeInputState(std::uint32_t input) override {
        Check(input == 0x5000u, "input object");
        Log("initialize-input");
    }
    std::uint32_t PetManager() override {
        Log("pet");
        return 0x6000u;
    }
    void InitializePetData(std::uint32_t pet) override {
        Check(pet == 0x6000u, "pet object");
        Log("initialize-pet");
    }
    std::uint32_t MinimapManager() override {
        Log("minimap");
        return 0x7000u;
    }
    void UpdateGrid(std::uint32_t minimap,
                    std::uint32_t position,
                    std::uint32_t direction) override {
        Check(minimap == 0x7000u && position == 0x80acu &&
              direction == 0x80a0u, "grid display operands");
        Log("grid");
    }
    float LeaderY() override {
        Log("leader-y");
        return y;
    }
    float LeaderZ() override {
        Log("leader-z");
        return z;
    }
    void SetMinimapTexture(std::uint32_t level) override {
        Log("texture:" + std::to_string(level));
    }
    std::uint32_t Milliseconds() override {
        Log("milliseconds");
        return 456u;
    }
    std::uint32_t GameState() override {
        Log("game-state");
        return 0x7000u;
    }
};

bool Has(const Runtime& runtime, const std::string& call) {
    return std::find(runtime.calls.begin(), runtime.calls.end(), call) !=
           runtime.calls.end();
}

void HintAndFinalState() {
    Runtime runtime;
    runtime.records = {{"OTHER", "Q14B", "wrong"},
                       {"Q14", "Q14B", "first"},
                       {"Q14", "Q14B", "second"}};
    Check(pal4::GameArenaReadyHandler::Run(runtime) == 0x2000,
          "returns last arena manager");
    Check(Has(runtime, "show:<dc0><df2>first</df2></dc0><br/>") &&
          !Has(runtime, "show:<dc0><df2>second</df2></dc0><br/>") &&
          Has(runtime, "texture:2"), "first matching hint and minimap level");
    Check(runtime.calls == std::vector<std::string>{
              "actor", "read:4096", "arena", "read:8388",
              "arena", "text:8416", "arena", "text:8200",
              "database", "hint-records", "create-hint",
              "show:<dc0><df2>first</df2></dc0><br/>", "release-hints",
              "actor", "process-actor", "actor", "clear-vector",
              "input", "initialize-input", "pet", "initialize-pet",
              "actor", "read:4100", "minimap", "grid",
              "arena", "text:8200", "arena", "text:8416",
              "leader-y", "texture:2", "milliseconds", "game-state",
              "write:30060:456", "game-state", "write:30068:456",
              "game-state", "read:30068", "game-state",
              "write:30068:456", "game-state", "write:30064:456",
              "arena", "write:8388:1", "arena", "write:8636:0"},
          "native stage ordering and timestamp/final flags");
}

void GateAndFailure() {
    Runtime no_match;
    no_match.records = {{"OTHER", "Q14B", "ignored"}};
    Check(pal4::GameArenaReadyHandler::Run(no_match) == 0x2000 &&
          Has(no_match, "release-hints") &&
          !Has(no_match, "create-hint") &&
          Has(no_match, "initialize-input") &&
          Has(no_match, "write:8636:0"),
          "no hint match releases collection and continues ready flow");

    Runtime no_actor;
    no_actor.actor_ready = 0;
    (void)pal4::GameArenaReadyHandler::Run(no_actor);
    Check(!Has(no_actor, "hint-records") &&
          !Has(no_actor, "release-hints") &&
          Has(no_actor, "initialize-input"), "actor gate skips hint path");

    Runtime already_ready;
    already_ready.arena_ready = 1;
    (void)pal4::GameArenaReadyHandler::Run(already_ready);
    Check(!Has(already_ready, "hint-records") &&
          Has(already_ready, "initialize-pet"),
          "arena gate skips hint path");

    Runtime failed_hint;
    failed_hint.records = {{"Q14", "Q14B", "hint"}};
    failed_hint.hint_window = 0;
    Check(pal4::GameArenaReadyHandler::Run(failed_hint) == -53 &&
          failed_hint.calls.back() == "release-hints" &&
          !Has(failed_hint, "initialize-input"),
          "hint creation failure returns release result early");

    Runtime throwing;
    throwing.records = {{"Q14", "Q14B", "hint"}};
    throwing.throw_at = "show:<dc0><df2>hint</df2></dc0><br/>";
    try {
        (void)pal4::GameArenaReadyHandler::Run(throwing);
        throw std::runtime_error("missing injected failure");
    } catch (const std::runtime_error& error) {
        Check(std::string_view(error.what()) == "injected failure",
              "original hint exception propagates");
    }
    Check(throwing.calls.back() == "release-hints",
          "hint collection released on exception");
}

void RestoreGate() {
    Runtime inactive;
    Check(pal4::GameArenaReadyHandler::Restore(inactive) == 0x2000 &&
          inactive.calls == std::vector<std::string>{"arena", "read:8636"},
          "clear restore flag returns manager without ready callback");

    Runtime active;
    active.restore_flag = 1;
    active.actor_ready = 0;
    Check(pal4::GameArenaReadyHandler::Restore(active) == 0x2000 &&
          active.calls[0] == "arena" && active.calls[1] == "read:8636" &&
          Has(active, "initialize-input") && Has(active, "write:8636:0"),
          "restore flag delegates to ready callback");
}

void MapCase(std::string arena, std::string area, float y, float z,
             int expected_level) {
    Runtime runtime;
    runtime.actor_ready = 0;
    runtime.current_actor = 0;
    runtime.arena = std::move(arena);
    runtime.area = std::move(area);
    runtime.y = y;
    runtime.z = z;
    (void)pal4::GameArenaReadyHandler::Run(runtime);
    Check(!Has(runtime, "grid"), "no current actor skips grid update");
    for (unsigned level = 0; level < 3; ++level)
        Check(Has(runtime, "texture:" + std::to_string(level)) ==
                  (expected_level == static_cast<int>(level)),
              "special minimap level and suppression");
}

void MinimapBoundaries() {
    MapCase("M15", "2", 3961.0f, 0.0f, 1);
    MapCase("m15", "2", 3550.0f, -1500.0f, 0);
    MapCase("M15", "2", 3550.1f, -1500.0f, 1);
    MapCase("M15", "2", 3630.0f, -1500.0f, 0);
    MapCase("M15", "2", 3600.0f, -1400.0f, 0);
    MapCase("M15", "3", 3600.0f, -1500.0f, -1);
    for (const auto* area : {"2", "3", "4"}) {
        MapCase("M08", area, 129.0f, 0.0f, 1);
        MapCase("M08", area, 130.0f, 0.0f, 0);
    }
    MapCase("M08", "5", 0.0f, 0.0f, -1);
    MapCase("Q06", "Q06Q", -1.0f, 0.0f, 1);
    MapCase("Q06", "Q06QY", 0.0f, 0.0f, 0);
    MapCase("Q06", "OTHER", -1.0f, 0.0f, -1);
    MapCase("Q14", "Q14B", 729.0f, 0.0f, 2);
    MapCase("Q14", "Q14B", 730.0f, 0.0f, 1);
    MapCase("Q14", "Q14B", 2616.0f, 0.0f, 0);
    const auto nan = std::numeric_limits<float>::quiet_NaN();
    MapCase("M15", "2", nan, 0.0f, 0);
    MapCase("M08", "2", nan, 0.0f, 1);
    MapCase("Q06", "Q06Q", nan, 0.0f, 1);
    MapCase("Q14", "Q14B", nan, 0.0f, 2);
    MapCase("Q14", "Q14A", 0.0f, 0.0f, -1);
    MapCase("OTHER", "Q14B", 0.0f, 0.0f, -1);
}

}  // namespace

int main() {
    try {
        HintAndFinalState();
        GateAndFailure();
        RestoreGate();
        MinimapBoundaries();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
