#include "pal4/native_monster_combat_collector.h"
#include "pal4/native_monster_manager_owner.h"
#include "pal4/native_monster_object_owner.h"
#include "pal4/native_game_object_animation_lifecycle.h"
#include "pal4/native_monster_combat_handoff.h"
#include "pal4/native_pal_game_iv_texture_storage.h"
#include "pal4/native_pal_game_iv_state_list.h"
#include "pal4/native_world_state_owner.h"
#include "pal4/native_event_document_runtime.h"
#include "pal4/native_script_combat_start.h"
#include "pal4/native_script_combat_config.h"
#include "pal4/native_floor_combat_areas.h"
#include "support/rw_dict_schema_fake.h"
#include <bit>
#include <cmath>
#include <cstring>
#include <iostream>
#include <stdexcept>

namespace {
void Check(bool value, const char* message) { if (!value) throw std::runtime_error(message); }
class Schema final : public pal4::test::RwDictSchemaFake {
public: std::uint32_t SetCurrentDict(std::uint32_t value) override {
    Check(!value, "collector fixture has no model dictionary"); return 0u;
}};
class Animation final : public pal4::NativeGameObjectAnimationLifecycle {
public:
    using NativeGameObjectAnimationLifecycle::NativeGameObjectAnimationLifecycle;
    std::uint32_t FindHierarchy(std::uint32_t) override { throw std::logic_error("unloaded model"); }
    void ProcessAnimationData(pal4::GameObjectEmbeddedAnimationState&, std::uint32_t, std::int32_t) override {
        throw std::logic_error("unloaded animation");
    }
};
class Geometry final : public pal4::IMonsterProximityRuntime {
public:
    pal4::NativeMonsterManagerOwner& manager;
    bool fail{};
    unsigned wall_queries{};
    explicit Geometry(pal4::NativeMonsterManagerOwner& manager) : manager(manager) {}
    pal4::MonsterProximityActor ControlledActor() override { throw std::logic_error("unused controlled query"); }
    double PlanarDistance(float x, float z) override { return std::sqrt(x*x+z*z); }
    float DetectionRadius(std::uint32_t) override { throw std::logic_error("unused detection query"); }
    std::uint32_t CollisionClump() override { return 123u; }
    bool CollisionBlocked(const std::array<float,6>& endpoints, std::uint32_t clump) override {
        ++wall_queries; Check(clump == 123u, "collector forwards the borrowed wall clump");
        bool rejected{};
        try { manager.Destroy(); } catch (const std::logic_error&) { rejected = true; }
        Check(rejected, "candidate query cannot destroy the manager's active group traversal");
        if (fail) throw std::runtime_error("intentional wall-query failure");
        return endpoints[3] == 30.0f;
    }
    void NotifyReturnTargetReached(std::uint32_t, std::uint32_t, std::uint32_t) override {
        throw std::logic_error("unused return query");
    }
};
pal4::BinaryXmlConfig Config(pal4::BinaryXmlNode node) {
    auto bytes = pal4::BinaryXmlConfig::Serialize(node);
    pal4::BinaryXmlConfig result;
    Check(bytes && result.Parse(*bytes), "serialize actual native config data"); return result;
}
}
int main(int argc, char** argv) try {
    pal4::NativeGameDatabase database;
    std::unique_ptr<pal4::NativeFloorCombatAreas> original_areas;
    if (argc == 2) {
        Check(database.Load(argv[1]), "load original database for collected handoff area");
        original_areas = std::make_unique<pal4::NativeFloorCombatAreas>(database);
    }
    pal4::NativeBaseObjectMemory memory;
    Schema schema; Animation animation(memory);
    pal4::NativeMonsterManagerOwner manager;
    (void)manager.Construct();
    std::vector<pal4::NativeMonsterInfoRecord> records;
    for (auto name : {"root","a-near","b-script","c-inactive","d-idle","e-far","f-high","g-wall","h-empty"}) {
        pal4::NativeMonsterInfoRecord record;
        record.name = name; record.resource = "fixture";
        record.values = record.name == "root" ? std::vector<std::uint32_t>{100,101} : std::vector<std::uint32_t>{200};
        if (record.name == "h-empty") record.values.clear();
        record.fields = {1u, std::bit_cast<std::uint32_t>(100.f), std::bit_cast<std::uint32_t>(1.f),
                         std::bit_cast<std::uint32_t>(100.f), static_cast<std::uint32_t>(record.values.size())};
        record.behavior = Config({"behavior"});
        record.buffer_cache = Config({"cache",{
            {"AAB_MST_ESSBEH_combat_pos_0",3u,std::string("3")},
            {"AAB_MST_ESSBEH_script_func",3u,record.name == "b-script" ? std::string("story") : std::string{}}}});
        records.push_back(std::move(record));
    }
    Check(manager.InitializeScene(records, [&](const auto& record) {
        auto owner = std::make_unique<pal4::NativeMonsterObjectOwner>(memory,schema,animation);
        const auto object = owner->Construct();
        pal4::NativeBorrowedActorBinding::BindLabel(memory,object,record.name);
        memory.WriteDword(object + 0x654u, manager.FindInfo(record.name));
        memory.WriteDword(object + 0x1f0u, record.name == "c-inactive" ? 0u : 1u);
        memory.WriteDword(object + 0x6e0u, record.name == "d-idle" ? 0u : 1u);
        memory.WriteDword(object + 0xdcu, std::bit_cast<std::uint32_t>(10.f));
        const auto x = record.name == "root" ? 0.f : record.name == "e-far" ? 200.f : record.name == "g-wall" ? 30.f : 5.f;
        const auto y = record.name == "f-high" ? 50.f : 0.f;
        for (auto offset : {0xacu,0xe0u}) memory.WriteDword(object+offset,std::bit_cast<std::uint32_t>(x));
        for (auto offset : {0xb0u,0xe4u}) memory.WriteDword(object+offset,std::bit_cast<std::uint32_t>(y));
        if (record.name == "a-near") memory.WriteDword(object+0x730u,2u);
        auto* component = owner.get();
        Check(manager.RegisterOwnedObject(record.name,object,std::move(owner),{},{},{},component),
            "register the actual creating native monster owner");
    }), "load native info/grid/owned scene fixtures");
    const auto root = manager.FindObject("root");
    memory.WriteDword(root+0x6e4u,root);
    auto& image = *reinterpret_cast<pal4::MonsterBehaviorManagerState*>(root);
    auto reset = [&] {
        for (auto p=image.words[0x724u/4u];p!=image.words[0x728u/4u];p+=16u)
            pal4::MonsterBehaviorManager::ClearHistoryRecord(p,manager.OwnedComponent(root).Runtime());
        image.words[0x718u/4u]=image.words[0x714u/4u];
        image.words[0x728u/4u]=image.words[0x724u/4u];
    };
    Geometry geometry(manager);
    pal4::NativeMonsterCombatCollector collector(manager,memory,geometry);
    for (unsigned iteration=0;iteration<3u;++iteration) {
        (void)collector.Collect(root);
        const auto begin=image.words[0x714u/4u], history=image.words[0x724u/4u];
        Check(image.words[0x718u/4u]==begin+36u && image.words[0x728u/4u]==history+48u,
            "collector includes initiator, nearby pursuing peer and empty peer name only");
        const std::array<std::array<std::uint32_t,3>,3> expected{{{100,3,0},{101,0,0},{200,3,2}}};
        Check(std::memcmp(reinterpret_cast<const void*>(begin),expected.data(),36u)==0,
            "ordered source ids, missing-position fallback and candidate abnormal state survive collection");
        unsigned index{};
        for (auto name : {"root","a-near","h-empty"}) {
            const auto text=memory.ReadDword(history+index++*16u+4u);
            Check(std::string_view(reinterpret_cast<const char*>(text))==name,"collected owned instance names");
        }
        Check(geometry.wall_queries>0u,"group collection checks actual wall-service boundary");
        reset();
    }
    geometry.fail=true;
    bool failed{};
    try { (void)collector.Collect(root); } catch (const std::runtime_error& e) {
        failed=std::string_view(e.what())=="intentional wall-query failure";
    }
    Check(failed && image.words[0x718u/4u]==image.words[0x714u/4u]+24u,
        "candidate failure preserves already collected initiator records");
    reset(); geometry.fail=false;
    (void)collector.Collect(root);
    Check(image.words[0x718u/4u]==image.words[0x714u/4u]+36u,
        "collection and tree guards unwind for retry");
    pal4::NativePalGameIVTextureStorage game([](auto) {});
    const auto game_address = game.Construct();
    pal4::NativeWorldStateOwner scene;
    const auto scene_address = scene.Construct();
    pal4::NativePalGameIVStateList states(game_address);
    Check(states.Register(scene_address, 1u, [] {}), "register Scene for collected handoff");
    memory.WriteDword(game_address + 396u, scene_address);
    pal4::NativeEventDocumentRuntime documents(memory);
    pal4::NativeScriptCombatConfig config;
    pal4::ScriptCombatExecutor executor{0x1000u, {}};
    unsigned entered{};
    pal4::NativeScriptCombatStart request(executor, documents, config, game_address, [&](auto document) {
        ++entered;
        Check(documents.ChildCount(document) == 3u && config.Snapshot().words[2] == 3u,
            "native handoff receives the collected record/name counts");
        Check(documents.AttributeToString(documents.FindNumericAttribute(document,"CombatArea")) == "M01",
            "floor area resolution reaches the actual parameter document");
        const auto last = documents.ChildAt(document, 2u);
        Check(documents.AttributeToInt(documents.FindNumericAttribute(last, "MonsterId"), -1) == 200 &&
              documents.AttributeToInt(documents.FindNumericAttribute(last, "MonsterAbState"), -1) == 1,
            "collected peer and snare state reach the native parameter writer");
    });
    pal4::NativeMonsterCombatHandoff handoff(game, states, request, [&](auto arena) {
        Check(arena == "M01", "handoff uses supplied actual scene identity");
        // Bounded integration with the retained original cave floor sample.
        // The scene bodies/geometry/Entry above remain explicit fixtures.
        return original_areas ? original_areas->Resolve(arena, 25, 0) : std::string("M01");
    });
    (void)handoff.Process(manager.OwnedComponent(root), -1, "M01");
    Check(entered == 1u && !documents.DocumentCount() && !request.Document(),
        "collected native handoff enters once and closes temporary XML");
    manager.Destroy();
    std::vector<std::string> retained;
    game.VisitStateMonsters([&](auto name) { retained.emplace_back(name); });
    Check(retained == std::vector<std::string>{"root","a-near","h-empty"},
        "game owns collected names after all scene monster owners are destroyed");
    memory.WriteDword(game_address + 396u, 0u);
    states.Clear(); scene.Destroy(); game.Destroy();
    Check(!memory.OutstandingAllocations() && !manager.OutstandingAllocations(),
        "scene destruction cleans nonempty host lists and all constructed native owners");
    std::cout << "PASS: native monster combat group collection, source config, filtering and owner cleanup\n";
    return 0;
} catch (const std::exception& e) { std::cerr << "FAIL: " << e.what() << '\n'; return 1; }
