#include "pal4/skill_system_dispatch.h"
#include "pal4/crt_noop_atexit_registration.h"

#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::ISkillSystemDispatchRuntime,
                       pal4::ICrtNoopAtExitRuntime {
    std::map<std::uint32_t, std::uint32_t> memory;
    std::map<std::uint32_t, std::uint32_t> skills;
    std::vector<std::string> calls;
    bool mutate_id = false;
    std::uint8_t guard = 0x80u;

    std::uint32_t ReadWord(std::uint32_t address) override {
        return memory[address];
    }
    std::uint32_t LookupSkill(std::uint32_t id) override {
        calls.push_back("lookup " + std::to_string(id));
        return skills[id];
    }
    void ActorEvent(std::uint32_t slot, std::uint32_t actor,
                    std::uint32_t skill, std::uint32_t id,
                    std::uint32_t target) override {
        calls.push_back("actor " + std::to_string(slot) + " " +
                        std::to_string(actor) + " " +
                        std::to_string(skill) + " " +
                        std::to_string(id) + " " + std::to_string(target));
        if (mutate_id) memory[skill + 16u] = id + 1u;
    }
    void TargetEvent(std::uint32_t slot, std::uint32_t target,
                     std::uint32_t actor, std::uint32_t skill,
                     std::uint32_t id) override {
        calls.push_back("target " + std::to_string(slot) + " " +
                        std::to_string(target) + " " +
                        std::to_string(actor) + " " +
                        std::to_string(skill) + " " + std::to_string(id));
    }
    void InvokeCallback(std::uint32_t callback, std::uint32_t skill,
                        std::uint32_t context, std::uint32_t actor,
                        std::uint32_t target) override {
        calls.push_back("callback " + std::to_string(callback) + " " +
                        std::to_string(skill) + " " +
                        std::to_string(context) + " " +
                        std::to_string(actor) + " " +
                        std::to_string(target));
        if (mutate_id) ++memory[skill + 16u];
    }
    int RegisterAtExit(std::uint32_t callback) override {
        calls.push_back("atexit " + std::to_string(callback));
        return -3;
    }
};

void SetEntries(Runtime& runtime,
                const std::vector<std::pair<std::int32_t, std::uint32_t>>& entries) {
    runtime.memory[0x7000u + 2348u] = 0x8000u;
    runtime.memory[0x7000u + 2352u] = 0x8000u +
        static_cast<std::uint32_t>(entries.size()) * 8u;
    for (std::size_t i = 0u; i < entries.size(); ++i) {
        const auto address = 0x8000u + static_cast<std::uint32_t>(i) * 8u;
        runtime.memory[address] = static_cast<std::uint32_t>(entries[i].first);
        runtime.memory[address + 4u] = entries[i].second;
    }
}

void BinarySearch() {
    Runtime runtime;
    SetEntries(runtime, {{-4, 41u}, {10, 101u}, {10, 102u}, {20, 201u}});
    using pal4::SkillSystemDispatch;
    Check(SkillSystemDispatch::FindCallback(0x7000u, -4, runtime) == 41u &&
          SkillSystemDispatch::FindCallback(0x7000u, 10, runtime) == 101u &&
          SkillSystemDispatch::FindCallback(0x7000u, 20, runtime) == 201u,
          "signed lower bound selects first matching callback");
    Check(SkillSystemDispatch::FindCallback(0x7000u, -5, runtime) == 0u &&
          SkillSystemDispatch::FindCallback(0x7000u, 11, runtime) == 0u &&
          SkillSystemDispatch::FindCallback(0x7000u, 21, runtime) == 0u,
          "missing IDs before, within and after sorted range return zero");
    SetEntries(runtime, {});
    Check(SkillSystemDispatch::FindCallback(0x7000u, 10, runtime) == 0u,
          "empty vector returns zero without reading an entry");
}

void LinkedChain() {
    Runtime runtime;
    SetEntries(runtime, {{10, 101u}, {20, 201u}});
    runtime.memory[0x9000u + 16u] = 10u;
    runtime.memory[0x9000u + 56u] = 2u;
    runtime.memory[0x9100u + 16u] = 20u;
    runtime.skills[2u] = 0x9100u;
    Check(pal4::SkillSystemDispatch::ProcessChain(
              0x7000u, 0x9000u, 3u, 4u, 5u, runtime) == 0u &&
          runtime.calls == std::vector<std::string>{
              "actor 288 4 36864 10 5", "target 316 5 4 36864 10",
              "callback 101 36864 3 4 5", "actor 292 4 36864 10 5",
              "target 320 5 4 36864 10", "lookup 2",
              "actor 288 4 37120 20 5", "target 316 5 4 37120 20",
              "callback 201 37120 3 4 5", "actor 292 4 37120 20 5",
              "target 320 5 4 37120 20"},
          "two linked skills dispatch callbacks between actor and target hooks");

    runtime.calls.clear();
    runtime.skills.erase(2u);
    static_cast<void>(pal4::SkillSystemDispatch::ProcessChain(
        0x7000u, 0x9000u, 3u, 4u, 5u, runtime));
    Check(runtime.calls.size() == 6u && runtime.calls.back() == "lookup 2",
          "missing linked skill stops after the current callback");
    runtime.calls.clear();
    runtime.memory[0x9000u + 16u] = 11u;
    static_cast<void>(pal4::SkillSystemDispatch::ProcessChain(
        0x7000u, 0x9000u, 3u, 4u, 5u, runtime));
    Check(runtime.calls.empty(), "missing initial callback skips all hooks");
}

void MutableSkillIdAndCrt() {
    Runtime runtime;
    SetEntries(runtime, {{10, 101u}});
    runtime.memory[0x9000u + 16u] = 10u;
    runtime.mutate_id = true;
    static_cast<void>(pal4::SkillSystemDispatch::ProcessChain(
        0x7000u, 0x9000u, 3u, 4u, 5u, runtime));
    Check(runtime.calls == std::vector<std::string>{
              "actor 288 4 36864 10 5", "target 316 5 4 36864 11",
              "callback 101 36864 3 4 5", "actor 292 4 36864 12 5",
              "target 320 5 4 36864 13"},
          "native hook sequence reloads the skill ID after each mutating call");
    runtime.calls.clear();
    Check(pal4::CrtNoopAtExitRegistration::Run(runtime.guard, runtime) == -3 &&
          runtime.guard == 0x81u &&
          runtime.calls == std::vector<std::string>{"atexit 6161760"},
          "0x58a8d0 reuses shared CRT guard and no-op callback");
}

}  // namespace

int main() {
    try {
        BinarySearch();
        LinkedChain();
        MutableSkillIdAndCrt();
        std::cout << "skill system dispatch tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
