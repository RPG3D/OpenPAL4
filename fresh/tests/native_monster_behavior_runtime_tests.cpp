#include "pal4/native_monster_behavior_runtime.h"
#include "pal4/native_monster_object_owner.h"
#include "pal4/native_game_object_animation_lifecycle.h"
#include "pal4/native_monster_info_reader.h"
#include "pal4/native_monster_fuzzy_behavior_plan.h"
#include "pal4/native_actor_animation_catalog.h"
#include "pal4/native_package_archive.h"
#include "pal4/native_arena_cache_owner.h"
#include "support/rw_dict_schema_fake.h"

#include <cstdint>
#include <bit>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <stdexcept>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::fprintf(stderr, "FAIL: %s\n", message);
        std::exit(EXIT_FAILURE);
    }
}

class EmptyAnimationRuntime final :
    public pal4::NativeGameObjectAnimationLifecycle {
public:
    using NativeGameObjectAnimationLifecycle::NativeGameObjectAnimationLifecycle;
    std::uint32_t FindHierarchy(std::uint32_t) override {
        throw std::logic_error("unloaded monster has no animation hierarchy");
    }
    void ProcessAnimationData(pal4::GameObjectEmbeddedAnimationState&,
                              std::uint32_t, std::int32_t) override {
        throw std::logic_error("unloaded monster has no animation data");
    }
};

class EmptySchemaRuntime final : public pal4::test::RwDictSchemaFake {
public:
    std::uint32_t SetCurrentDict(std::uint32_t dict) override {
        Check(dict == 0u, "empty monster base clears resource dictionaries");
        ++resets;
        return 0u;
    }
    std::uint32_t resets = 0u;
};

}  // namespace

int main(int argc, char** argv) {
    pal4::NativeBaseObjectMemory memory;
    EmptySchemaRuntime schema;
    EmptyAnimationRuntime animation(memory);
    pal4::NativeMonsterBehaviorRuntime runtime(memory, schema, animation);
    for (const auto references : {0u, 0xffu, 2u}) {
        const auto text = memory.Allocate(6u);
        memory.WriteByte(text, static_cast<std::uint8_t>(references));
        std::array<std::uint32_t, 4u> record{123u, text + 1u, 4u, 4u};
        const auto record_address = reinterpret_cast<std::uint32_t>(record.data());
        runtime.SetRecordErrorFlag(record_address, 1u);
        Check(record[0u] == 123u && record[1u] == 0u && record[2u] == 0u &&
                  record[3u] == 0u &&
                  memory.OutstandingAllocations() == (references == 2u ? 1u : 0u),
              "native history cleanup releases or detaches the existing legacy string image");
        if (references == 2u) {
            Check(memory.ReadByte(text) == 1u,
                  "shared history string decrements its actual reference byte");
            (void)memory.Free(text);
        }
    }
    const auto address = memory.Allocate(
        sizeof(pal4::MonsterBehaviorManagerState));
    std::memset(reinterpret_cast<void*>(static_cast<std::uintptr_t>(address)),
                0, sizeof(pal4::MonsterBehaviorManagerState));
    auto& state = *reinterpret_cast<pal4::MonsterBehaviorManagerState*>(
        static_cast<std::uintptr_t>(address));
    Check(pal4::MonsterBehaviorManager::Construct(state, address, runtime) ==
              address && state.words[0] == 0x842c98u &&
              state.words[0x6a8u / 4u] == 4u &&
              runtime.DocumentCount() == 1u &&
              runtime.BehaviorTreeNil() != 0u,
          "native monster constructor initializes base, event document and four behaviors");
    const auto event = runtime.AddScriptEvent(state, "MST-Fuzzy-Behavior");
    const auto allocations_with_event = memory.OutstandingAllocations();
    Check(event != 0u &&
              std::strcmp(reinterpret_cast<const char*>(
                  static_cast<std::uintptr_t>(memory.ReadDword(event + 4u))),
                  "MST-Fuzzy-Behavior") == 0 &&
              runtime.AddScriptEvent(state, "MST-Fuzzy-Behavior") == 0u &&
              memory.OutstandingAllocations() == allocations_with_event,
          "native monster event inserts once and rejects duplicate names");
    const auto second_event = runtime.AddScriptEvent(state, "MST-Secondary-Test");
    Check(second_event != 0u && second_event != event &&
              std::strcmp(reinterpret_cast<const char*>(
                  static_cast<std::uintptr_t>(memory.ReadDword(event + 4u))),
                  "MST-Fuzzy-Behavior") == 0,
          "native event array growth preserves the first event");
    const std::string_view atomic_names[] = {
        "behAIAtomic-attribute", "behAIAtomic-attribute"};
    runtime.SetEventAtomicNodeNames(event, atomic_names);
    const auto atomic_begin = memory.ReadDword(event + 24u);
    Check(atomic_begin != 0u &&
              memory.ReadDword(event + 28u) == atomic_begin + 120u &&
              memory.ReadDword(atomic_begin + 56u) == 0u &&
              std::strcmp(reinterpret_cast<const char*>(
                  static_cast<std::uintptr_t>(
                      memory.ReadDword(atomic_begin + 4u))),
                  "behAIAtomic-attribute") == 0,
          "native event owns original-sized XML atomic nodes");
    runtime.AddEventAtomicIntAttribute(
        event, 0u, "behAIAtomic-classID", 1);
    runtime.AddEventAtomicIntAttribute(
        event, 0u, "behAIAtomic-proportion", 0);
    runtime.AddEventAtomicIntAttribute(
        event, 0u, "beh-vtm-waitMode", 2);
    runtime.AddEventAtomicFloatAttribute(
        event, 0u, "beh-vtm-waitTime", 4.0f);
    runtime.AddEventAtomicFloatAttribute(
        event, 0u, "beh-vtm-expectTime", 10.0f);
    Check(memory.ReadDword(atomic_begin + 52u) == 5u,
          "fuzzy atomic has five numeric XML attributes");
    const auto find_attribute = [&](const char* name) {
        const auto head = memory.ReadDword(atomic_begin + 24u);
        auto node = memory.ReadDword(head + 4u);
        const auto nil = runtime.DocumentNil();
        while (node != nil) {
            const auto text = reinterpret_cast<const char*>(
                static_cast<std::uintptr_t>(memory.ReadDword(node + 16u)));
            const auto order = std::strcmp(name, text);
            if (order == 0) return node + 28u;
            node = memory.ReadDword(node + (order < 0 ? 0u : 8u));
        }
        return std::uint32_t{0u};
    };
    const auto class_attribute = find_attribute("behAIAtomic-classID");
    const auto wait_attribute = find_attribute("beh-vtm-waitTime");
    Check(class_attribute != 0u && wait_attribute != 0u &&
              memory.ReadDword(class_attribute + 16u) == 1u &&
              memory.ReadDword(class_attribute + 24u) == 1u &&
              memory.ReadDword(wait_attribute + 16u) == 2u &&
              memory.ReadDword(wait_attribute + 24u) ==
                  std::bit_cast<std::uint32_t>(4.0f),
          "fuzzy atomic tree stores original int and float types");
    const auto before_failed_nodes = memory.OutstandingAllocations();
    const std::string_view invalid_names[] = {
        "behAIAtomic-attribute", ""};
    bool rejected_empty_name = false;
    try {
        runtime.SetEventAtomicNodeNames(second_event, invalid_names);
    } catch (const std::invalid_argument&) {
        rejected_empty_name = true;
    }
    Check(rejected_empty_name &&
              memory.ReadDword(second_event + 24u) == 0u &&
              memory.OutstandingAllocations() == before_failed_nodes,
          "failed atomic array construction releases earlier XML nodes");
    std::uint32_t record[20]{};
    record[0x48u / 4u] = 1u;
    bool behavior_unbound = false;
    try {
        (void)pal4::MonsterBehaviorManager::InitializeComponent(
            state, address, static_cast<std::uint32_t>(
                reinterpret_cast<std::uintptr_t>(record)), false, runtime);
    } catch (const std::logic_error&) {
        behavior_unbound = true;
    }
    Check(behavior_unbound,
          "nonempty XML behavior reports its unbound native runtime");
    pal4::MonsterBehaviorManager::Destroy(state, address, runtime);
    Check(runtime.DocumentCount() == 0u &&
              runtime.BehaviorTreeNil() == 0u &&
              runtime.BehaviorTreeReferences() == 0u &&
              memory.OutstandingAllocations() == 1u,
          "native monster destructor releases all embedded members");
    (void)memory.Free(address);
    Check(memory.OutstandingAllocations() == 0u && schema.resets == 3u,
          "native monster storage and dictionaries return to baseline");
    {
        pal4::NativeMonsterObjectOwner object(memory, schema, animation);
        const auto monster = object.Construct();
        Check(monster != 0u && object.Runtime().DocumentCount() == 1u,
              "monster owner constructs the complete behavior object");
        object.BindBorrowedActor(0x123400u, 0x123800u);
        const pal4::NativeBorrowedAnimationClip clips[] = {
            {"stand", 0x234500u}, {"walk", 0x234600u}};
        object.BindBorrowedAnimations("gamedata\\PALActor\\421\\", clips);
        object.SetStateFlag(1u);
        Check(object.SelectBorrowedAction("walk") &&
                  !object.SelectBorrowedAction("missing") &&
                  memory.ReadDword(monster + 0x14u) == 0x123400u &&
                  memory.ReadDword(monster + 0x17cu) !=
                      memory.ReadDword(monster + 0x178u),
              "monster object binds borrowed clumps and selects action records");
        object.BindManagerGrid(0x345600u);
        const auto document=[](pal4::BinaryXmlNode root) {
            pal4::BinaryXmlConfig config;
            const auto bytes=pal4::BinaryXmlConfig::Serialize(root);
            Check(bytes && config.Parse(*bytes),"encode monster storage document");
            return config;
        };
        const auto snapshot=document({"soMonster",{{"valName",3u,std::string("monster")},
                                                    {"valVisible",1u,std::int32_t(0)}}});
        std::array<std::uint32_t,20u> storage_info{};
        storage_info[0x4cu/4u]=0x123456u;
        const auto absent=document({"bufferCache"});
        Check(!object.CaptureStorageDocument(absent),"monster without component has no save snapshot");
        Check(object.RestoreStorageDocument(snapshot,absent)==0u && memory.ReadDword(monster+0x1f0u)==1u,
              "monster without component ignores storage snapshot");
        memory.WriteDword(monster+0x654u,reinterpret_cast<std::uint32_t>(storage_info.data()));
        const auto old_name=memory.ReadDword(monster+4u);
        const char storage_name[]="monster";
        memory.WriteDword(monster+4u,reinterpret_cast<std::uint32_t>(storage_name));
        for(const auto gate:{-1,0,1}) {
            object.SetStateFlag(7u);
            const auto config=document({"bufferCache",{{"PAL4_MST_NEED_STORAGE",1u,std::int32_t(gate)}}});
            const auto allocations=memory.OutstandingAllocations();
            const auto captured=object.CaptureStorageDocument(config);
            Check(bool(captured)==(gate>0) && memory.OutstandingAllocations()==allocations,
                  "native capture obeys storage gate without legacy allocations");
            if (captured)
                Check(captured->id==11u && captured->document.Root().name=="soMonster" &&
                      std::get<std::string>(captured->document.Root().FindAttribute("valName")->value)=="monster" &&
                      captured->document.Root().FindAttribute("valVisible")->ToInt(0u)==7u,
                      "shared monster writer snapshots live name and state");
            (void)object.RestoreStorageDocument(snapshot,config);
            Check(memory.ReadDword(monster+0x1f0u)==(gate>0?0u:7u),
                  "native storage uses original positive configuration gate");
        }
        object.SetStateFlag(7u);
        (void)object.RestoreStorageDocument(snapshot,absent);
        Check(memory.ReadDword(monster+0x1f0u)==7u,"missing storage gate preserves monster state");
        bool bad_snapshot=false;
        try { (void)object.RestoreStorageDocument(document({"soEffect"}),absent); }
        catch(const std::invalid_argument&) { bad_snapshot=true; }
        Check(bad_snapshot && memory.ReadDword(monster+0x1f0u)==7u,
              "wrong monster snapshot root fails before mutation");
        const auto enabled=document({"bufferCache",{{"PAL4_MST_NEED_STORAGE",1u,std::int32_t(1)}}});
        object.SetStateFlag(0u);
        const auto saved_monster=object.CaptureStorageDocument(enabled);
        Check(saved_monster.has_value(),"enabled hidden monster is saved rather than filtered by visibility");
        object.SetStateFlag(7u);
        pal4::NativeArenaCacheSection cache_section;
        pal4::NativeArenaCacheEntry cache_entry;
        cache_entry.names={"world","scene"};
        cache_entry.groups[1].push_back(*saved_monster);
        auto missing_root=snapshot.Root();
        for(auto& attribute:missing_root.attributes)
            if(attribute.name=="valName") attribute.value=std::string("absent");
        cache_entry.groups[1].push_back({11u,document(missing_root)});
        cache_section.entries.push_back(cache_entry);
        pal4::NativeGameSaveArchiveBuffer cache_bytes;
        pal4::NativeArenaCacheSaveSection cache_writer(cache_bytes);
        Check(cache_writer.Serialize(cache_section),"write monster cache with live and missing names");
        pal4::NativeArenaCacheOwner saved_cache;
        pal4::NativeGameSaveArchiveBuffer native_cache_bytes;
        Check(saved_cache.SaveCurrent(cache_entry,native_cache_bytes) && native_cache_bytes.Bytes()==cache_bytes.Bytes(),
              "native ArenaCache capture/write includes monsters with exact structured section bytes");
        pal4::NativeGameSaveArchiveBuffer repeat_bytes;
        const auto saved_allocations=saved_cache.OutstandingAllocations();
        Check(saved_cache.SaveCurrent(cache_entry,repeat_bytes) && repeat_bytes.Bytes()==native_cache_bytes.Bytes() &&
              saved_cache.OutstandingAllocations()==saved_allocations,
              "repeated monster cache capture replaces snapshots without leaks or duplication");
        saved_cache.Destroy();
        Check(saved_cache.OutstandingAllocations()==0u,"native saved monster cache destroys all snapshots");
        pal4::NativeArenaCacheOwner cache;
        std::size_t cache_consumed{};
        Check(cache.LoadSection(std::span(reinterpret_cast<const std::uint8_t*>(cache_bytes.Bytes().data()),
              cache_bytes.Bytes().size()),cache_consumed) && cache_consumed==cache_bytes.Bytes().size(),
              "shared cache loader retains monster snapshot and exact cursor");
        unsigned restored_monsters{},restored_minimaps{};
        cache.BindRestoreCallbacks({}, {}, [&](auto,const auto&) { ++restored_minimaps; },
            [&](std::string_view name) { return name=="monster" ? monster : 0u; },
            [&](std::uint32_t address,const pal4::NativeArenaCacheChild& child) {
                Check(address==monster && child.id==11u,"monster cache resolves owned receiver and type");
                (void)object.RestoreStorageDocument(child.document,enabled);
                ++restored_monsters;
            });
        Check(cache.Restore("world","scene")==1u && restored_monsters==1u && restored_minimaps==1u &&
              memory.ReadDword(monster+0x1f0u)==0u,
              "cache restores native monster state, skips missing names and continues to minimap");
        cache.BindRestoreCallbacks({}, {}, {});
        bool callbacks_cleared=false;
        try { (void)cache.Restore("world","scene"); } catch(const std::logic_error&) { callbacks_cleared=true; }
        Check(callbacks_cleared,"clearing scene bindings also releases monster callbacks");
        cache.Destroy();
        Check(cache.OutstandingAllocations()==0u,"monster snapshots release all cache allocations");
        memory.WriteDword(monster+0x654u,0u);
        memory.WriteDword(monster+4u,old_name);
        object.Destroy();
        Check(object.Address() == 0u &&
                  object.Runtime().DocumentCount() == 0u &&
                  memory.OutstandingAllocations() == 0u,
              "monster owner releases records and members without owning clips");
    }
    if (argc == 2) {
        pal4::NativeMonsterInfoReader reader;
        Check(reader.Load(argv[1], "M06", "3A") &&
                  reader.Records().size() == 3u,
              "original M06/3A monster records load");
        const auto& records = reader.Records();
        const auto first = pal4::NativeMonsterFuzzyBehaviorPlan::FromDocument(
            records[0].behavior);
        const auto second = pal4::NativeMonsterFuzzyBehaviorPlan::FromDocument(
            records[1].behavior);
        const auto third = pal4::NativeMonsterFuzzyBehaviorPlan::FromDocument(
            records[2].behavior);
        Check(records[0].name == "m06-02-05" &&
                  records[1].name == "m06-02-07" &&
                  records[2].name == "m06-05-02" &&
                  first && second && !third,
              "M06/3A has two fuzzy behavior events and one empty behavior");
        pal4::NativePackageArchive actors;
        Check(actors.Open((std::filesystem::path(argv[1]) / "gamedata" /
                           "PALActor.cpk").string().c_str()),
              "open original monster actor package");
        for (const auto& monster : records) {
            const auto dff = monster.resource + "\\" + monster.resource +
                ".dff";
            pal4::NativeActorAnimationCatalog catalog;
            Check(actors.Find(dff.c_str()) >= 0 &&
                      catalog.Load(argv[1], monster.resource) &&
                      !catalog.Entries().empty(),
                  "M06/3A monster has a model and action table");
        }
        Check(first->group_mode == 0 && first->atomics.size() == 1u &&
                  first->atomics[0].class_id == 1 &&
                  first->atomics[0].proportion == 0 &&
                  first->atomics[0].wait_mode == 2 &&
                  first->atomics[0].wait_time == 4.0f &&
                  first->atomics[0].expect_time == 10.0f &&
                  second->group_mode == 0 && second->atomics.size() == 1u &&
                  second->atomics[0].class_id == 1 &&
                  second->atomics[0].wait_mode == 2 &&
                  second->atomics[0].wait_time == 3.0f &&
                  second->atomics[0].expect_time == 30.0f,
              "M06/3A fuzzy rules preserve original type, weights and timing");
        for (std::size_t index = 0u; index < records.size(); ++index) {
            pal4::NativeMonsterObjectOwner object(memory, schema, animation);
            const auto monster = object.Construct();
            object.BindBorrowedActor(0x123400u +
                                         static_cast<std::uint32_t>(index) * 0x100u,
                                     0u);
            object.BindManagerGrid(0x345600u);
            std::uint32_t info[20]{};
            info[0x48u / 4u] = 0x123000u +
                                  static_cast<std::uint32_t>(index) * 4u;
            info[0x4cu / 4u] = 0x124000u + static_cast<std::uint32_t>(index)*4u;
            object.InitializeComponent(
                static_cast<std::uint32_t>(
                    reinterpret_cast<std::uintptr_t>(info)),
                records[index].behavior);
            pal4::BinaryXmlNode storage_root{"soMonster",{{"valName",3u,records[index].name},
                {"valVisible",1u,std::int32_t(0)}}};
            pal4::BinaryXmlConfig storage_snapshot;
            const auto storage_bytes=pal4::BinaryXmlConfig::Serialize(storage_root);
            Check(storage_bytes && storage_snapshot.Parse(*storage_bytes),"build original monster snapshot");
            object.SetStateFlag(7u);
            (void)object.RestoreStorageDocument(storage_snapshot,records[index].buffer_cache);
            const auto* storage_gate=records[index].buffer_cache.Root().FindAttribute("PAL4_MST_NEED_STORAGE");
            const bool required=storage_gate && static_cast<std::int32_t>(storage_gate->ToInt(0u))>0;
            Check(memory.ReadDword(monster+0x1f0u)==(required?0u:7u),
                  "original M06/3A buffer-cache config controls native monster restoration");
            const auto processor = monster +
                pal4::MonsterBehaviorManager::kProcessorOffset;
            const auto begin = memory.ReadDword(processor + 4u);
            const auto end = memory.ReadDword(processor + 8u);
            const auto event_count = begin == 0u ? 0u : (end - begin) / 4u;
            Check(event_count == (index < 2u ? 1u : 0u),
                  "original monster component creates expected fuzzy event");
            if (event_count != 0u) {
                const auto fuzzy_event = memory.ReadDword(begin);
                const auto atomic_node = memory.ReadDword(fuzzy_event + 24u);
                Check(memory.ReadDword(fuzzy_event + 16u) == 0u &&
                          memory.ReadDword(fuzzy_event + 28u) ==
                              atomic_node + 60u &&
                          memory.ReadDword(atomic_node + 52u) == 5u,
                      "original fuzzy event holds one complete atomic record");
            }
            object.Destroy();
            Check(memory.OutstandingAllocations() == 0u &&
                      object.Runtime().DocumentCount() == 0u,
                  "original monster behavior component tears down cleanly");
        }
    }
}
