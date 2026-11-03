#include "pal4/actor_manager_player_combat_actor.h"

#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

using pal4::ActorManagerPlayerCombatActor;
constexpr std::uint32_t actor = 0x1000u;
constexpr std::uint32_t source = 0x8123u;

struct Event {
    std::string name;
    std::uint32_t a = 0u;
    std::uint32_t b = 0u;
    std::uint32_t c = 0u;
    bool operator==(const Event&) const = default;
};

void Check(const bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IActorManagerPlayerCombatActorRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<Event> trace;
    std::uint32_t damage_storage = 0x7000u;
    std::uint32_t facet_storage = 0x8000u;

    std::uint32_t Allocate(const std::uint32_t size) override {
        trace.push_back({"allocate", size});
        return size == 12u ? damage_storage : facet_storage;
    }
    void Free(const std::uint32_t address) override {
        trace.push_back({"free", address});
    }
    std::uint32_t ReadWord(const std::uint32_t address) override {
        return words[address];
    }
    std::uint8_t ReadByte(const std::uint32_t address) override {
        return bytes[address];
    }
    void WriteWord(const std::uint32_t address,
                   const std::uint32_t value) override {
        words[address] = value;
        trace.push_back({"word", address, value});
    }
    void WriteByte(const std::uint32_t address,
                   const std::uint8_t value) override {
        bytes[address] = value;
        trace.push_back({"byte", address, value});
    }
    std::uint32_t ConstructDamage(const std::uint32_t storage) override {
        trace.push_back({"damage", storage});
        return storage + 0x100u;
    }
    void ConstructActorData(const std::uint32_t address,
                            const std::uint32_t mode,
                            const std::uint32_t damage) override {
        trace.push_back({"actor-data", address, mode, damage});
    }
    void ConstructTextArray(const std::uint32_t address,
                            const std::uint32_t count) override {
        trace.push_back({"construct-text", address, count, 16u});
    }
    void ConstructEmbeddedArrays(const std::uint32_t address) override {
        trace.push_back({"embedded-arrays", address});
    }
    void CopyActorData(const std::uint32_t address,
                       const std::uint32_t original) override {
        trace.push_back({"copy-data", address, original});
    }
    std::uint32_t ConstructFacet(const std::uint32_t storage,
                                 const std::uint32_t address) override {
        trace.push_back({"facet", storage, address});
        return storage + 4u;
    }
    void CleanupProperty(const std::uint32_t address,
                         const std::uint32_t context) override {
        trace.push_back({"cleanup-property", address, context});
    }
    void DestroyFacet(const std::uint32_t facet) override {
        trace.push_back({"destroy-facet", facet});
    }
    void DeleteChild(const std::uint32_t child,
                     const std::uint32_t flags) override {
        trace.push_back({"delete-child", child, flags});
    }
    void DestroyTextArray(const std::uint32_t address,
                          const std::uint32_t count) override {
        trace.push_back({"destroy-text", address, count, 16u});
    }
    void DestroyActorData(const std::uint32_t address,
                          const std::uint32_t context) override {
        trace.push_back({"destroy-data", address, context});
    }
};

void Construction() {
    Runtime runtime;
    Check(ActorManagerPlayerCombatActor::Construct(actor, source, runtime) ==
              actor &&
              runtime.trace == std::vector<Event>{
                  {"allocate", 12u}, {"damage", 0x7000u},
                  {"actor-data", actor, 1u, 0x7100u},
                  {"word", actor + 896u}, {"word", actor + 900u},
                  {"word", actor + 904u}, {"word", actor + 908u},
                  {"word", actor + 912u}, {"word", actor + 916u},
                  {"construct-text", actor + 920u, 5u, 16u},
                  {"construct-text", actor + 1000u, 5u, 16u},
                  {"byte", actor + 1080u, 0x23u},
                  {"word", actor + 1084u}, {"word", actor + 1088u},
                  {"word", actor + 1092u},
                  {"byte", actor + 1096u, 0x23u},
                  {"word", actor + 1100u}, {"word", actor + 1104u},
                  {"word", actor + 1108u},
                  {"embedded-arrays", actor + 1112u},
                  {"word", actor + 1296u},
                  {"byte", actor + 1292u, 0x23u},
                  {"word", actor + 1300u}, {"word", actor + 1304u},
                  {"byte", actor + 1312u}, {"word", actor + 1316u},
                  {"word", actor, ActorManagerPlayerCombatActor::kVtable},
                  {"word", actor + 716u,
                   ActorManagerPlayerCombatActor::kFacetVtable},
                  {"word", actor + 724u,
                   ActorManagerPlayerCombatActor::kPropertyVtable},
                  {"copy-data", actor, source},
                  {"allocate", 8u}, {"facet", 0x8000u, actor},
                  {"word", actor + 1288u, 0x8004u},
                  {"byte", actor + 1308u}},
          "player actor constructor preserves native field and call order");
    Check(runtime.words[actor + 1296u] == 0u &&
              runtime.bytes[actor + 1312u] == 0u &&
              runtime.words[actor + 1288u] == 0x8004u &&
              ActorManagerPlayerCombatActor::kObjectSize == 0x528u,
          "player actor final layout and object size");

    Runtime no_damage;
    no_damage.damage_storage = 0u;
    (void)ActorManagerPlayerCombatActor::Construct(actor, 0x100u, no_damage);
    Check(no_damage.trace[0] == Event{"allocate", 12u} &&
              no_damage.trace[1] == Event{"actor-data", actor, 1u, 0u} &&
              no_damage.bytes[actor + 1080u] == 0u &&
              no_damage.bytes[actor + 1096u] == 0u &&
              no_damage.bytes[actor + 1292u] == 0u,
          "null damage allocation and source low-byte marker");

    Runtime no_facet;
    no_facet.facet_storage = 0u;
    Check(ActorManagerPlayerCombatActor::InitializeFacet(
              actor, no_facet) == 0u &&
              no_facet.trace == std::vector<Event>{
                  {"allocate", 8u}, {"word", actor + 1288u},
                  {"byte", actor + 1308u}},
          "facet allocation failure clears pointer and flag");

    Runtime direct_facet;
    Check(ActorManagerPlayerCombatActor::InitializeFacet(
              actor, direct_facet) == 0x8004u &&
              direct_facet.trace == std::vector<Event>{
                  {"allocate", 8u}, {"facet", 0x8000u, actor},
                  {"word", actor + 1288u, 0x8004u},
                  {"byte", actor + 1308u}},
          "direct facet initialization returns constructed object");
}

void Getters() {
    Runtime runtime;
    runtime.words[actor + 1196u] = 11u;
    runtime.words[actor + 1200u] = 22u;
    runtime.words[actor + 1204u] = 33u;
    Check(ActorManagerPlayerCombatActor::Value1196(actor, runtime) == 11u &&
              ActorManagerPlayerCombatActor::Value1200(actor, runtime) == 22u &&
              ActorManagerPlayerCombatActor::Value1204(actor, runtime) == 33u,
          "three getters use distinct word offsets");
}

void Destruction() {
    Runtime runtime;
    runtime.words[actor + 1288u] = 0x7100u;
    runtime.bytes[actor + 1312u] = 1u;
    runtime.words[actor + 1316u] = 0x7200u;
    runtime.words[actor + 1296u] = 0x7300u;
    runtime.words[actor + 1100u] = 0x7400u;
    runtime.words[actor + 1084u] = 0x7500u;
    Check(ActorManagerPlayerCombatActor::Delete(
              actor, 1u, 0x9000u, runtime) == actor &&
              runtime.trace == std::vector<Event>{
                  {"word", actor, ActorManagerPlayerCombatActor::kVtable},
                  {"word", actor + 716u,
                   ActorManagerPlayerCombatActor::kFacetVtable},
                  {"word", actor + 724u,
                   ActorManagerPlayerCombatActor::kPropertyVtable},
                  {"cleanup-property", actor, 0x9000u},
                  {"destroy-facet", 0x7100u}, {"free", 0x7100u},
                  {"delete-child", 0x7200u, 1u},
                  {"free", 0x7300u},
                  {"word", actor + 1296u}, {"word", actor + 1300u},
                  {"word", actor + 1304u},
                  {"destroy-text", actor + 1208u, 5u, 16u},
                  {"destroy-text", actor + 1112u, 4u, 16u},
                  {"free", 0x7400u},
                  {"word", actor + 1100u}, {"word", actor + 1104u},
                  {"word", actor + 1108u},
                  {"free", 0x7500u},
                  {"word", actor + 1084u}, {"word", actor + 1088u},
                  {"word", actor + 1092u},
                  {"destroy-text", actor + 1000u, 5u, 16u},
                  {"destroy-text", actor + 920u, 5u, 16u},
                  {"destroy-data", actor, 0x9000u},
                  {"free", actor}},
          "deleting destructor releases owned fields in native order");
    for (const auto offset : {1084u, 1088u, 1092u, 1100u, 1104u,
                              1108u, 1296u, 1300u, 1304u})
        Check(runtime.words[actor + offset] == 0u,
              "destructor clears pointer ranges");

    Runtime retained;
    retained.words[actor + 1316u] = 0x7200u;
    Check(ActorManagerPlayerCombatActor::Delete(
              actor, 0u, 0x9000u, retained) == actor &&
              retained.trace.back() == Event{"destroy-data", actor, 0x9000u},
          "nondeleting destructor skips actor free and unowned child");
    retained.trace.clear();
    retained.bytes[actor + 1312u] = 1u;
    (void)ActorManagerPlayerCombatActor::Delete(
        actor, 2u, 0x9000u, retained);
    Check(retained.trace.back() == Event{"destroy-data", actor, 0x9000u} &&
              retained.trace[4] == Event{"delete-child", 0x7200u, 1u},
          "flag bit zero controls free while owned child still deletes");
}

}  // namespace

int main() {
    try {
        Construction();
        Getters();
        Destruction();
        std::cout << "actor_manager_player_combat_actor_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
