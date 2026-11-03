#include "pal4/actor_manager_monster_collection.h"

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

using pal4::ActorManagerMonsterCollection;
constexpr std::uint32_t manager = 0x1000u;
constexpr std::uint32_t actor = 0x8000u;

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IActorManagerMonsterCollectionRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> trace;
    std::uint32_t record = 0x6000u;
    std::uint32_t storage = 0x7000u;
    std::uint32_t constructed = actor;
    std::uint32_t next_router = 0x9000u;
    std::uint32_t property = 7u;
    std::uint32_t array_allocation = 0x9000u;
    std::string assigned_name;

    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
        trace.emplace_back("write:" + std::to_string(address) + ":" +
                           std::to_string(value));
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        bytes[address] = value;
        trace.emplace_back("byte:" + std::to_string(address) + ":" +
                           std::to_string(value));
    }
    std::uint32_t Free(std::uint32_t address) override {
        trace.emplace_back("free:" + std::to_string(address));
        return address + 1u;
    }
    std::uint32_t AllocateArray(std::uint32_t bytes) override {
        trace.emplace_back("allocate-array:" + std::to_string(bytes));
        return array_allocation;
    }
    std::uint32_t DialogData(std::uint32_t parameter) override {
        Check(parameter == 0x777u, "dialog event parameter");
        trace.emplace_back("dialog-data");
        return 0xaaaau;
    }
    std::uint32_t EventRouter() override {
        trace.emplace_back("router");
        return next_router++;
    }
    void RegisterEvent(std::uint32_t router, std::uint32_t mode,
                       std::string_view name, std::uint32_t owner,
                       std::uint32_t callback) override {
        Check(mode == 2u && owner == manager,
              "registration mode and owner");
        if (name == "me_join_stage")
            Check(router == 0x9000u && callback == 0x533950u,
                  "join callback");
        else if (name == "me_leave_stage")
            Check(router == 0x9001u && callback == 0x53b580u,
                  "leave callback");
        else
            Check(name == "combat_over" && router == 0x9002u &&
                      callback == 0x57b8d0u, "combat-over callback");
        trace.emplace_back("register:" + std::string(name));
    }
    std::uint32_t FindMonsterRecord(std::uint32_t key) override {
        Check(key == 0x777u, "database key");
        trace.emplace_back("find");
        return record;
    }
    void HandleMissingMonster(std::uint32_t key) override {
        Check(key == 0x777u, "missing key");
        trace.emplace_back("missing");
    }
    std::uint32_t AllocateActor(std::uint32_t bytes) override {
        Check(bytes == 0x490u, "actor allocation size");
        trace.emplace_back("allocate");
        return storage;
    }
    std::uint32_t ConstructActor(std::uint32_t address,
                                 std::uint32_t source) override {
        Check(address == storage && source == record,
              "actor construction arguments");
        trace.emplace_back("construct");
        return constructed;
    }
    std::string_view ActorBaseName(std::uint32_t address) override {
        Check(address == actor, "actor base name receiver");
        trace.emplace_back("base-name");
        return "Goblin";
    }
    void AssignActorName(std::uint32_t address,
                         std::string_view name) override {
        Check(address == actor, "actor name receiver");
        assigned_name = name;
        trace.emplace_back("assign-name");
    }
    void ActorSlot24(std::uint32_t address,
                     std::uint32_t argument) override {
        Check(address == actor && argument == 0x123u,
              "slot 24 receives requested state");
        trace.emplace_back("slot24");
    }
    void DispatchNewActor(std::uint32_t address) override {
        Check(address == actor, "new actor event payload");
        trace.emplace_back("me_new_added");
    }
    void ActorEvent268(std::uint32_t address,
                       std::uint32_t event,
                       std::uint32_t payload) override {
        Check(address == actor && event == 17020u && payload == actor,
              "group-one event payload");
        trace.emplace_back("event17020");
    }
    std::uint32_t CreateIterator(std::uint32_t list,
                                 std::uint32_t begin) override {
        Check((list == manager && begin == 0x4000u) ||
                  (list == manager + 16u && begin == 0x5000u),
              "iterator list owner and start");
        trace.emplace_back("iterator:" + std::to_string(list));
        return list + 0x9000u;
    }
    std::uint32_t ActorProperty4(std::uint32_t address) override {
        trace.emplace_back("property:" + std::to_string(address));
        return words[address + 4u];
    }
    void ActorSlot388(std::uint32_t address) override {
        trace.emplace_back("slot388:" + std::to_string(address));
    }
    void LogRelease(std::uint32_t address,
                    std::uint32_t name) override {
        Check(name == 0xaaaau || name ==
                  ActorManagerMonsterCollection::kEmptyName,
              "release name or empty fallback");
        trace.emplace_back("log:" + std::to_string(address));
    }
    void ActorSlot248(std::uint32_t address,
                      std::uint32_t flags) override {
        Check(flags == 1u, "actor deleting destructor flag");
        trace.emplace_back("slot248:" + std::to_string(address));
    }
};

void ConstructionAndRegistration() {
    Runtime runtime;
    Check(ActorManagerMonsterCollection::Construct(manager, runtime) ==
              manager &&
              runtime.trace == std::vector<std::string>{
                  "byte:4096:0", "write:4100:0", "write:4104:0",
                  "write:4108:0", "byte:4112:0", "write:4116:0",
                  "write:4120:0", "write:4124:0", "router",
                  "register:me_join_stage", "router",
                  "register:me_leave_stage", "router",
                  "register:combat_over"},
          "construct two empty ranges then register three callbacks");
}

void MonsterCreation() {
    Runtime missing;
    missing.record = 0u;
    Check(ActorManagerMonsterCollection::CreateMonster(
              manager, 0x777u, 0x123u, 1u, missing) == 0u &&
              missing.trace == std::vector<std::string>{"find", "missing"},
          "database miss stops before allocation");

    Runtime no_memory;
    no_memory.storage = 0u;
    Check(ActorManagerMonsterCollection::CreateMonster(
              manager, 0x777u, 0x123u, 1u, no_memory) == 0u &&
              no_memory.trace ==
                  std::vector<std::string>{"find", "allocate"},
          "allocation failure remains defined");

    Runtime success;
    success.words[manager + 4u] = 0x4000u;
    success.words[manager + 8u] = 0x4008u;
    success.words[manager + 12u] = 0x400cu;
    Check(ActorManagerMonsterCollection::CreateMonster(
              manager, 0x777u, 0x123u, 1u, success) == actor &&
              success.assigned_name == "2_Goblin" &&
              success.words[0x4008u] == actor &&
              success.words[manager + 8u] == 0x400cu &&
              success.trace == std::vector<std::string>{
                  "find", "allocate", "construct", "base-name",
                  "assign-name", "slot24", "write:16392:32768",
                  "write:4104:16396", "me_new_added",
                  "event17020"},
          "actor name, state, insertion and group-one event order");

    Runtime ordinary;
    Check(ActorManagerMonsterCollection::CreateMonster(
              manager, 0x777u, 0x123u, 2u, ordinary) == actor &&
              ordinary.assigned_name == "0_Goblin" &&
              ordinary.words[manager + 4u] == 0x9000u &&
              ordinary.words[manager + 8u] == 0x9004u &&
              ordinary.trace.back() == "me_new_added",
          "empty collection starts at index zero and skips group event");

    Runtime failed_insert;
    failed_insert.array_allocation = 0u;
    Check(ActorManagerMonsterCollection::CreateMonster(
              manager, 0x777u, 0x123u, 2u, failed_insert) == 0u &&
              failed_insert.trace == std::vector<std::string>{
                  "find", "allocate", "construct", "base-name",
                  "assign-name", "slot24", "allocate-array:4",
                  "log:32768", "slot248:32768"},
          "failed pointer insertion releases constructed actor");
}

void RepeatedPointerInsertion() {
    Runtime empty;
    empty.words[manager + 4u] = 0x4000u;
    empty.words[manager + 8u] = 0x4008u;
    empty.words[manager + 12u] = 0x4008u;
    Check(ActorManagerMonsterCollection::InsertCopies(
              manager, 0x4004u, 0u, 0x6000u, empty) == 0x4008u &&
              empty.trace.empty(),
          "zero count preserves array and returns its end");

    Runtime middle;
    middle.words[manager + 4u] = 0x4000u;
    middle.words[manager + 8u] = 0x400cu;
    middle.words[manager + 12u] = 0x4018u;
    middle.words[0x4000u] = 1u;
    middle.words[0x4004u] = 2u;
    middle.words[0x4008u] = 3u;
    middle.words[0x6000u] = 9u;
    Check(ActorManagerMonsterCollection::InsertCopies(
              manager, 0x4004u, 2u, 0x6000u, middle) == 0x4014u &&
              middle.words[0x4000u] == 1u &&
              middle.words[0x4004u] == 9u &&
              middle.words[0x4008u] == 9u &&
              middle.words[0x400cu] == 2u &&
              middle.words[0x4010u] == 3u &&
              middle.words[manager + 8u] == 0x4014u &&
              middle.words[manager + 12u] == 0x4018u,
          "in-place insertion moves a tail at least as long as count");

    Runtime short_tail;
    short_tail.words[manager + 4u] = 0x4000u;
    short_tail.words[manager + 8u] = 0x400cu;
    short_tail.words[manager + 12u] = 0x401cu;
    short_tail.words[0x4000u] = 1u;
    short_tail.words[0x4004u] = 2u;
    short_tail.words[0x4008u] = 3u;
    short_tail.words[0x6000u] = 9u;
    Check(ActorManagerMonsterCollection::InsertCopies(
              manager, 0x4008u, 3u, 0x6000u, short_tail) == 0x4018u &&
              short_tail.words[0x4008u] == 9u &&
              short_tail.words[0x400cu] == 9u &&
              short_tail.words[0x4010u] == 9u &&
              short_tail.words[0x4014u] == 3u,
          "in-place insertion preserves a tail shorter than count");

    Runtime growth;
    growth.words[manager + 4u] = 0x4000u;
    growth.words[manager + 8u] = 0x400cu;
    growth.words[manager + 12u] = 0x400cu;
    growth.words[0x4000u] = 1u;
    growth.words[0x4004u] = 2u;
    growth.words[0x4008u] = 3u;
    growth.words[0x6000u] = 9u;
    Check(ActorManagerMonsterCollection::InsertCopies(
              manager, 0x4004u, 2u, 0x6000u, growth) == 0x9014u &&
              growth.words[manager + 4u] == 0x9000u &&
              growth.words[manager + 8u] == 0x9014u &&
              growth.words[manager + 12u] == 0x9018u &&
              growth.words[0x9000u] == 1u &&
              growth.words[0x9004u] == 9u &&
              growth.words[0x9008u] == 9u &&
              growth.words[0x900cu] == 2u &&
              growth.words[0x9010u] == 3u &&
              growth.trace.front() == "allocate-array:24" &&
              std::find(growth.trace.begin(), growth.trace.end(),
                        "free:16384") != growth.trace.end(),
          "growth doubles three slots and preserves both sides");

    Runtime failure;
    failure.array_allocation = 0u;
    failure.words[manager + 4u] = 0x4000u;
    failure.words[manager + 8u] = 0x4004u;
    failure.words[manager + 12u] = 0x4004u;
    failure.words[0x4000u] = 1u;
    Check(ActorManagerMonsterCollection::InsertCopies(
              manager, 0x4000u, 2u, 0x6000u, failure) == 0u &&
              failure.words[manager + 4u] == 0x4000u &&
              failure.words[manager + 8u] == 0x4004u &&
              failure.words[0x4000u] == 1u &&
              failure.trace == std::vector<std::string>{"allocate-array:12"},
          "allocation failure preserves the original range");
}

void IteratorsAndAverage() {
    Runtime runtime;
    runtime.words[manager + 4u] = 0x4000u;
    runtime.words[manager + 20u] = 0x5000u;
    Check(ActorManagerMonsterCollection::FirstIterator(
              manager, 0x6000u, runtime) == 0x6000u &&
              runtime.words[0x6000u] == manager + 0x9000u &&
              ActorManagerMonsterCollection::SecondIterator(
                  manager, 0x6004u, runtime) == 0x6004u &&
              runtime.words[0x6004u] == manager + 16u + 0x9000u &&
              runtime.trace == std::vector<std::string>{
                  "iterator:4096", "write:24576:40960",
                  "iterator:4112", "write:24580:40976"},
          "iterator factories bind correct list and output");
    runtime.trace.clear();
    runtime.words[manager + 8u] = 0x400cu;
    runtime.words[0x4000u] = 0x8000u;
    runtime.words[0x4004u] = 0x8100u;
    runtime.words[0x4008u] = 0x8200u;
    runtime.words[0x8004u] = 4u;
    runtime.words[0x8104u] = 8u;
    runtime.words[0x8204u] = 9u;
    Check(ActorManagerMonsterCollection::AverageProperty4(manager, runtime) ==
              7u &&
              runtime.trace == std::vector<std::string>{
                  "property:32768", "property:33024", "property:33280"},
          "average uses actor slot 52 index 4 and integer division");
    runtime.words[manager + 8u] = runtime.words[manager + 4u];
    try {
        (void)ActorManagerMonsterCollection::AverageProperty4(manager, runtime);
        Check(false, "empty average must fail");
    } catch (const std::domain_error&) {
    }
}

void ClearingAndDestruction() {
    Runtime runtime;
    runtime.words[manager + 4u] = 0x4000u;
    runtime.words[manager + 8u] = 0x4008u;
    runtime.words[0x4000u] = 0x8000u;
    runtime.words[0x4004u] = 0x8100u;
    runtime.words[0x8000u + 816u] = 0xaaaau;
    runtime.words[manager + 20u] = 0x5000u;
    runtime.words[manager + 24u] = 0x5004u;
    Check(ActorManagerMonsterCollection::Clear(manager, runtime) ==
              0x5004u &&
              runtime.words[manager + 8u] == 0x4000u &&
              runtime.words[manager + 24u] == 0x5000u &&
              runtime.trace == std::vector<std::string>{
                  "slot388:32768", "slot388:33024", "log:32768",
                  "slot248:32768", "log:33024", "slot248:33024",
                  "write:4104:16384", "write:4120:20480"},
          "clear first resets all actors then releases all and rewinds ranges");
    runtime.trace.clear();
    Check(ActorManagerMonsterCollection::Destroy(manager, runtime) ==
              0x4001u &&
              runtime.trace == std::vector<std::string>{
                  "write:4104:16384", "write:4120:20480", "free:20480",
                  "write:4116:0", "write:4120:0", "write:4124:0",
                  "free:16384", "write:4100:0", "write:4104:0",
                  "write:4108:0"},
          "destroy clears, frees second then first buffer and forwards free result");
    runtime.trace.clear();
    ActorManagerMonsterCollection::ReleaseMonster(0u, runtime);
    Check(runtime.trace.empty(), "null release is a defined no-op");
}

void JoinStageArray() {
    Runtime spare;
    spare.words[manager + 20u] = 0x5000u;
    spare.words[manager + 24u] = 0x5004u;
    spare.words[manager + 28u] = 0x5008u;
    Check(ActorManagerMonsterCollection::OnJoinStage(
              manager, 0x777u, spare) == 0x5008u &&
              spare.words[0x5004u] == 0xaaaau &&
              spare.words[manager + 24u] == 0x5008u &&
              spare.trace == std::vector<std::string>{
                  "dialog-data", "write:20484:43690", "write:4120:20488"},
          "join stage appends in spare slot");

    Runtime growth;
    growth.words[manager + 20u] = 0x5000u;
    growth.words[manager + 24u] = 0x5008u;
    growth.words[manager + 28u] = 0x5008u;
    growth.words[0x5000u] = 0x111u;
    growth.words[0x5004u] = 0x222u;
    Check(ActorManagerMonsterCollection::OnJoinStage(
              manager, 0x777u, growth) == 0x900cu &&
              growth.words[manager + 20u] == 0x9000u &&
              growth.words[manager + 24u] == 0x900cu &&
              growth.words[manager + 28u] == 0x9010u &&
              growth.words[0x9000u] == 0x111u &&
              growth.words[0x9004u] == 0x222u &&
              growth.words[0x9008u] == 0xaaaau &&
              growth.trace == std::vector<std::string>{
                  "dialog-data", "allocate-array:16", "write:36864:273",
                  "write:36868:546", "write:36872:43690", "free:20480",
                  "write:4116:36864", "write:4120:36876",
                  "write:4124:36880"},
          "join stage grows from two to four slots and moves existing values");

    Runtime failed;
    failed.array_allocation = 0u;
    Check(ActorManagerMonsterCollection::OnJoinStage(
              manager, 0x777u, failed) == 0u &&
              failed.trace == std::vector<std::string>{
                  "dialog-data", "allocate-array:4"},
          "failed first allocation leaves both range fields unchanged");
}

}  // namespace

int main() {
    try {
        ConstructionAndRegistration();
        MonsterCreation();
        RepeatedPointerInsertion();
        IteratorsAndAverage();
        ClearingAndDestruction();
        JoinStageArray();
        std::cout << "actor_manager_monster_collection_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
