#include "pal4/actor_manager_player_stat_refresh.h"

#include <algorithm>
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

constexpr std::uint32_t actor = 0x1000u;
constexpr std::uint32_t data = actor + 340u;
constexpr std::uint32_t temporary = 0x7000u;

struct Runtime final : pal4::IActorManagerPlayerStatRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> trace;
    std::vector<std::uint32_t> deleted;
    std::vector<std::uint32_t> freed;
    std::vector<std::int32_t> max0_values{10};
    std::vector<std::int32_t> max2_values{20};
    std::size_t max0_calls = 0u;
    std::size_t max2_calls = 0u;
    std::int32_t value7 = 5;
    std::int32_t value9 = 6;
    std::int32_t value8 = 7;
    std::int32_t property9 = 6;
    std::uint32_t storage = 0x6000u;
    std::uint32_t damage = 0x6000u;
    std::int32_t health_result = 0x1234;
    bool throw_construct = false;
    bool throw_move = false;

    std::uint32_t ReadWord(std::uint32_t address) override {
        trace.emplace_back("read:" + std::to_string(address));
        if (throw_move && address == actor + 800u)
            throw std::runtime_error("move failed");
        return words[address];
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        trace.emplace_back("word:" + std::to_string(address));
        words[address] = value;
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        trace.emplace_back("read-byte:" + std::to_string(address));
        return bytes[address];
    }
    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override {
        trace.emplace_back("byte:" + std::to_string(address));
        bytes[address] = value;
    }
    void DeleteActor(std::uint32_t object,
                     std::uint32_t flags) override {
        Check(flags == 1u, "damage deletion flag");
        trace.emplace_back("delete:" + std::to_string(object));
        deleted.push_back(object);
    }
    void Free(std::uint32_t address) override {
        trace.emplace_back("free:" + std::to_string(address));
        freed.push_back(address);
    }
    std::uint32_t Allocate(std::uint32_t bytes_requested) override {
        Check(bytes_requested == 12u, "12-byte damage allocation");
        trace.emplace_back("allocate");
        return storage;
    }
    std::uint32_t ConstructDamage(std::uint32_t actual_storage) override {
        Check(actual_storage == storage,
              "damage constructor receives allocation");
        trace.emplace_back("construct");
        if (throw_construct) throw std::runtime_error("construct failed");
        return damage;
    }
    std::uint32_t TemporarySlotAddress() override {
        trace.emplace_back("temporary");
        return temporary;
    }
    std::int32_t ActorSlot52(std::uint32_t actual_actor,
                             std::uint32_t mode) override {
        Check(actual_actor == actor, "player stat actor address");
        trace.emplace_back("slot:" + std::to_string(mode));
        if (mode == 7u) return value7;
        if (mode == 9u) return value9;
        Check(mode == 8u, "player stat virtual mode");
        return value8;
    }
    void CopyBytes(std::uint32_t destination,
                   std::uint32_t from,
                   std::uint32_t count) override {
        Check(destination == data && from == actor + 4u &&
                  count == 0x150u,
              "rebuild copies 336-byte actor base block");
        trace.emplace_back("copy");
    }
    void ProcessInventoryItemSlots(std::uint32_t slots,
                                   std::uint32_t destination,
                                   std::uint32_t from) override {
        Check(slots == actor + 896u && destination == data &&
                  from == actor + 4u,
              "inventory slots process copied and source data");
        trace.emplace_back("inventory-slots");
    }
    void ProcessSkills(std::uint32_t actual_actor,
                       std::uint32_t mode,
                       std::uint32_t context) override {
        Check(actual_actor == actor && mode == 1u && context == actor,
              "skills batch arguments");
        trace.emplace_back("skills");
    }
    std::int32_t DataElement(std::uint32_t actual_data,
                             std::uint32_t index) override {
        Check(actual_data == data && (index == 0u || index == 2u),
              "actor data element lookup");
        trace.emplace_back("element:" + std::to_string(index));
        if (index == 0u) {
            const auto position = std::min(max0_calls++,
                                           max0_values.size() - 1u);
            return max0_values[position];
        }
        const auto position = std::min(max2_calls++,
                                       max2_values.size() - 1u);
        return max2_values[position];
    }
    void SetValidatedProperty(std::uint32_t actual_data,
                              std::uint32_t index,
                              std::uint32_t value) override {
        Check(actual_data == data &&
                  (index == 7u || index == 8u || index == 9u),
              "validated property target");
        trace.emplace_back("set:" + std::to_string(index) + ":" +
                           std::to_string(value));
    }
    std::int32_t GetProperty(std::uint32_t actual_data,
                             std::uint32_t index) override {
        Check(actual_data == data && index == 9u,
              "property nine query");
        trace.emplace_back("get:9");
        return property9;
    }
    std::int32_t UpdateHealthStatus(std::uint32_t actual_actor) override {
        Check(actual_actor == actor, "final health status actor");
        trace.emplace_back("health");
        return health_result;
    }
};

std::size_t Position(const std::vector<std::string>& trace,
                     const std::string& event) {
    const auto it = std::find(trace.begin(), trace.end(), event);
    Check(it != trace.end(), "expected stat operation missing");
    return static_cast<std::size_t>(it - trace.begin());
}

void DamageOwnershipAndOrder() {
    Runtime runtime;
    runtime.bytes[actor + 796u] = 1u;
    runtime.words[actor + 800u] = 0x5000u;
    Check(pal4::ActorManagerPlayerStatRefresh::Rebuild(actor, runtime) ==
              runtime.health_result &&
              runtime.deleted == std::vector<std::uint32_t>{0x5000u} &&
              runtime.words.at(actor + 800u) == runtime.damage &&
              runtime.bytes.at(actor + 796u) == 1u &&
              runtime.bytes.at(temporary) == 0u,
          "old owned damage is replaced by new damage without double delete");
    const auto& trace = runtime.trace;
    Check(trace.front() == "allocate" &&
              Position(trace, "allocate") < Position(trace, "construct") &&
              Position(trace, "construct") <
                  Position(trace, "delete:20480") &&
              Position(trace, "delete:20480") <
                  Position(trace, "slot:7") &&
              Position(trace, "slot:7") <
                  Position(trace, "slot:9") &&
              Position(trace, "slot:9") <
                  Position(trace, "slot:8") &&
              Position(trace, "slot:8") < Position(trace, "copy") &&
              Position(trace, "copy") <
                  Position(trace, "inventory-slots") &&
              Position(trace, "inventory-slots") <
                  Position(trace, "skills") &&
              trace.back() == "health",
          "damage, snapshots, copy, inventory, skills and health order");

    Runtime same;
    same.words[actor + 800u] = same.damage;
    Check(pal4::ActorManagerPlayerStatRefresh::Rebuild(actor, same) ==
              same.health_result && same.deleted.empty() &&
              same.bytes.at(actor + 796u) == 1u,
          "same damage pointer sets owner without deleting it");

    Runtime no_storage;
    no_storage.storage = 0u;
    no_storage.words[actor + 800u] = 0x5000u;
    no_storage.bytes[actor + 796u] = 1u;
    Check(pal4::ActorManagerPlayerStatRefresh::Rebuild(actor, no_storage) ==
              no_storage.health_result &&
              no_storage.deleted == std::vector<std::uint32_t>{0x5000u} &&
              no_storage.words.at(actor + 800u) == 0u &&
              no_storage.bytes.at(actor + 796u) == 0u &&
              std::find(no_storage.trace.begin(), no_storage.trace.end(),
                        "construct") == no_storage.trace.end(),
          "allocation failure clears owned slot without constructing damage");
}

void StatsClampAndCorrection() {
    Runtime runtime;
    runtime.value7 = 15;
    runtime.value9 = 25;
    runtime.value8 = 13;
    runtime.max0_values = {10, 10, 9, 9, 8};
    runtime.max2_values = {20, 20, 19};
    runtime.property9 = 30;
    (void)pal4::ActorManagerPlayerStatRefresh::Rebuild(actor, runtime);
    Check(runtime.words.at(actor + 572u) == 10u &&
              runtime.words.at(actor + 580u) == 20u &&
              runtime.max0_calls == 5u &&
              runtime.max2_calls == 3u,
          "initial values clamp to maxima and re-query later corrections");
    Check(Position(runtime.trace, "set:8:13") <
              Position(runtime.trace, "set:7:9") &&
              Position(runtime.trace, "set:7:9") <
                  Position(runtime.trace, "set:9:8"),
          "validated property eight precedes seven and nine corrections");

    Runtime negative;
    negative.value7 = -3;
    negative.value9 = -4;
    negative.value8 = -5;
    (void)pal4::ActorManagerPlayerStatRefresh::Rebuild(actor, negative);
    Check(negative.words.at(actor + 572u) == 0u &&
              negative.words.at(actor + 580u) == 0u &&
              Position(negative.trace, "set:8:4294967291") <
                  Position(negative.trace, "health"),
          "negative mode seven/nine values clamp to zero; eight stays signed");

    Runtime equal;
    equal.value7 = 10;
    equal.value9 = 20;
    equal.property9 = 20;
    (void)pal4::ActorManagerPlayerStatRefresh::Rebuild(actor, equal);
    Check(equal.words.at(actor + 572u) == 10u &&
              equal.words.at(actor + 580u) == 20u &&
              std::find(equal.trace.begin(), equal.trace.end(),
                        "set:7:10") == equal.trace.end() &&
              std::find(equal.trace.begin(), equal.trace.end(),
                        "set:9:10") == equal.trace.end(),
          "equal maxima do not cause secondary correction");
}

void ExceptionCleanup() {
    Runtime construction;
    construction.throw_construct = true;
    try {
        (void)pal4::ActorManagerPlayerStatRefresh::Rebuild(
            actor, construction);
        throw std::runtime_error("construction should throw");
    } catch (const std::runtime_error& error) {
        Check(std::string(error.what()) == "construct failed" &&
                  construction.freed ==
                      std::vector<std::uint32_t>{construction.storage},
              "construction exception frees raw damage allocation");
    }

    Runtime moving;
    moving.throw_move = true;
    try {
        (void)pal4::ActorManagerPlayerStatRefresh::Rebuild(actor, moving);
        throw std::runtime_error("move should throw");
    } catch (const std::runtime_error& error) {
        Check(std::string(error.what()) == "move failed" &&
                  moving.deleted ==
                      std::vector<std::uint32_t>{moving.damage} &&
                  std::find(moving.trace.begin(), moving.trace.end(),
                            "slot:7") == moving.trace.end(),
              "move exception destroys temporary damage before stat update");
    }
}

}  // namespace

int main() {
    try {
        DamageOwnershipAndOrder();
        StatsClampAndCorrection();
        ExceptionCleanup();
        std::cout << "actor_manager_player_stat_refresh_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
