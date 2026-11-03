#include "pal4/script_trade_npc_behavior.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct TradeRuntime final : pal4::IScriptTradeSlotRuntime {
    std::vector<std::string> calls;
    std::uint32_t TradeSystem() override {
        calls.emplace_back("trade");
        return 0x2000u;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.push_back("read:" + std::to_string(address));
        return 0xabcdef01u;
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        calls.push_back("write:" + std::to_string(address) + ":" +
                        std::to_string(value));
    }
};

struct NpcRuntime final : pal4::IScriptNpcBehaviorRuntime {
    std::uint32_t npc = 0x4000u;
    std::uint32_t info = 0x5000u;
    std::uint32_t primary_result = 0x12u;
    unsigned manager_gets = 0;
    std::vector<std::string> calls;
    std::uint32_t NpcManager() override {
        calls.emplace_back("npc-manager");
        return 0x3000u + manager_gets++;
    }
    std::uint32_t FindNpc(std::uint32_t manager,
                          std::string_view name) override {
        Check(manager == 0x3000u, "NPC lookup uses first manager");
        calls.push_back("find-npc:" + std::string(name));
        return npc;
    }
    void CleanupEventsAndScripts(std::uint32_t value,
                                 std::uint32_t flag) override {
        Check(value == npc && flag == 0u, "pause cleanup operands");
        calls.emplace_back("cleanup-events");
    }
    std::uint32_t SetActionCode(std::uint32_t component,
                                std::string_view code) override {
        Check(component == npc + 0x170u, "NPC action component offset");
        calls.push_back("action:" + std::string(code));
        if (code == "C01") return primary_result;
        Check(code == "Z01", "fallback action code");
        return 0x34u;
    }
    std::uint32_t FindNpcInfo(std::uint32_t manager,
                              std::string_view name) override {
        Check(manager == 0x3001u, "NPC info uses second manager");
        calls.push_back("find-info:" + std::string(name));
        return info;
    }
    void ProcessEventsAndClear(std::uint32_t value,
                               std::uint32_t record,
                               std::uint32_t flag) override {
        Check(value == npc && record == info && flag == 0u,
              "resume event-processing operands");
        calls.emplace_back("process-events");
    }
};

void TradeSlots() {
    TradeRuntime runtime;
    Check(pal4::ScriptTradeSlots::Get(2, runtime) == 0xabcdef01u &&
          runtime.calls == std::vector<std::string>{"trade", "read:8204"},
          "trade read uses slot offset plus four and forwards value");
    runtime.calls.clear();
    Check(pal4::ScriptTradeSlots::Set(-1, 0x12345678u, runtime) == 0x2000u &&
          runtime.calls == std::vector<std::string>{
              "trade", "write:8192:305419896"},
          "negative index wraps to trade base and setter returns instance");
}

void PauseNpc() {
    NpcRuntime missing;
    missing.npc = 0u;
    Check(pal4::ScriptNpcBehavior::Pause(nullptr, missing) == 0u &&
          missing.calls == std::vector<std::string>{
              "npc-manager", "find-npc:"},
          "null name uses empty string and missing NPC returns early");

    NpcRuntime primary;
    Check(pal4::ScriptNpcBehavior::Pause("npc", primary) == 0x12u &&
          primary.calls == std::vector<std::string>{
              "npc-manager", "find-npc:npc", "cleanup-events", "action:C01"},
          "successful C01 action skips fallback");

    NpcRuntime fallback;
    fallback.primary_result = 0u;
    Check(pal4::ScriptNpcBehavior::Pause("npc", fallback) == 0x34u &&
          fallback.calls == std::vector<std::string>{
              "npc-manager", "find-npc:npc", "cleanup-events",
              "action:C01", "action:Z01"},
          "failed C01 action tries Z01 and forwards result");
}

void ResumeNpc() {
    NpcRuntime missing;
    missing.npc = 0u;
    pal4::ScriptNpcBehavior::Resume("npc", missing);
    Check(missing.calls == std::vector<std::string>{
              "npc-manager", "find-npc:npc"},
          "missing NPC skips info search");

    NpcRuntime no_info;
    no_info.info = 0u;
    pal4::ScriptNpcBehavior::Resume("npc", no_info);
    Check(no_info.calls == std::vector<std::string>{
              "npc-manager", "find-npc:npc", "npc-manager", "find-info:npc"},
          "missing info skips event processing");

    NpcRuntime present;
    pal4::ScriptNpcBehavior::Resume(nullptr, present);
    Check(present.calls == std::vector<std::string>{
              "npc-manager", "find-npc:", "npc-manager", "find-info:",
              "process-events"},
          "resume searches twice with empty fallback then processes events");
}

}  // namespace

int main() {
    try {
        TradeSlots();
        PauseNpc();
        ResumeNpc();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
