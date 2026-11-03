#include "pal4/native_monster_manager_owner.h"
#include "pal4/monster_behavior_manager.h"
#include "pal4/spatial_grid.h"
#include "support/grid_secondary_snapshot_probe.h"

#include <array>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <stdexcept>

namespace {

struct CountedOwner final {
    explicit CountedOwner(std::uint32_t& count) : count_(count) {}
    ~CountedOwner() { ++count_; }
    std::uint32_t& count_;
};

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::fprintf(stderr, "FAIL: %s\n", message);
        std::exit(EXIT_FAILURE);
    }
}

void VerifyRenderTraversal() {
    pal4::NativeMonsterManagerOwner manager;
    (void)manager.Construct();
    std::array<pal4::MonsterBehaviorManagerState,3> images{};
    std::uint32_t destroyed{};
    const std::array<const char*,3> names{"z","a","m"};
    std::vector<std::uint32_t> visited;
    bool fail=true;
    for(unsigned i=0;i<3u;++i){
        images[i].words[0x1f0u/4u]=i==2u?0u:1u;
        const auto address=reinterpret_cast<std::uint32_t>(&images[i]);
        Check(manager.RegisterOwnedObject(names[i],address,std::make_unique<CountedOwner>(destroyed),
            {},{},{},nullptr,{},[&,address](auto camera,auto&){
                Check(camera==0xabcdefu,"render traversal forwards original camera");
                visited.push_back(address);
                if(!fail)return;
                for(unsigned which=0;which<5u;++which){bool rejected{};try{
                    if(which==0u)manager.Destroy();
                    if(which==1u)(void)manager.InitializeScene({});
                    if(which==2u)(void)manager.RegisterConstructedObject("foreign",address);
                    if(which==3u)(void)manager.RenderFrame(camera);
                    if(which==4u)(void)manager.ForEachObject([](auto){return 0u;});
                }catch(const std::logic_error&){rejected=true;}
                Check(rejected && !destroyed && manager.ObjectCount()==3u,"render protects live tree and owned callbacks");}
                throw std::runtime_error("intentional monster render failure");
            }),"register owned render service");
    }
    bool failed{};try{(void)manager.RenderFrame(0xabcdefu);}catch(const std::runtime_error& e){failed=std::string_view(e.what())=="intentional monster render failure";}
    Check(failed && visited==std::vector<std::uint32_t>{reinterpret_cast<std::uint32_t>(&images[1])},"render failure stops actual ordered tree at first callback");
    fail=false;visited.clear();const auto result=manager.RenderFrame(0xabcdefu);
    Check(result.active_objects==2u && visited==std::vector<std::uint32_t>{reinterpret_cast<std::uint32_t>(&images[1]),reinterpret_cast<std::uint32_t>(&images[0])},"render retries ordered active objects and skips inactive object");
    manager.Destroy();Check(destroyed==3u && !manager.OutstandingAllocations(),"render callbacks retire with real owned entities");
    (void)manager.Construct();images[0].words[0x1f0u/4u]=0u;
    Check(manager.RegisterConstructedObject("unbound",reinterpret_cast<std::uint32_t>(&images[0])),"register inactive unbound render image");
    Check(!manager.RenderFrame(0xabcdefu).active_objects,"inactive render does not require creating render service");
    images[0].words[0x1f0u/4u]=1u;failed=false;
    try{(void)manager.RenderFrame(0xabcdefu);}catch(const std::logic_error&){failed=true;}
    Check(failed,"active unowned render cannot silently succeed");manager.Destroy();
}

}  // namespace

int main(const int argc, char** argv) {
    try {
        VerifyRenderTraversal();
        pal4::NativeMonsterManagerOwner manager;
        const auto address = manager.Construct();
        VerifyNativeGridSecondarySnapshot(manager, Check);
        Check(address != 0u && address == manager.Address() &&
                  manager.InfoCount() == 0u && manager.GridAddress() == 0u,
              "native monster manager constructs its empty information tree");
        Check(manager.InitializeScene({}) && manager.GridAddress() != 0u,
              "empty monster scene initializes the original grid");
        Check(manager.SetPursuitTargetAll(0x12340000u) != 0u,
              "empty monster scene accepts the current player target");
        Check(manager.ProcessCollisions(0.1f) != 0u,
              "empty monster scene completes collision traversal");
        Check(manager.ClearStopPursuitAll() != 0u,
              "empty monster scene completes stop-pursuit reset");
        std::uint32_t visited = 0u;
        manager.VisitObjects([&](std::uint32_t) {
            ++visited;
            return true;
        });
        Check(visited == 0u,
              "empty monster scene callback visits no object");
        const auto* grid = reinterpret_cast<const pal4::SpatialGridState*>(
            static_cast<std::uintptr_t>(manager.GridAddress()));
        Check(grid->dimensions == std::array<std::uint32_t, 3>{20u, 1u, 20u} &&
                  grid->buckets != 0u && manager.FindInfo("absent") == 0u,
              "monster grid preserves original X20/Y1/Z20 dimensions");
        Check(manager.InitializeScene({}) && manager.InfoCount() == 0u &&
                  manager.GridAddress() != 0u,
              "empty monster scene can reinitialize without stale state");
        Check(manager.RegisterConstructedObject("monster-b", 0x12340000u) &&
                  manager.RegisterConstructedObject("monster-a", 0x12340001u) &&
                  manager.RegisterConstructedObject("monster-c", 0x12340002u) &&
                  manager.ObjectCount() == 3u,
              "monster object tree registers three constructed objects");
        Check(manager.FindObject("monster-a") == 0x12340001u &&
                  manager.FindObject("monster-b") == 0x12340000u &&
                  manager.FindObject("monster-c") == 0x12340002u &&
                  manager.FindObject("missing") == 0u &&
                  !manager.RegisterConstructedObject("monster-b", 0x12340003u) &&
                  manager.ObjectCount() == 3u,
              "monster object tree finds exact names and rejects duplicates");
        std::array<std::uint32_t, 3> objects{};
        manager.VisitObjects([&](const std::uint32_t object) {
            objects[visited++] = object;
            return true;
        });
        Check(visited == 3u &&
                  objects == std::array<std::uint32_t, 3>{
                      0x12340001u, 0x12340000u, 0x12340002u},
              "monster object traversal follows sorted names");
        manager.Destroy();
        Check(manager.Address() == 0u && manager.GridAddress() == 0u &&
                  manager.OutstandingAllocations() == 0u,
              "monster manager releases tree, grid and all owned allocations");

        std::array<pal4::MonsterBehaviorManagerState, 3> reset_states{};
        Check(manager.Construct() != 0u, "construct stop-pursuit reset fixture");
        const std::array<const char*, 3> reset_names{"c", "a", "b"};
        for (std::size_t index = 0; index < reset_states.size(); ++index) {
            auto& words = reset_states[index].words;
            words.fill(0x13572468u);
            // The original clear walk includes inactive objects too.
            words[0x1f0u / 4u] = index == 1u ? 0u : 1u;
            words[0x704u / 4u] = static_cast<std::uint32_t>(index + 1u);
            Check(manager.RegisterConstructedObject(reset_names[index],
                      reinterpret_cast<std::uint32_t>(&reset_states[index])),
                  "register stop-pursuit state in name tree");
        }
        (void)manager.ClearStopPursuitAll();
        for (std::size_t index = 0; index < reset_states.size(); ++index) {
            const auto& words = reset_states[index].words;
            for (std::size_t word = 0; word < words.size(); ++word) {
                const auto expected = word == 0x704u / 4u ? 0u :
                    word == 0x1f0u / 4u ? (index == 1u ? 0u : 1u) :
                    0x13572468u;
                Check(words[word] == expected,
                      "reset clears only stop-pursuit word on every tree object");
            }
        }
        manager.Destroy();
        Check(manager.OutstandingAllocations() == 0u,
              "stop-pursuit reset fixture releases manager storage");
        (void)manager.Construct();
        for (std::size_t index=0; index<reset_states.size(); ++index) {
            reset_states[index] = {};
            Check(manager.RegisterConstructedObject(reset_names[index],
                      reinterpret_cast<std::uint32_t>(&reset_states[index])), "register idle stop fixture");
        }
        (void)manager.StopPursuitAll(0.0f);
        for (const auto& state : reset_states)
            Check(state.words[0x704u/4u] == 1u, "locking player pauses every idle monster");
        reset_states[1].words[0x6e0u/4u] = 1u;
        bool missing_return = false;
        try { (void)manager.StopPursuitAll(0.0f); }
        catch (const std::logic_error&) { missing_return = true; }
        Check(missing_return, "active pursuit cannot silently skip its required return path");
        manager.Destroy();
        Check(manager.OutstandingAllocations() == 0u, "stop fixture teardown releases all storage");

        if (argc == 2) {
            pal4::NativeMonsterInfoReader reader;
            const auto root = std::filesystem::path(argv[1]);
            Check(reader.Load(root, "Q01", "N01") &&
                      manager.Construct() != 0u &&
                      manager.InitializeScene(reader.Records()) &&
                      manager.InfoCount() == 0u,
                  "original Q01/N01 monster stage initializes empty manager");
            Check(manager.SetPursuitTargetAll(0x12340000u) != 0u,
                  "original Q01/N01 manager processes the player target");
            manager.Destroy();
            Check(manager.OutstandingAllocations() == 0u,
                  "original empty monster stage tears down cleanly");

            Check(reader.Load(root, "Q01", "Q01") &&
                      reader.Records().empty(),
                  "original Q01/Q01 monster scene is empty");

            Check(reader.Load(root, "M06", "3A") &&
                      reader.Records().size() == 3u &&
                      manager.Construct() != 0u &&
                      manager.LoadInfo(reader.Records()) &&
                      manager.InfoCount() == 3u,
                  "original M06/3A records enter native monster info tree");
            for (const auto& record : reader.Records()) {
                const auto info = manager.FindInfo(record.name);
                Check(info != 0u &&
                          *reinterpret_cast<const std::uint32_t*>(
                              static_cast<std::uintptr_t>(info + 64u)) ==
                              record.values.size() &&
                          manager.InfoDocument(
                              *reinterpret_cast<const std::uint32_t*>(
                                  static_cast<std::uintptr_t>(info + 72u))) !=
                              nullptr,
                      "native monster info tree retains arrays and XND");
            }
            Check(manager.FindInfo("missing") == 0u,
                  "monster info tree rejects absent names");
            bool needs_entity_binding = false;
            try {
                (void)manager.InitializeScene(reader.Records());
            } catch (const std::logic_error&) {
                needs_entity_binding = true;
            }
            Check(needs_entity_binding,
                  "nonempty monster scene exposes the entity creation boundary");
            std::uint32_t destroyed = 0u;
            std::uint32_t resets = 0u;
            std::uint32_t entries = 0u;
            std::uint32_t selections = 0u;
            std::uint32_t next_object = 0x12340000u;
            Check(manager.InitializeScene(
                      reader.Records(),
                      [&](const pal4::NativeMonsterInfoRecord& record) {
                          auto owner = std::make_unique<CountedOwner>(destroyed);
                          Check(manager.FindInfo(record.name) != 0u &&
                                    manager.RegisterOwnedObject(
                                        record.name, ++next_object,
                                        std::move(owner), [&] {
                                            ++resets;
                                            return 17u;
                                        }, [&](std::uint32_t target) {
                                            Check(target == 0x8877u,
                                                  "owned pursuit entry target");
                                            ++entries;
                                            return 23u;
                                        }, [&](std::string_view action) {
                                            ++selections;
                                            return action == "C01";
                                        }),
                                "scene callback registers an owned monster");
                      }) &&
                      manager.ObjectCount() == 3u &&
                      manager.FindObject("m06-02-05") != 0u &&
                      manager.FindObject("m06-02-07") != 0u &&
                      manager.FindObject("m06-05-02") != 0u,
                  "M06/3A scene callback materializes all three name-tree entries");
            const auto reset_object = manager.FindObject("m06-02-05");
            bool missing_component = false;
            try {
                (void)manager.OwnedComponent(reset_object);
            } catch (const std::logic_error&) {
                missing_component = true;
            }
            Check(missing_component,
                  "generic owned registration does not invent a native monster component");
            Check(manager.ResetOwnedPursuit(reset_object) == 17u && resets == 1u,
                  "monster manager dispatches reset to the object's owned service");
            Check(manager.EnterOwnedPursuit(reset_object, 0x8877u) == 23u &&
                      entries == 1u,
                  "monster manager dispatches pursuit entry to the owned service");
            Check(manager.SelectOwnedAnimation(reset_object, "C01") &&
                      !manager.SelectOwnedAnimation(reset_object, "absent") &&
                      selections == 2u,
                  "owned animation selector preserves success and missing-action results");
            const auto retained_identity = manager.Address();
            const auto retained_header = *reinterpret_cast<const std::uint32_t*>(retained_identity + 0x2cu);
            bool missing_notify = false;
            try { manager.CleanupScene({}); }
            catch (const std::logic_error&) { missing_notify = true; }
            Check(missing_notify && destroyed == 0u && manager.ObjectCount() == 3u,
                  "monster cleanup requires a real notification service before deleting objects");
            unsigned notifications = 0u;
            manager.CleanupScene([&](std::uint32_t object) {
                Check(destroyed == notifications && object != 0u,
                      "each monster is notified before its owner is deleted");
                ++notifications;
            });
            const auto retained_allocations = manager.OutstandingAllocations();
            manager.CleanupScene({});
            Check(manager.Address() == retained_identity &&
                      *reinterpret_cast<const std::uint32_t*>(retained_identity + 0x2cu) == retained_header &&
                      manager.OutstandingAllocations() == retained_allocations && notifications == 3u &&
                      manager.ProcessCollisions(0.1f) != 0u,
                  "empty monster manager survives repeated cleanup and Scene collision traversal");
            Check(manager.InitializeScene({}) && destroyed == 3u &&
                      manager.ObjectCount() == 0u &&
                      manager.InfoCount() == 0u,
                  "scene reload destroys owned monsters before clearing names");
            bool rejected_stale_reset = false;
            try {
                (void)manager.ResetOwnedPursuit(reset_object);
            } catch (const std::invalid_argument&) {
                rejected_stale_reset = true;
            }
            Check(rejected_stale_reset && resets == 1u,
                  "monster scene reload discards reset callbacks with their owners");
            bool rejected_stale_entry = false;
            try {
                (void)manager.EnterOwnedPursuit(reset_object, 0x8877u);
            } catch (const std::invalid_argument&) {
                rejected_stale_entry = true;
            }
            Check(rejected_stale_entry && entries == 1u,
                  "monster scene reload discards entry callbacks with their owners");
            bool rejected_stale_selection = false;
            try {
                (void)manager.SelectOwnedAnimation(reset_object, "C01");
            } catch (const std::invalid_argument&) {
                rejected_stale_selection = true;
            }
            Check(rejected_stale_selection && selections == 2u,
                  "scene reload discards animation selectors with their owners");
            bool rejected_stale_component = false;
            try {
                (void)manager.OwnedComponent(reset_object);
            } catch (const std::invalid_argument&) {
                rejected_stale_component = true;
            }
            Check(rejected_stale_component,
                  "scene reload rejects component access for a removed object");
            bool interrupted = false;
            try {
                (void)manager.InitializeScene(
                    reader.Records(),
                    [&](const pal4::NativeMonsterInfoRecord& record) {
                        if (manager.ObjectCount() != 0u)
                            throw std::runtime_error("simulated second monster failure");
                        auto owner = std::make_unique<CountedOwner>(destroyed);
                        Check(manager.RegisterOwnedObject(
                                  record.name, ++next_object,
                                  std::move(owner)),
                              "partial scene registers its first monster");
                    });
            } catch (const std::runtime_error&) {
                interrupted = true;
            }
            Check(interrupted && manager.ObjectCount() == 1u,
                  "interrupted scene retains its first owned monster");
            manager.Destroy();
            Check(destroyed == 4u && manager.OutstandingAllocations() == 0u,
                  "interrupted monster scene releases entity, tree and grid");
        }
        return EXIT_SUCCESS;
    } catch (const std::exception& error) {
        std::fprintf(stderr, "FAIL: unexpected exception: %s\n", error.what());
        return EXIT_FAILURE;
    }
}
