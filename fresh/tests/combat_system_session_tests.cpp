#include "pal4/combat_system_session.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace {

constexpr std::uint32_t kObject = 0x1000u;
constexpr std::uint32_t kManager = 0x2000u;

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::ICombatSystemSessionRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::unordered_map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> calls;

    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
        calls.emplace_back("write:" + std::to_string(address) + ":" +
                           std::to_string(value));
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        bytes[address] = value;
        calls.emplace_back("byte:" + std::to_string(address));
    }
    void LogInitialization(bool beginning) override {
        Check(beginning, "first log is initialization begin");
        calls.emplace_back("log-begin");
    }
    std::uint32_t LogInitialized() override {
        calls.emplace_back("log-end");
        return 0x1234u;
    }
    void GetSkillSystem() override { calls.emplace_back("skill"); }
    void LoadTranslator(std::uint32_t translator,
                        std::string_view path) override {
        Check(translator == kObject + 380u &&
                  path == "gamedata\\database\\combatDb.db",
              "translator address and exact database path");
        calls.emplace_back("load-translator");
    }
    std::uint32_t GetConsole() override {
        calls.emplace_back("get-console");
        return 0x3000u;
    }
    void SubscribeConsoleEvents(std::uint32_t console) override {
        Check(console == 0x3000u, "console subscription target");
        calls.emplace_back("subscribe");
    }
    void InitializeMatFx(std::uint32_t object) override {
        Check(object == kObject, "MatFx receives combat object");
        calls.emplace_back("matfx");
    }
    void EndCombat(std::uint32_t object) override {
        Check(object == kObject, "end combat target");
        calls.emplace_back("end-combat");
    }
    void EndCombatSequence(std::uint32_t object,
                           std::uint32_t mode) override {
        Check(object == kObject && mode == 0u,
              "end combat sequence mode");
        calls.emplace_back("end-sequence");
    }
    void SetRenderObject(std::uint32_t object) override {
        Check(object == 0x4444u, "render system receives object +428");
        calls.emplace_back("render");
    }
    std::uint32_t GetActorManager() override {
        calls.emplace_back("get-manager");
        return kManager;
    }
    void ResetTaskFlags(std::uint32_t queue) override {
        Check(queue == kManager + 96u, "task flag queue offset");
        calls.emplace_back("reset-flags");
    }
    void CleanupTaskQueue(std::uint32_t queue) override {
        Check(queue == kManager + 96u, "task cleanup queue offset");
        calls.emplace_back("cleanup-queue");
    }
    void DeinitializeGameObject(std::uint32_t object) override {
        Check(object == 0x5555u, "game object pointer at +424");
        calls.emplace_back("deinit-object");
    }
    void DestructActorManager(std::uint32_t manager) override {
        Check(manager == kManager, "actor manager destructor target");
        calls.emplace_back("destruct-manager");
    }
    void DestroyCombatDataRange(std::uint32_t begin,
                                std::uint32_t end) override {
        calls.emplace_back("range:" + std::to_string(begin) + ":" +
                           std::to_string(end));
    }
    void DestroyConsole(std::uint32_t console) override {
        Check(console == 0x3000u, "console cleanup target");
        calls.emplace_back("destroy-console");
    }
    void CleanupWorld(std::uint32_t object) override {
        Check(object == kObject, "world cleanup target");
        calls.emplace_back("cleanup-world");
    }
    void DestructTranslator(std::uint32_t translator) override {
        Check(translator == kObject + 380u,
              "translator destructor target");
        calls.emplace_back("destruct-translator");
    }
    void InitializeGameStats(std::uint32_t handler) override {
        Check(handler == 0x6666u, "game stats handler at +416");
        calls.emplace_back("reset-stats");
    }
};

void Initialization() {
    Runtime runtime;
    Check(pal4::CombatSystemSession::Initialize(kObject, runtime) ==
                  0x1234u &&
              runtime.bytes[kObject + 517u] == 1u &&
              runtime.calls == std::vector<std::string>{
                  "log-begin", "skill", "load-translator", "get-console",
                  "subscribe", "matfx", "skill", "log-end", "byte:4613"},
          "initialization stages and final flag order");
}

void EndAndState() {
    Runtime runtime;
    runtime.words[kObject + 428u] = 0x4444u;
    runtime.words[kObject + 424u] = 0x5555u;
    runtime.words[kObject + 416u] = 0x6666u;
    runtime.words[kManager + 68u] = 0x7100u;
    runtime.words[kManager + 72u] = 0x7200u;
    runtime.words[kManager + 84u] = 0x7300u;
    runtime.words[kManager + 88u] = 0x7400u;
    pal4::CombatSystemSession::End(kObject, runtime);
    Check(runtime.calls == std::vector<std::string>{
              "skill", "end-combat", "end-sequence", "render",
              "get-manager", "reset-flags", "get-manager",
              "cleanup-queue", "deinit-object", "get-manager",
              "destruct-manager", "get-manager", "range:28928:29184",
              "write:8264:28928", "range:29440:29696",
              "write:8280:29440", "get-console", "destroy-console",
              "cleanup-world", "destruct-translator", "reset-stats",
              "skill"},
          "end sequence follows all four actor manager lookups");
    Check(runtime.words[kManager + 72u] == 0x7100u &&
              runtime.words[kManager + 88u] == 0x7300u,
          "both combat data ranges are cleared");
    runtime.calls.clear();
    Check(pal4::CombatSystemSession::SetEndState(
              kObject, 0x12345678u, runtime) == 0x12345678u &&
              runtime.words[kObject + 508u] == 7u &&
              runtime.words[kObject + 504u] == 0x12345678u &&
              runtime.calls == std::vector<std::string>{
                  "write:4604:7", "write:4600:305419896"},
          "end state writes status before value and returns input");
}

}  // namespace

int main() {
    try {
        Initialization();
        EndAndState();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
