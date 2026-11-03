#include "pal4/combat_action_registry.h"

#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

constexpr std::uint32_t kManager = 0x1000u;
constexpr std::uint32_t kQueue = kManager + 96u;
constexpr std::uint32_t kHead = 0x2000u;
constexpr std::uint32_t kEvent = 0x3000u;
constexpr std::uint32_t kEntry = 0x4000u;

struct Runtime final : pal4::ICombatActionRegistryRuntime {
    std::map<std::uint32_t, std::uint32_t> words{
        {kQueue + 4u, kHead}, {kQueue + 8u, 0u},
        {kHead, kHead}, {kHead + 4u, kHead}};
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> calls;
    std::uint32_t manager = kManager;
    std::uint32_t allocation = kEntry;

    std::uint32_t ActorManagerSingleton() override {
        calls.push_back("manager");
        return manager;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.push_back("read:" + std::to_string(address));
        return words.at(address);
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        calls.push_back("write:" + std::to_string(address));
        words[address] = value;
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        return bytes.at(address);
    }
    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override {
        bytes[address] = value;
    }
    std::uint32_t Allocate(std::uint32_t size) override {
        Check(size == 16u, "queue allocates 16-byte node");
        calls.push_back("allocate");
        return allocation;
    }
    std::uint32_t Free(std::uint32_t) override {
        throw std::runtime_error("unexpected free");
    }
    std::uint32_t EventIdentity(std::uint32_t) override {
        throw std::runtime_error("empty queue has no identity checks");
    }
    bool EventProcessed(std::uint32_t) override {
        throw std::runtime_error("empty queue has no processed checks");
    }
    void PrepareEvent(std::uint32_t event) override {
        Check(event == kEvent, "prepare given action");
        calls.push_back("prepare");
    }
    void SetProcessedFlag(std::uint32_t event_data) override {
        Check(event_data == kEvent + 4u, "processed flag offset");
        calls.push_back("set_processed");
    }
    void GetSkillSystem() override {
        throw std::runtime_error("unexpected skill system");
    }
    void RunNoOp() override {
        throw std::runtime_error("unexpected no-op");
    }
    void StartIdentityAction(std::uint32_t) override {
        throw std::runtime_error("unexpected identity action");
    }
    void UpdateContainer(std::uint32_t, float) override {
        throw std::runtime_error("unexpected update");
    }
    void StopTask(std::uint32_t) override {
        throw std::runtime_error("unexpected stop");
    }
    std::uint32_t ResultEvent(std::uint32_t) override {
        throw std::runtime_error("unexpected result event");
    }
    void DeleteTask(std::uint32_t event,
                    std::uint32_t flags) override {
        Check(event == kEvent && flags == 1u,
              "allocation failure deletes incoming action");
        calls.push_back("delete_task");
    }
};

}  // namespace

int main() {
    try {
        Runtime r;
        Check(pal4::CombatActionRegistry::RegisterActorEvent(kEvent, r) ==
                  kEvent &&
                  r.words[kQueue + 4u] == kHead &&
                  r.words[kQueue + 8u] == 1u &&
                  r.words[kHead] == kEntry &&
                  r.words[kHead + 4u] == kEntry &&
                  r.words[kEntry + 12u] == kEvent &&
                  r.bytes[kEntry + 8u] == 1u &&
                  r.calls.front() == "manager",
              "actor-manager queue at +96 links and counts event");

        r = Runtime{};
        Check(pal4::CombatActionRegistry::RegisterActorEvent(0u, r) == 0u &&
                  r.calls == std::vector<std::string>{"manager"},
              "null event leaves queue untouched");

        r = Runtime{};
        r.allocation = 0u;
        Check(pal4::CombatActionRegistry::RegisterActorEvent(kEvent, r) ==
                  0u && r.calls.back() == "delete_task" &&
                  r.words[kQueue + 8u] == 0u,
              "node allocation failure deletes incoming event");

        r = Runtime{};
        r.manager = 0u;
        Check(pal4::CombatActionRegistry::RegisterActorEvent(kEvent, r) ==
                  0u &&
                  r.calls == std::vector<std::string>{"manager"},
              "missing actor manager returns safely");
        std::cout << "Combat action registry tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
