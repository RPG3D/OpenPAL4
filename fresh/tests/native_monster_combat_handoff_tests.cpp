#include "pal4/native_monster_combat_handoff.h"
#include "pal4/native_monster_object_owner.h"
#include "pal4/native_pal_game_iv_texture_storage.h"
#include "pal4/native_pal_game_iv_state_list.h"
#include "pal4/native_world_state_owner.h"
#include "pal4/native_world_loading_state_owner.h"
#include "pal4/native_script_combat_start.h"
#include "pal4/native_script_combat_config.h"
#include "pal4/native_event_document_runtime.h"
#include "pal4/native_game_object_animation_lifecycle.h"
#include "support/rw_dict_schema_fake.h"

#include <array>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace {
void Check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}
class EmptyAnimation final : public pal4::NativeGameObjectAnimationLifecycle {
public:
    using NativeGameObjectAnimationLifecycle::NativeGameObjectAnimationLifecycle;
    std::uint32_t FindHierarchy(std::uint32_t) override {
        throw std::logic_error("handoff does not load an actor");
    }
    void ProcessAnimationData(pal4::GameObjectEmbeddedAnimationState&,
        std::uint32_t, std::int32_t) override {
        throw std::logic_error("handoff does not draw an actor");
    }
};
class EmptySchema final : public pal4::test::RwDictSchemaFake {
public:
    std::uint32_t SetCurrentDict(std::uint32_t value) override {
        Check(!value, "unloaded handoff owner has no dictionary"); return 0u;
    }
};
}

int main() try {
    pal4::NativeBaseObjectMemory memory;
    EmptySchema schema;
    EmptyAnimation animation(memory);
    pal4::NativeMonsterObjectOwner monster(memory, schema, animation);
    const auto object = monster.Construct();
    auto& image = *reinterpret_cast<pal4::MonsterBehaviorManagerState*>(object);
    pal4::NativePalGameIVTextureStorage game([](auto) {
        throw std::logic_error("handoff test has no texture dictionaries");
    });
    const auto game_address = game.Construct();
    pal4::NativeWorldStateOwner scene;
    pal4::NativeWorldLoadingStateOwner loading;
    const auto scene_address = scene.Construct();
    const auto loading_address = loading.Construct();
    pal4::NativePalGameIVStateList states(game_address);
    Check(states.Register(scene_address, 1u, [] {}) &&
          states.Register(loading_address, 6u, [] {}), "register actual state owners");
    const auto set_state = [&](auto state) { memory.WriteDword(game_address + 396u, state); };
    pal4::NativeEventDocumentRuntime docs(memory);
    pal4::NativeScriptCombatConfig config;
    pal4::ScriptCombatExecutor executor{0x1000u, {}};
    (void)executor.commands.Push(999u, 7u);
    unsigned entries{}, resolutions{};
    bool fail_entry{}, fail_resolver{};
    std::uint32_t expected_mode{}, saved{};
    std::string resolved_area = "M01";
    pal4::NativeMonsterCombatHandoff* bridge{};
    auto integer = [&](auto node, const char* key) {
        return docs.AttributeToInt(docs.FindNumericAttribute(node, key), -1);
    };
    pal4::NativeScriptCombatStart request(executor, docs, config, game_address,
        [&](auto document) {
            ++entries;
            Check(docs.NodeName(document) == "CombatParam" && docs.ChildCount(document) == 3u,
                "handoff uses the actual native parameter document");
            Check(docs.AttributeToString(docs.FindNumericAttribute(document, "CombatScene")) == "combatWorld" &&
                  docs.AttributeToString(docs.FindNumericAttribute(document, "CombatArea")) ==
                    (resolved_area.empty() ? "x" : resolved_area), "original world node case and area fallback");
            Check(integer(document, "StartType") == static_cast<int>(expected_mode),
                "original surprise mode mapping");
            Check(config.Snapshot().words[2] == 3u, "property110 counts deduplicated scene monster names");
            for (unsigned i = 0; i < 3u; ++i) {
                const auto child = docs.ChildAt(document, i);
                Check(integer(child, "MonsterId") == static_cast<int>(2579u + i) &&
                      integer(child, "CombatPosition") == static_cast<int>(i) &&
                      integer(child, "MonsterAbState") == (i == 1u ? 1 : 0),
                    "native document retains ordered records and snare group mapping");
            }
            Check(image.words[0x718u / 4u] == image.words[0x714u / 4u] &&
                  image.words[0x728u / 4u] == image.words[0x724u / 4u],
                "pending arrays are consumed before synchronous entry as in434E30");
            bool rejected{};
            try { (void)bridge->Process(monster, -1, "M01"); }
            catch (const std::logic_error&) { rejected = true; }
            Check(rejected, "state entry cannot recursively replace its temporary document");
            if (fail_entry) throw std::runtime_error("intentional encounter entry failure");
            saved = docs.CloneDocument(document);
        });
    pal4::NativeMonsterCombatHandoff handoff(game, states, request, [&](auto arena) {
        ++resolutions;
        Check(arena == "M01", "resolver receives the live arena name");
        if (fail_resolver) throw std::runtime_error("intentional area lookup failure");
        return resolved_area;
    });
    bridge = &handoff;
    std::uint32_t records{}, history{};
    auto publish = [&] {
        auto& storage = monster.Runtime().Encounters();
        for (const auto& record : std::array<std::array<std::uint32_t, 3>, 3>{{{2579,0,0},{2580,1,2},{2581,2,1}}})
            storage.Append(image, record);
        for (const auto* name : {"m01-01-01", "m01-01-01", "m01-01-02", ""})
            storage.AddName(image, name);
        records = image.words[0x714u / 4u];
        history = image.words[0x724u / 4u];
    };
    publish();
    set_state(0u);
    Check(handoff.Process(monster, -1, "M01") == 0 && !entries && !resolutions,
        "no current controller does not consume or create a request");
    set_state(loading_address);
    Check(handoff.Process(monster, -1, "M01") == 6 && !entries && !resolutions,
        "registered non-Scene state returns its real id");
    set_state(scene_address);
    game.AddStateMonster("stale");
    const auto before_lookup = memory.OutstandingAllocations();
    fail_resolver = true;
    bool failed{};
    try { (void)handoff.Process(monster, -1, "M01"); }
    catch (const std::runtime_error& e) { failed = std::string_view(e.what()) == "intentional area lookup failure"; }
    Check(failed && !request.Document() && !docs.DocumentCount() &&
          memory.OutstandingAllocations() == before_lookup && game.StateMonsterCount() == 1u &&
          image.words[0x718u / 4u] == records + 36u,
        "lookup failure closes temporary XML before consuming names or pending monsters");
    fail_resolver = false;
    for (auto mode : {0, 1, 2, -1}) {
        expected_mode = mode == 1 ? 2u : mode == 0 ? 0u : 1u;
        fail_entry = mode == 0;
        failed = false;
        try { (void)handoff.Process(monster, mode, "M01"); }
        catch (const std::runtime_error& e) { failed = std::string_view(e.what()) == "intentional encounter entry failure"; }
        Check(failed == fail_entry && !request.Document(), "entry success and failure release request scope");
        Check(executor.commands.Commands().size() == 1u && executor.commands.Commands()[0].command == 999u,
            "natural encounter leaves the giAddCombatMonster queue intact");
        std::vector<std::string> names;
        game.VisitStateMonsters([&](auto name) { names.emplace_back(name); });
        Check(names == std::vector<std::string>{"m01-01-01", "m01-01-02", ""},
            "persistent game collection owns unique copied names after history cleanup");
        if (saved) {
            Check(docs.ChildCount(saved) == 3u, "state-owned clone survives temporary XML cleanup");
            docs.DestroyDocument(saved); (void)docs.Free(saved); saved = 0u;
        }
        Check(!docs.DocumentCount(), "no temporary native document leaked");
        if (mode != -1) publish();
        if (mode == 2) resolved_area.clear();
    }
    memory.WriteDword(game_address + 396u, 0u);
    states.Clear(); scene.Destroy(); loading.Destroy(); game.Destroy(); monster.Destroy();
    Check(memory.OutstandingAllocations() == 0u, "handoff releases all fixture/monster/XML memory");
    std::cout << "PASS: native encounter handoff, actual state registry/name ownership/XML writer, modes, failures and copied document\n";
    return 0;
} catch (const std::exception& e) {
    std::cerr << "FAIL: " << e.what() << '\n'; return 1;
}
