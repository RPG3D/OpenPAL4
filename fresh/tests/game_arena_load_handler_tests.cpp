#include "pal4/game_arena_load_handler.h"

#include <array>
#include <cstdint>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

constexpr std::array<std::string_view, 4> kKeys = {
    "PALGAMEIV_LOADING_TYPE", "PALGAMEIV_LOADING_ARENA",
    "PALGAMEIV_LOADING_AREA", "PALGAMEIV_LOADING_AREA_RES"};

struct Runtime final : pal4::IGameArenaLoadRuntime {
    std::uint32_t arena_state = 1;
    unsigned existing_mask = 0;
    std::string throw_at;
    std::vector<std::string> calls;
    unsigned arena_gets = 0;
    unsigned state_gets = 0;
    unsigned actor_gets = 0;
    unsigned current_key = 0;

    void Log(std::string value) {
        calls.push_back(value);
        if (value == throw_at) throw std::runtime_error("injected failure");
    }
    std::uint32_t ArenaManager() override {
        Log("arena-manager");
        return 0x1000u + 0x100u * arena_gets++;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        Log("read:" + std::to_string(address));
        if (address == 0x10c4u) return arena_state;
        Check(address == 0x7574u, "timestamp reread address");
        return 987u;
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        Log("write:" + std::to_string(address) + ":" +
            std::to_string(value));
    }
    std::uint32_t InitializeXmlDocument() override {
        Log("document");
        return 0x2000u;
    }
    std::optional<std::uint32_t> FindAttribute(
        std::uint32_t document, std::string_view key) override {
        Check(document == 0x2000u, "lookup document");
        for (unsigned i = 0; i < kKeys.size(); ++i) {
            if (key == kKeys[i]) {
                current_key = i;
                Log("find:" + std::to_string(i));
                return existing_mask & (1u << i)
                    ? std::optional<std::uint32_t>(0x3000u + i)
                    : std::nullopt;
            }
        }
        throw std::runtime_error("unexpected XML key");
    }
    void SetInt(std::uint32_t attribute, std::int32_t value) override {
        Check(attribute == 0x3000u && value == 0, "existing int");
        Log("set-int:0");
    }
    void SetString(std::uint32_t attribute,
                   std::string_view value) override {
        Check(attribute == 0x3000u + current_key, "existing string");
        Log("set-string:" + std::string(value));
    }
    std::uint32_t InitializeIntAttribute(
        std::string_view key, std::int32_t value) override {
        Check(key == kKeys[0] && value == 0, "new int");
        Log("init-int:0");
        return 0x4000u;
    }
    std::uint32_t InitializeStringAttribute(
        std::string_view key, std::string_view value) override {
        Check(key == kKeys[current_key], "new string key");
        Log("init-string:" + std::string(value));
        return 0x4000u + current_key;
    }
    void AddAttribute(std::uint32_t document,
                      std::uint32_t attribute) override {
        Check(document == 0x2000u &&
              attribute == 0x4000u + current_key, "add operands");
        Log("add:" + std::to_string(current_key));
    }
    void DestroyAttribute(std::uint32_t attribute) override {
        Check(attribute == 0x4000u + current_key, "temporary cleanup");
        Log("destroy-attribute:" + std::to_string(current_key));
    }
    std::uint32_t GameInstance() override {
        Log("game");
        return 0x5000u;
    }
    void InitializeStateAndUi(std::uint32_t game,
                              std::uint32_t state,
                              std::uint32_t document) override {
        Check(game == 0x5000u && state == 6u && document == 0x2000u,
              "state transition operands");
        Log("state:6");
    }
    std::uint32_t ActorControl() override {
        Log("actor-control");
        return 0x6000u + actor_gets++;
    }
    void ProcessActorInput(std::uint32_t actor) override {
        Check(actor == 0x6000u, "first actor control");
        Log("process-actor");
    }
    void ProcessActorInputAndClearVector(std::uint32_t actor) override {
        Check(actor == 0x6001u, "second actor control");
        Log("clear-actor-vector");
    }
    std::uint32_t InputManager() override {
        Log("input-manager");
        return 0x6800u;
    }
    void InitializeInputState(std::uint32_t input) override {
        Check(input == 0x6800u, "input manager operand");
        Log("initialize-input");
    }
    std::uint32_t Milliseconds() override {
        Log("milliseconds");
        return 987u;
    }
    std::uint32_t GameState() override {
        Log("game-state");
        ++state_gets;
        return 0x7000u;
    }
    std::int32_t DestroyXmlDocument(std::uint32_t document) override {
        Check(document == 0x2000u, "document cleanup");
        Log("destroy-document");
        return -47;
    }
};

constexpr pal4::GameLoadArgument kArena{"arena"};
constexpr pal4::GameLoadArgument kArea{"area"};
constexpr pal4::GameLoadArgument kResource{"res"};

void StateGate() {
    for (const auto state : {0u, 2u, 0xffffffffu}) {
        Runtime runtime;
        runtime.arena_state = state;
        Check(pal4::GameArenaLoadHandler::Load(
                  kArena, kArea, kResource, runtime) ==
                  static_cast<std::int32_t>(state),
              "non-ready state returned unchanged");
        Check(runtime.calls == std::vector<std::string>{
                  "arena-manager", "read:4292"},
              "non-ready state has no side effects");
    }
}

void AllAttributeCombinations() {
    for (unsigned mask = 0; mask < 16; ++mask) {
        Runtime runtime;
        runtime.existing_mask = mask;
        Check(pal4::GameArenaLoadHandler::Load(
                  kArena, kArea, kResource, runtime) == -47,
              "document destructor result");
        std::vector<std::string> expected = {
            "arena-manager", "read:4292", "arena-manager",
            "write:4548:0", "document"};
        const std::array<std::string, 4> values = {"0", "arena", "area", "res"};
        for (unsigned i = 0; i < 4; ++i) {
            expected.push_back("find:" + std::to_string(i));
            if (mask & (1u << i)) {
                expected.push_back(std::string(i == 0 ? "set-int:" :
                                                   "set-string:") + values[i]);
            } else {
                expected.push_back(std::string(i == 0 ? "init-int:" :
                                                   "init-string:") + values[i]);
                expected.push_back("add:" + std::to_string(i));
                expected.push_back("destroy-attribute:" + std::to_string(i));
            }
        }
        expected.insert(expected.end(), {
            "game", "state:6", "actor-control", "process-actor",
            "actor-control", "clear-actor-vector", "input-manager",
            "initialize-input", "milliseconds", "game-state",
            "write:30060:987", "game-state", "write:30068:987",
            "game-state", "read:30068", "game-state",
            "write:30068:987", "game-state", "write:30064:987",
            "destroy-document"});
        Check(runtime.calls == expected, "exact XML, input and time order");
        Check(runtime.arena_gets == 2 && runtime.actor_gets == 2 &&
              runtime.state_gets == 5, "repeated singleton access counts");
    }
}

void NullStrings() {
    Runtime runtime;
    runtime.existing_mask = 0b1010u;
    (void)pal4::GameArenaLoadHandler::Load(
        {nullptr}, {nullptr}, {nullptr}, runtime);
    Check(runtime.calls[10] == "set-string:" &&
          runtime.calls[12] == "init-string:" &&
          runtime.calls[16] == "set-string:",
          "null native string pointer uses empty string");
}

void ExceptionCleanup() {
    for (unsigned i = 0; i < 4; ++i) {
        Runtime runtime;
        runtime.throw_at = "add:" + std::to_string(i);
        try {
            (void)pal4::GameArenaLoadHandler::Load(
                kArena, kArea, kResource, runtime);
            throw std::runtime_error("missing injected failure");
        } catch (const std::runtime_error& error) {
            Check(std::string_view(error.what()) == "injected failure",
                  "original add exception propagates");
        }
        Check(runtime.calls[runtime.calls.size() - 2] ==
                  "destroy-attribute:" + std::to_string(i) &&
              runtime.calls.back() == "destroy-document",
              "temporary attribute then document cleaned on add failure");
    }
    for (const auto* stage : {"state:6", "process-actor", "initialize-input"}) {
        Runtime runtime;
        runtime.throw_at = stage;
        try {
            (void)pal4::GameArenaLoadHandler::Load(
                kArena, kArea, kResource, runtime);
            throw std::runtime_error("missing injected failure");
        } catch (const std::runtime_error& error) {
            Check(std::string_view(error.what()) == "injected failure",
                  "original runtime exception propagates");
        }
        Check(runtime.calls.back() == "destroy-document",
              "document cleaned on downstream failure");
    }
}

}  // namespace

int main() {
    try {
        StateGate();
        AllAttributeCombinations();
        NullStrings();
        ExceptionCleanup();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
