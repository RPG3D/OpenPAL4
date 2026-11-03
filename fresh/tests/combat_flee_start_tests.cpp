#include "pal4/combat_flee_sequence_task.h"
#include "pal4/combat_miss_sequence_task.h"

#include <array>
#include <cmath>
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

constexpr std::uint32_t kTask = 0x1000u;
constexpr std::uint32_t kActor = 0x5000u;
constexpr std::uint32_t kPartyIterator = 0x6000u;
constexpr std::uint32_t kCandidateIterator = 0x6100u;

struct AddedTask {
    std::uint32_t child;
    std::uint32_t text;
    std::array<std::uint32_t, 3> timing;
};

struct Runtime final : pal4::IFleeSequenceRuntime {
    std::map<std::uint32_t, std::uint32_t> words{
        {kTask + 44u, kActor}, {kActor + 804u, 1u},
        {0x8f9620u, 0x9200u}, {0x8f96e4u, 0x9300u},
        {0x8f960cu, 0x9400u}};
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::map<std::uint32_t, std::int32_t> properties{
        {kActor, 20}, {0x5100u, 10}, {0x5200u, 30}};
    std::vector<std::uint32_t> party{0x5100u, 0x5200u};
    std::vector<std::uint32_t> candidates{0x5100u, 0x5200u};
    std::map<std::uint32_t, std::size_t> positions;
    std::vector<std::uint32_t> factories;
    std::vector<AddedTask> added;
    std::vector<std::string> calls;
    bool critical = true;
    bool flagged = false;
    bool combat_flag = false;
    bool ready_second = false;
    bool nan_average = false;
    bool throw_on_party_next = false;
    bool throw_on_candidate_next = false;
    std::size_t fail_child = 99u;
    std::uint32_t add_result = 0x7700u;
    std::uint32_t destroy_result = 0x8800u;

    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.push_back("read:" + std::to_string(address));
        return words.at(address);
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        calls.emplace_back("write_word");
        words[address] = value;
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        calls.emplace_back("write_byte");
        bytes[address] = value;
    }
    void NotifyActor(std::uint32_t actor) override {
        Check(actor == kActor, "Flee notify actor");
        calls.emplace_back("notify");
    }
    std::uint32_t CreateActorIterator(std::uint32_t actor,
                                      std::uint32_t offset) override {
        Check(actor == kActor, "Flee iterator actor");
        Check(offset == 472u || offset == 464u,
              "Flee actor iterator virtual offset");
        const auto iterator = offset == 472u ? kPartyIterator : kCandidateIterator;
        positions[iterator] = 0u;
        calls.push_back(offset == 472u ? "party_iterator" : "candidate_iterator");
        return iterator;
    }
    bool IteratorAtEnd(std::uint32_t iterator) override {
        return positions.at(iterator) >=
               (iterator == kPartyIterator ? party.size() : candidates.size());
    }
    std::uint32_t IteratorValue(std::uint32_t iterator) override {
        return (iterator == kPartyIterator ? party : candidates)
            .at(positions.at(iterator));
    }
    void IteratorNext(std::uint32_t iterator) override {
        if ((iterator == kPartyIterator && throw_on_party_next) ||
            (iterator == kCandidateIterator && throw_on_candidate_next))
            throw std::runtime_error("iterator next failed");
        ++positions.at(iterator);
    }
    std::uint32_t DestroyActorIterator(std::uint32_t iterator,
                                       std::uint32_t flags) override {
        Check(flags == 1u, "Flee iterator deleting destructor flag");
        calls.push_back(iterator == kPartyIterator ? "destroy_party" :
                                                       "destroy_candidate");
        return destroy_result;
    }
    std::int32_t ActorPropertySix(std::uint32_t actor) override {
        calls.push_back("property:" + std::to_string(actor));
        return properties.at(actor);
    }
    bool ActorFlag412(std::uint32_t actor) override {
        Check(actor != kActor, "Flee party flag belongs to member");
        return flagged && actor == 0x5100u;
    }
    bool CheckCriticalHit(float actor_value, float average) override {
        Check(actor_value == 20.0f, "Flee source property argument");
        nan_average = std::isnan(average);
        Check(nan_average || average == 20.0f,
              "Flee party property average");
        calls.emplace_back("critical");
        return critical;
    }
    bool CombatFlag501() override {
        calls.emplace_back("combat_flag");
        return combat_flag;
    }
    bool IsActorReady(std::uint32_t actor) override {
        calls.emplace_back("ready");
        return actor == 0x5100u || ready_second;
    }
    std::uint32_t CreateChildTask(std::uint32_t factory_vtable) override {
        factories.push_back(factory_vtable);
        calls.emplace_back("create");
        return factories.size() == fail_child ? 0u :
               0x2000u + static_cast<std::uint32_t>(factories.size()) * 0x100u;
    }
    std::uint32_t AddTaskAndInitialize(
        std::uint32_t container, std::uint32_t child,
        std::uint32_t text,
        const std::array<std::uint32_t, 3>& timing) override {
        Check(container == kTask + 4u, "Flee task container");
        added.push_back({child, text, timing});
        calls.emplace_back("add");
        return add_result;
    }
    void AssignText(std::uint32_t destination, std::uint32_t source,
                    std::uint32_t start, std::uint32_t count) override {
        Check(destination >= 0x2100u && source != 0u && start == 0u &&
                  count == 0xffffffffu,
              "Flee child text assignment");
        calls.push_back("text:" + std::to_string(source));
    }
};

std::size_t Count(const Runtime& runtime, const std::string& call) {
    std::size_t result = 0u;
    for (const auto& item : runtime.calls)
        if (item == call) ++result;
    return result;
}

void PartyChance() {
    Runtime runtime;
    Check(pal4::FleeSequenceTask::CheckPartyChance(kTask, runtime) &&
              Count(runtime, "destroy_party") == 1u &&
              Count(runtime, "combat_flag") == 1u,
          "party average permits flee and releases iterator");
    runtime = Runtime{};
    runtime.flagged = true;
    Check(!pal4::FleeSequenceTask::CheckPartyChance(kTask, runtime),
          "flagged party member blocks flee");
    runtime = Runtime{};
    runtime.combat_flag = true;
    Check(!pal4::FleeSequenceTask::CheckPartyChance(kTask, runtime),
          "combat flag blocks flee");
    runtime = Runtime{};
    runtime.critical = false;
    Check(!pal4::FleeSequenceTask::CheckPartyChance(kTask, runtime) &&
              Count(runtime, "combat_flag") == 0u,
          "failed critical check skips combat flag");
    runtime = Runtime{};
    runtime.party.clear();
    Check(pal4::FleeSequenceTask::CheckPartyChance(kTask, runtime) &&
              runtime.nan_average &&
              Count(runtime, "destroy_party") == 1u,
          "empty party preserves native NaN average");
    runtime = Runtime{};
    runtime.throw_on_party_next = true;
    bool caught = false;
    try { (void)pal4::FleeSequenceTask::CheckPartyChance(kTask, runtime); }
    catch (const std::runtime_error&) { caught = true; }
    Check(caught && Count(runtime, "destroy_party") == 1u,
          "party iterator releases on exception");
}

void DirectAndQuick() {
    Runtime runtime;
    runtime.words[kActor + 804u] = 0u;
    Check(pal4::FleeSequenceTask::Start(kTask, runtime) == 0x7700u &&
              runtime.factories == std::vector<std::uint32_t>{
                  pal4::FleeFinishChildFactory::kVtable} &&
              runtime.added.at(0).timing ==
                  std::array<std::uint32_t, 3>{1u, 0u, 0x3f800000u} &&
              runtime.bytes.at(0x2140u) == 1u &&
              runtime.words.at(0x213cu) == kActor,
          "direct Flee schedules one finish child with unit delay");
    runtime = Runtime{};
    runtime.critical = false;
    runtime.words[0x8f96e4u] = 0u;
    Check(pal4::QuickFleeSequenceTask::Start(kTask, runtime) == 0x7700u &&
              runtime.added.at(0).text == 0x841964u &&
              runtime.added.at(0).timing ==
                  std::array<std::uint32_t, 3>{1u, 0u, 0u} &&
              runtime.bytes.at(0x2140u) == 0u,
          "QuickFlee forwards failed chance and empty text");
}

void MessageAndCandidates() {
    Runtime runtime;
    runtime.critical = false;
    Check(pal4::FleeSequenceTask::Start(kTask, runtime) == 0x7700u &&
              runtime.factories == std::vector<std::uint32_t>{
                  pal4::MissMessageChildFactory::kVtable,
                  pal4::FleeFinishChildFactory::kVtable} &&
              runtime.added.at(0).text == 0x9400u &&
              runtime.added.at(1).text == 0x9300u &&
              runtime.words.at(0x213cu) == kActor &&
              runtime.bytes.at(0x2240u) == 0u &&
              Count(runtime, "text:9413272") == 1u,
          "failed active Flee schedules message then finish");
    runtime = Runtime{};
    runtime.add_result = 0u;
    Check(pal4::FleeSequenceTask::Start(kTask, runtime) == 0x8800u &&
              runtime.factories == std::vector<std::uint32_t>{
                  pal4::FleeCandidateChildFactory::kVtable,
                  pal4::FleeFinishChildFactory::kVtable} &&
              runtime.added.at(0).timing ==
                  std::array<std::uint32_t, 3>{1u, 0u, 0u} &&
              runtime.added.at(1).timing ==
                  std::array<std::uint32_t, 3>{1u, 0x40000000u, 0u} &&
              runtime.words.at(0x213cu) == 0x5100u &&
              runtime.bytes.at(0x2150u) == 0u &&
              runtime.bytes.at(0x2240u) == 1u &&
              Count(runtime, "ready") == 2u &&
              Count(runtime, "destroy_candidate") == 1u,
          "candidate branch filters readiness and returns iterator result even on zero add");
    runtime = Runtime{};
    runtime.ready_second = true;
    runtime.words[0x8f9620u] = 0u;
    Check(pal4::FleeSequenceTask::Start(kTask, runtime) == 0x8800u &&
              runtime.factories.size() == 3u &&
              runtime.added.at(0).text == 0x841964u &&
              runtime.added.at(1).text == 0x841964u,
          "all ready candidates use configured or fallback text");
}

void FailureAndFactorySlots() {
    pal4::FleeChildFactorySlot slot;
    Check(pal4::FleeCandidateChildFactory::Initialize(&slot) == &slot &&
              slot.vtable == 0x845d34u &&
              pal4::FleeFinishChildFactory::Initialize(&slot) == &slot &&
              slot.vtable == 0x845d38u,
          "Flee factory slots retain identity and exact vtables");
    Runtime runtime;
    runtime.fail_child = 2u;
    Check(pal4::FleeSequenceTask::Start(kTask, runtime) == 0u &&
              Count(runtime, "destroy_candidate") == 1u,
          "null finish child exits and releases candidate iterator");
    runtime = Runtime{};
    runtime.fail_child = 1u;
    Check(pal4::FleeSequenceTask::Start(kTask, runtime) == 0u &&
              Count(runtime, "destroy_candidate") == 1u,
          "null candidate child exits and releases iterator");
    runtime = Runtime{};
    runtime.throw_on_candidate_next = true;
    bool caught = false;
    try { (void)pal4::FleeSequenceTask::Start(kTask, runtime); }
    catch (const std::runtime_error&) { caught = true; }
    Check(caught && Count(runtime, "destroy_candidate") == 1u,
          "candidate iterator releases on exception");
}

}  // namespace

int main() {
    try {
        PartyChance();
        DirectAndQuick();
        MessageAndCandidates();
        FailureAndFactorySlots();
        std::cout << "Combat Flee start tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
