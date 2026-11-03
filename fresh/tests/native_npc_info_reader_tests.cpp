#include "pal4/native_npc_info_reader.h"
#include "pal4/native_npc_manager_owner.h"
#include "pal4/spatial_grid.h"
#include "pal4/string_buffer.h"
#include "support/grid_secondary_snapshot_probe.h"

#include <bit>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <exception>
#include <stdexcept>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::fprintf(stderr, "FAIL: %s\n", message);
        std::exit(EXIT_FAILURE);
    }
}

// An empty object tree must never read the separately owned object memory.
class NoObjects final : public pal4::IStringBufferStorageRuntime {
public:
    std::uint32_t Allocate(std::uint32_t) override { throw std::logic_error("unused allocation"); }
    std::uint32_t Free(std::uint32_t) override { throw std::logic_error("unused free"); }
    std::uint32_t ReadDword(std::uint32_t) override { throw std::logic_error("unused object read"); }
    void WriteDword(std::uint32_t, std::uint32_t) override { throw std::logic_error("unused write"); }
    void WriteByte(std::uint32_t, std::uint8_t) override { throw std::logic_error("unused byte"); }
};

class GridElements final : public pal4::INativeNpcGridElements {
public:
    struct UpdateCall {
        std::uint32_t element, context, argument;
        bool operator==(const UpdateCall&) const = default;
    };
    void Update(std::uint32_t element, std::uint32_t context,
                std::uint32_t argument) override {
        updates.push_back({element, context, argument});
    }
    void Release(std::uint32_t element) override { releases.push_back(element); }
    std::vector<UpdateCall> updates;
    std::vector<std::uint32_t> releases;
};

void VerifyGridElementBoundary() {
    pal4::NativeNpcManagerOwner manager;
    const auto owner = manager.Construct();
    bool rejected = false;
    try { (void)manager.AddGridElement(1u); }
    catch (const std::logic_error&) { rejected = true; }
    Check(rejected, "unbound grid insertion fails before registering helpers");
    auto services = std::make_shared<GridElements>();
    std::weak_ptr<GridElements> lifetime = services;
    manager.BindGridElements(services);
    Check(!manager.AddGridElement(1u), "grid must exist before adding elements");
    Check(manager.InitializeSceneGrid() != 0u && manager.GridWrapperAddress() != 0u &&
              *reinterpret_cast<const std::uint32_t*>(manager.GridWrapperAddress()) == 0x84b6b4u,
          "grid wrapper exposed separately from spatial grid");
    // Pointers belong to a different owner, never to the manager allocator.
    std::uint32_t helpers[3]{};
    const auto pointer = [&](unsigned i) {
        return static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(&helpers[i]));
    };
    Check(!manager.AddGridElement(0u) && manager.AddGridElement(pointer(0)) &&
              manager.AddGridElement(pointer(1)) && manager.AddGridElement(pointer(2)),
          "shared grid pointer vector accepts external helpers and grows");
    // NPC allocator intentionally rejects unowned memory. Borrow actual owned
    // pointer slots for this synthetic read fixture; do not relax that boundary.
    VerifyNativeGridSecondarySnapshot(manager, Check, true);
    rejected = false;
    try { manager.BindGridElements({}); }
    catch (const std::logic_error&) { rejected = true; }
    Check(rejected, "live elements retain their dispatch services");
    rejected = false;
    try { (void)manager.InitializeSceneGrid(); }
    catch (const std::logic_error&) { rejected = true; }
    Check(rejected, "live element associations prevent grid replacement");
    NoObjects objects;
    const auto frame = [&] {
        (void)manager.ProcessWorldObjects(0.125f, 7u, objects,
            [](std::uint32_t, float, std::uint32_t) {
                throw std::logic_error("empty object tree dispatched");
            });
    };
    frame();
    const auto elapsed = std::bit_cast<std::uint32_t>(0.125f);
    Check(services->updates == std::vector<GridElements::UpdateCall>{
              {pointer(0),owner + 0x48u,elapsed}, {pointer(1),owner + 0x48u,elapsed},
              {pointer(2),owner + 0x48u,elapsed}},
          "world frame forwards original secondary context and elapsed bits in order");
    manager.SetGridUpdating(true);
    frame();
    Check(services->updates.size() == 3u, "original set flag suppresses grid dispatch");
    manager.SetGridUpdating(false);
    Check(manager.RemoveGridElement(pointer(1)) && !manager.RemoveGridElement(pointer(1)) &&
              services->releases == std::vector<std::uint32_t>{pointer(1)},
          "middle removal shifts vector and releases exactly once");
    services->updates.clear();
    frame();
    Check(services->updates.size() == 2u && services->updates[0].element == pointer(0) &&
              services->updates[1].element == pointer(2),
          "removed helper never receives subsequent world updates");
    services.reset();
    Check(!lifetime.expired(), "manager strongly retains element services");
    manager.CleanupScene({}, {});
    Check(lifetime.lock()->releases == std::vector<std::uint32_t>{pointer(1),pointer(0),pointer(2)} &&
              manager.GridWrapperAddress() == 0u,
          "scene cleanup releases remaining helpers before destroying grid");
    manager.CleanupScene({}, {});
    Check(lifetime.lock()->releases.size() == 3u, "repeated scene cleanup does not double release");
    manager.BindGridElements({});
    Check(lifetime.expired(), "empty grid permits service retirement");
    manager.Destroy();
    Check(manager.OutstandingAllocations() == 0u, "grid boundary leaves no manager allocations");
}

}  // namespace

int RunTests(const int argc, char** argv) {
    VerifyGridElementBoundary();
    pal4::NativeNpcInfoReader reader;
    pal4::NativeNpcInfoRecord blank;
    Check(!blank.DefaultAction(),
          "NPC without cache attributes has no default action");
    pal4::BinaryXmlNode cache_root;
    cache_root.name = "NPCINFO_BufferCache";
    cache_root.children.push_back({"NPCINFO_BufferCache_Attr", {}, {}});
    const auto cache_bytes = pal4::BinaryXmlConfig::Serialize(cache_root);
    Check(cache_bytes && blank.buffer_cache.Parse(*cache_bytes) &&
              blank.DefaultAction() == "C01",
          "NPC cache attribute node uses the original C01 fallback");
    Check(!reader.Parse({}) && reader.Records().empty() &&
              reader.Status()==pal4::NativeNpcInfoReader::LoadStatus::InvalidData,
          "truncated NPC header is rejected");
    const std::vector<std::uint8_t> empty{0u, 0u, 0u, 0u};
    Check(reader.Parse(empty) && reader.Records().empty() &&
              reader.Status()==pal4::NativeNpcInfoReader::LoadStatus::Ready,
          "empty NPC file has no records");
    const std::vector<std::uint8_t> invalid_count{1u, 0u, 0u, 0u};
    Check(!reader.Parse(invalid_count) && reader.Records().empty(),
          "record count must fit the file");
    pal4::NativeNpcManagerOwner manager;
    const auto address = manager.Construct();
    Check(address != 0u && address == manager.Address() &&
              manager.OutstandingAllocations() == 6u,
          "native NPC manager owns both empty trees and its string buffer");
    const auto grid = manager.InitializeSceneGrid();
    const auto& image = *reinterpret_cast<const pal4::SpatialGridState*>(
        static_cast<std::uintptr_t>(grid));
    Check(grid != 0u && grid == manager.GridAddress() &&
              image.dimensions == std::array<std::uint32_t, 3>{20u, 1u, 20u} &&
              image.buckets != 0u &&
              manager.OutstandingAllocations() == 9u,
          "native NPC scene grid owns original 400 buckets");
    Check(manager.InitializeSceneGrid() != 0u &&
              manager.OutstandingAllocations() == 9u,
          "NPC scene grid replacement releases its previous allocation");
    const auto grid_flag = [&] {
        return *reinterpret_cast<const std::uint8_t*>(
            static_cast<std::uintptr_t>(manager.Address() + 56u + 37u));
    };
    manager.SetGridUpdating(true);
    Check(grid_flag() == 1u, "NPC owner sets the grid update flag");
    manager.SetGridUpdating(false);
    Check(grid_flag() == 0u, "Scene entry clears the NPC grid update flag");
    pal4::NativeNpcInfoRecord sample;
    sample.name = "TEST_NPC";
    sample.resource = "201J";
    sample.position_bits = {1u, 2u, 3u};
    sample.rotation_bits = {4u, 5u, 6u};
    sample.flags = {7u, 8u, 9u};
    Check(sample.buffer_cache.Parse(*cache_bytes) &&
              manager.LoadInfo(std::span(&sample, 1u)) &&
              manager.InfoCount() == 1u,
          "decoded NPC record enters the native info tree");
    const auto info = manager.FindInfo("TEST_NPC");
    const auto* words = reinterpret_cast<const std::uint32_t*>(
        static_cast<std::uintptr_t>(info));
    Check(info != 0u && manager.FindInfo("MISSING") == 0u &&
              words[9u] == 1u && words[11u] == 3u &&
              words[12u] == 4u && words[14u] == 6u &&
              words[15u] == 7u && words[16u] == 8u && words[17u] == 9u &&
              manager.InfoDocument(words[20u]) != nullptr &&
              manager.InfoDocument(words[20u])->Root().name ==
                  "NPCINFO_BufferCache",
          "native NPC record retains transforms, flags and XND document");
    Check(manager.LoadInfo(std::span(&sample, 1u)) &&
              manager.InfoCount() == 1u &&
              manager.FindInfo("TEST_NPC") != 0u,
          "reloading NPC info replaces the previous tree");
    const std::array<pal4::NativeNpcInfoRecord, 2u> duplicates{sample, sample};
    Check(!manager.LoadInfo(duplicates) && manager.InfoCount() == 0u &&
              manager.FindInfo("TEST_NPC") == 0u,
          "duplicate NPC names leave the manager tree empty");
    Check(manager.RegisterConstructedObject("MN001", 0x1000u) &&
              manager.RegisterConstructedObject("MZ999", 0x2000u) &&
              manager.RegisterConstructedObject("MA000", 0x3000u) &&
              manager.RegisterConstructedObject("MN002", 0x4000u) &&
              manager.ObjectCount() == 4u &&
              manager.FindObject("MN001") == 0x1000u &&
              manager.FindObject("MZ999") == 0x2000u &&
              manager.FindObject("MA000") == 0x3000u &&
              manager.FindObject("MN002") == 0x4000u &&
              manager.FindObject("MISSING") == 0u,
          "native NPC object tree inserts and searches rotated nodes");
    Check(!manager.RegisterConstructedObject("MN001", 0x5000u) &&
              manager.ObjectCount() == 4u &&
              manager.FindObject("MN001") == 0x1000u,
          "NPC object tree rejects duplicate names");
    std::vector<std::uint32_t> visited;
    manager.ForEachObject([&](const std::uint32_t object) {
        visited.push_back(object);
        return true;
    });
    Check(visited == std::vector<std::uint32_t>{0x3000u, 0x1000u,
                                                  0x4000u, 0x2000u},
          "NPC owner visits its registered objects in tree order");
    visited.clear();
    manager.ForEachObject([&](const std::uint32_t object) {
        visited.push_back(object);
        return false;
    });
    Check(visited == std::vector<std::uint32_t>{0x3000u},
          "NPC owner stops when callback returns zero");
    bool missing_services = false;
    Check(manager.LoadInfo(std::span(&sample,1u)), "cleanup fixture reloads actual NPC info");
    try { manager.CleanupScene({},{}); }
    catch (const std::logic_error&) { missing_services = true; }
    Check(missing_services && manager.ObjectCount() == 4u && manager.InfoCount() == 1u,
          "populated NPC cleanup rejects missing object services before mutation");
    std::vector<std::uint32_t> cleanup_order;
    const auto header = *reinterpret_cast<const std::uint32_t*>(address + 0x2cu);
    const auto nil = manager.TreeGlobals().nil_address;
    manager.CleanupScene([&](std::uint32_t object) {
        cleanup_order.push_back(object);
    }, [&](std::uint32_t object) { cleanup_order.push_back(object | 1u); });
    Check(cleanup_order == std::vector<std::uint32_t>{0x3000u,0x3001u,0x1000u,0x1001u,0x4000u,0x4001u,0x2000u,0x2001u} &&
              manager.Address() == address && manager.ObjectCount() == 0u && manager.InfoCount() == 0u &&
              *reinterpret_cast<const std::uint32_t*>(address + 0x2cu) == header &&
              manager.TreeGlobals().nil_address == nil && !manager.FindObject("MN001"),
          "shared NPC cleanup orders notify/delete and retains manager and sentinels");
    const auto retained = manager.OutstandingAllocations();
    manager.CleanupScene({},{});
    Check(manager.OutstandingAllocations() == retained && manager.InitializeSceneGrid() &&
              manager.LoadInfo(std::span(&sample,1u)) &&
              manager.RegisterConstructedObject("again",0x5000u) && manager.FindObject("again") == 0x5000u,
          "NPC cleanup repeats and permits a subsequent scene using the same manager");
    manager.CleanupScene([](std::uint32_t) {}, [](std::uint32_t) {});
    manager.Destroy();
    Check(manager.Address() == 0u && manager.GridAddress() == 0u &&
              manager.FindObject("MN001") == 0u &&
              manager.OutstandingAllocations() == 0u,
          "NPC manager releases populated trees, grid and string storage");
    Check(manager.Construct() != 0u &&
              manager.OutstandingAllocations() == 6u,
          "native NPC manager can reconstruct after teardown");
    manager.Destroy();

    if (argc == 2) {
        const auto root = std::filesystem::path(argv[1]);
        Check(reader.Load(root, "Q01", "N01"),
              "decode original Q01/N01 NPC file");
        const auto& records = reader.Records();
        Check(records.size() == 1u && records.front().name == "MN001" &&
                  records.front().resource == "201J" &&
                  records.front().reserved.empty(),
              "original Q01/N01 NPC identity fields");
        std::printf("Q01/N01 NPC: %s / %s; XND roots: %s / %s\n",
                    records.front().name.c_str(),
                    records.front().resource.c_str(),
                    records.front().behavior.Root().name.c_str(),
                    records.front().buffer_cache.Root().name.c_str());
        Check(records.front().behavior.Root().children.empty(),
              "original MN001 behavior document has no event child");
        Check(records.front().DefaultAction() == "J01",
              "original MN001 default action is J01");
        Check(manager.Construct() != 0u && manager.LoadInfo(records) &&
                  manager.InfoCount() == 1u,
              "original Q01/N01 NPC enters the native manager tree");
        const auto original_info = manager.FindInfo("MN001");
        const auto* original_words = reinterpret_cast<const std::uint32_t*>(
            static_cast<std::uintptr_t>(original_info));
        Check(original_info != 0u &&
                  original_words[15u] == records.front().flags[0u] &&
                  manager.InfoDocument(original_words[19u]) != nullptr &&
                  manager.InfoDocument(original_words[19u])->Root().name ==
                      "NPCINFO_Behavior" &&
                  manager.InfoDocument(original_words[20u]) != nullptr &&
                  manager.InfoDocument(original_words[20u])->Root().name ==
                      "NPCINFO_BufferCache",
              "original NPC flags and both XND documents survive tree load");
        const auto behavior_document = original_words[19u];
        manager.Destroy();
        Check(manager.OutstandingAllocations() == 0u &&
                  manager.InfoDocument(behavior_document) == nullptr,
              "original NPC tree releases its records and documents");
        Check(!records.front().behavior.Root().name.empty() &&
                  !records.front().buffer_cache.Root().name.empty(),
              "original NPC behavior and cache XND documents parse");
        Check(reader.Load(root, "Q01", "Q01"),
              "decode original Q01/Q01 NPC file");
        const auto& q01_npcs = reader.Records();
        Check(q01_npcs.size() == 4u &&
                  q01_npcs[0].name == "MN001" &&
                  q01_npcs[0].resource == "408" &&
                  q01_npcs[1].name == "MN002" &&
                  q01_npcs[1].resource == "236" &&
                  q01_npcs[2].name == "MN003" &&
                  q01_npcs[2].resource == "223" &&
                  q01_npcs[3].name == "MN004" &&
                  q01_npcs[3].resource == "236J01",
              "original Q01/Q01 NPC identities");
        Check(!reader.Load(root, "M08", "3") && reader.Records().empty() &&
                  reader.Status()==pal4::NativeNpcInfoReader::LoadStatus::MissingMember,
              "original absent NPC member has explicit status and clears previous records");
        Check(reader.Load(root,"Q01","N01") && reader.Status()==pal4::NativeNpcInfoReader::LoadStatus::Ready,
              "successful read resets missing status");
        Check(!reader.Load(root / "__missing_package__","Q01","N01") && reader.Records().empty() &&
                  reader.Status()==pal4::NativeNpcInfoReader::LoadStatus::ArchiveError,
              "archive open failure is distinct from optional missing NPC member");
        Check(!reader.Load(root, "Q01/other", "N01") &&
                  reader.Records().empty() && reader.Status()==pal4::NativeNpcInfoReader::LoadStatus::InvalidRequest,
              "invalid scene request clears prior NPC records");
    }
    return EXIT_SUCCESS;
}

int main(const int argc, char** argv) {
    try {
        return RunTests(argc, argv);
    } catch (const std::exception& error) {
        std::fprintf(stderr, "FAIL: unexpected exception: %s\n", error.what());
        return EXIT_FAILURE;
    }
}
