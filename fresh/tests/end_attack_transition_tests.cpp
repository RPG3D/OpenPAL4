#include "pal4/end_attack_task.h"

#include "pal4/start_attack_task.h"

#include <algorithm>
#include <bit>
#include <cstdint>
#include <iostream>
#include <limits>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

constexpr std::uint32_t kTask = 0x1000u;
constexpr std::uint32_t kSource = 0x2000u;
constexpr std::uint32_t kTarget = 0x3000u;
constexpr std::uint32_t kNext = 0x4000u;

struct Runtime final : pal4::IEndAttackTransitionRuntime {
    std::map<std::uint32_t, std::uint32_t> words{
        {kTask + 44u, kSource}, {kTask + 48u, kTarget},
        {0x8b8b58u, 0x7777u}};
    std::map<std::uint32_t, std::uint8_t> bytes{
        {kTask + 52u, 1u}};
    std::vector<std::string> calls;
    bool ready = true;
    bool target_ready = true;
    float chance = 0.75f;
    std::uint32_t random = 500u;
    std::uint32_t source_status = 2u;
    std::uint32_t target_status = 0u;
    std::uint32_t allocation = kNext;
    bool throw_on_name = false;
    bool throw_on_init = false;

    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.push_back("read:" + std::to_string(address));
        return words.at(address);
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        calls.push_back("write:" + std::to_string(address));
        words[address] = value;
    }
    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override {
        calls.push_back("write_byte:" + std::to_string(address));
        bytes[address] = value;
    }
    void NotifySource(std::uint32_t actor) override {
        Check(actor == kSource, "source notification receives source actor");
        calls.push_back("notify");
    }
    std::uint32_t CombatSystem() override {
        calls.push_back("combat_system");
        return 0x5000u;
    }
    bool AllCombatObjectsReady(std::uint32_t combat) override {
        Check(combat == 0x5000u, "readiness receives combat system");
        calls.push_back("ready");
        return ready;
    }
    void DispatchEvent(std::uint32_t key,
                       std::uint32_t first,
                       std::uint32_t second,
                       std::uint32_t mode) override {
        Check(key == 0x7777u && first == 0u && second == 0u &&
                  mode == 1u,
              "select_join_atk event uses zero payload and mode one");
        calls.push_back("dispatch");
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        calls.push_back("read_byte:" + std::to_string(address));
        return bytes.at(address);
    }
    bool IsActorReady(std::uint32_t actor) override {
        Check(actor == kTarget, "readiness receives target actor");
        calls.push_back("target_ready");
        return target_ready;
    }
    float ActorChance(std::uint32_t actor,
                      std::uint32_t index) override {
        Check(actor == kTarget && index == 4u,
              "chance uses target virtual slot and index four");
        calls.push_back("chance");
        return chance;
    }
    std::uint32_t RandomNumber() override {
        calls.push_back("random");
        return random;
    }
    std::uint32_t SourceStatus(std::uint32_t actor) override {
        Check(actor == kSource, "source status receives source");
        calls.push_back("source_status");
        return source_status;
    }
    std::uint32_t TargetStatus(std::uint32_t actor) override {
        Check(actor == kTarget, "target status receives target");
        calls.push_back("target_status");
        return target_status;
    }
    void TargetSetSource(std::uint32_t target,
                         std::uint32_t source) override {
        Check(target == kTarget && source == kSource,
              "target bind receives actor pair");
        calls.push_back("bind");
    }
    std::uint32_t Allocate(std::uint32_t size) override {
        Check(size == pal4::StartAttackTask::kSize,
              "StartAttack successor allocates 56 bytes");
        calls.push_back("allocate");
        return allocation;
    }
    void Free(std::uint32_t address) override {
        Check(address == kNext, "free receives successor task");
        calls.push_back("free");
    }
    void InitializeContainer(std::uint32_t container) override {
        Check(container == kNext + 4u,
              "successor GOM container begins at +4");
        calls.push_back("init_container");
        if (throw_on_init) throw std::runtime_error("init");
    }
    void DestroyContainer(std::uint32_t container) override {
        Check(container == kNext + 4u,
              "GOM cleanup begins at +4");
        calls.push_back("destroy_container");
    }
    void AssignName(std::uint32_t name,
                    std::string_view value) override {
        Check(name == kNext + 20u && value == "StartAttack",
              "successor receives StartAttack name");
        calls.push_back("assign_name");
        if (throw_on_name) throw std::runtime_error("name");
        words[kNext + 28u] = static_cast<std::uint32_t>(value.size());
    }
    void CleanupName(std::uint32_t name) override {
        Check(name == kNext + 20u, "name cleanup begins at +20");
        calls.push_back("cleanup_name");
    }
};

std::size_t Index(const Runtime& runtime, const std::string& call) {
    const auto it = std::find(runtime.calls.begin(), runtime.calls.end(), call);
    Check(it != runtime.calls.end(), "expected runtime call");
    return static_cast<std::size_t>(it - runtime.calls.begin());
}

void EventAndEarlyGates() {
    Runtime runtime;
    runtime.ready = false;
    Check(pal4::EndAttackTask::CreateNext(kTask, runtime) == 0u &&
          runtime.calls == std::vector<std::string>{
              "read:4140", "notify", "combat_system", "ready",
              "read:9145176", "dispatch"},
          "not-ready combat still dispatches event before return");
    runtime = Runtime{};
    runtime.bytes[kTask + 52u] = 0u;
    Check(pal4::EndAttackTask::CreateNext(kTask, runtime) == 0u &&
          runtime.calls.back() == "read_byte:4148",
          "clear task flag returns after event");
    runtime = Runtime{};
    runtime.target_ready = false;
    Check(pal4::EndAttackTask::CreateNext(kTask, runtime) == 0u &&
          runtime.calls.back() == "target_ready",
          "unready target returns before chance and randomness");
}

void ProbabilityAndStatusGates() {
    Runtime runtime;
    runtime.chance = 0.25f;
    Check(pal4::EndAttackTask::CreateNext(kTask, runtime) == 0u &&
          runtime.calls.back() == "random",
          "chance below random threshold rejects attack");
    runtime = Runtime{};
    runtime.chance = 0.0f;
    runtime.random = 0u;
    Check(pal4::EndAttackTask::CreateNext(kTask, runtime) == 0u,
          "equal chance and threshold rejects attack");
    runtime = Runtime{};
    runtime.chance = std::numeric_limits<float>::quiet_NaN();
    Check(pal4::EndAttackTask::CreateNext(kTask, runtime) == 0u,
          "unordered chance rejects attack");
    runtime = Runtime{};
    runtime.source_status = 4u;
    Check(pal4::EndAttackTask::CreateNext(kTask, runtime) == 0u &&
          runtime.calls.back() == "source_status",
          "source status four short-circuits target status");
    runtime = Runtime{};
    runtime.target_status = 1u;
    Check(pal4::EndAttackTask::CreateNext(kTask, runtime) == 0u &&
          runtime.calls.back() == "target_status",
          "target status one prevents binding");
}

void SuccessAndFailure() {
    Runtime runtime;
    Check(pal4::EndAttackTask::CreateNext(kTask, runtime) == kNext &&
          runtime.words[kNext] == pal4::StartAttackTask::kVtable &&
          runtime.words[kNext + 24u] == 0u &&
          runtime.words[kNext + 28u] == 11u &&
          runtime.words[kNext + 32u] == 0u &&
          runtime.words[kNext + 36u] == 0u &&
          runtime.words[kNext + 44u] == kTarget &&
          runtime.words[kNext + 48u] == kSource &&
          runtime.bytes[kNext + 20u] ==
              static_cast<std::uint8_t>(
                  std::bit_cast<std::uint32_t>(runtime.chance) >> 24u) &&
          runtime.bytes[kNext + 40u] == 1u &&
          runtime.bytes[kNext + 52u] == 0u &&
          runtime.bytes[kNext + 53u] == 0u &&
          runtime.bytes[kNext + 54u] == 0u,
          "success creates StartAttack with swapped actors and chance byte");
    Check(Index(runtime, "dispatch") < Index(runtime, "chance") &&
          Index(runtime, "bind") < Index(runtime, "allocate") &&
          Index(runtime, "init_container") <
              Index(runtime, "assign_name") &&
          runtime.calls.back() ==
              "write:" + std::to_string(kNext),
          "event, bind, allocation and final vtable follow IDA order");

    runtime = Runtime{};
    runtime.allocation = 0u;
    Check(pal4::EndAttackTask::CreateNext(kTask, runtime) == 0u &&
          runtime.calls.back() == "allocate" &&
          Index(runtime, "bind") < Index(runtime, "allocate"),
          "binding side effect precedes failed allocation");
}

void ExceptionCleanup() {
    Runtime runtime;
    runtime.throw_on_name = true;
    bool caught = false;
    try {
        (void)pal4::EndAttackTask::CreateNext(kTask, runtime);
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught &&
          Index(runtime, "cleanup_name") <
              Index(runtime, "destroy_container") &&
          runtime.calls.back() == "free",
          "name failure releases name, container and storage");
    runtime = Runtime{};
    runtime.throw_on_init = true;
    caught = false;
    try {
        (void)pal4::EndAttackTask::CreateNext(kTask, runtime);
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught && runtime.calls.back() == "free" &&
          std::find(runtime.calls.begin(), runtime.calls.end(),
                    "destroy_container") == runtime.calls.end(),
          "container init failure frees raw allocation");
}

}  // namespace

int main() {
    EventAndEarlyGates();
    ProbabilityAndStatusGates();
    SuccessAndFailure();
    ExceptionCleanup();
    std::cout << "EndAttack transition tests passed\n";
}
