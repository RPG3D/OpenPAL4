#include "pal4/native_game_arena_manager_owner.h"
#include "pal4/native_event_document_runtime.h"
#include "pal4/fog_config.h"
#include "pal4/game_arena_release.h"

#include <algorithm>
#include <array>
#include <bit>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <stdexcept>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

std::uint32_t Read(std::uint32_t address) {
    return *reinterpret_cast<const std::uint32_t*>(
        static_cast<std::uintptr_t>(address));
}

void EmptyArenaLifecycle() {
    pal4::NativeGameArenaManagerOwner arena;
    const auto image = arena.Construct();
    Check(image != 0u && arena.Address() == image &&
              arena.ReadyFlag() == 1u &&
              Read(image) ==
                  pal4::GameArenaManagerLifecycle::kVtableAddress &&
              Read(image + 320u) == 0x84a004u &&
              Read(image + 408u) == 0xffffffffu &&
              Read(image + 416u) == 0xffffffffu &&
              Read(image + 448u + 8u) != 0u,
          "native arena owns original manager, config, names and cache");
    for (const auto offset : {200u, 212u, 224u, 240u, 252u,
                              508u, 520u, 544u, 556u, 580u, 592u})
        Check(Read(image + offset) != 0u &&
                  Read(image + offset + 4u) == 0u,
              "native arena owns all eleven empty string buffers");
    for (const auto offset : {28u, 56u, 76u, 96u, 116u, 136u})
        Check(Read(image + offset) == 0u &&
                  Read(image + offset + 4u) == 0u &&
                  Read(image + offset + 8u) == 0x3f800000u,
              "native arena initializes each world dictionary schema");
    Check(arena.OutstandingAllocations() == 14u,
          "arena owns image, eleven texts and two tree nodes");
    arena.SetSceneName("N01");
    Check(arena.SceneName() == "N01" && Read(image + 224u + 4u) == 3u &&
              arena.OutstandingAllocations() == 14u,
          "scene runtime name occupies the original owned string buffer");
    arena.SetSceneName("N01_scene_name_longer_than_initial_capacity");
    Check(arena.SceneName() ==
              "N01_scene_name_longer_than_initial_capacity" &&
              arena.OutstandingAllocations() == 14u,
          "scene name growth replaces its owned allocation");
    arena.SetSceneName("N01");
    Check(arena.SceneName() == "N01",
          "scene name can be reassigned after growth");
    bool duplicate_rejected = false;
    try {
        (void)arena.Construct();
    } catch (const std::logic_error&) {
        duplicate_rejected = true;
    }
    Check(duplicate_rejected, "arena owner rejects duplicate construction");
    arena.Destroy();
    Check(arena.Address() == 0u &&
              arena.OutstandingAllocations() == 0u,
          "empty arena cleanup releases every owned allocation");
    arena.Destroy();
}

void EmptyAllocatedRenderQueue() {
    pal4::NativeGameArenaManagerOwner arena;
    const auto image = arena.Construct();
    const auto buffer = arena.AllocateQueueBytes(16u);
    auto* queue = reinterpret_cast<std::uint32_t*>(
        static_cast<std::uintptr_t>(image + 300u));
    queue[1] = buffer;
    queue[2] = buffer;
    queue[3] = buffer + 16u;
    Check(arena.OutstandingAllocations() == 15u,
          "empty render queue retains its allocated capacity");
    arena.Destroy();
    Check(arena.OutstandingAllocations() == 0u,
          "arena releases an allocated render queue with no pending Atomics");
}

void ActiveSceneObjectUsesArenaStorage() {
    pal4::NativeGameArenaManagerOwner arena;
    const auto image = arena.Construct();
    std::array<std::uint32_t, 98u> object{};
    constexpr char name[] = "Q01_object";
    object[1u] = static_cast<std::uint32_t>(
        reinterpret_cast<std::uintptr_t>(name));
    object[0x184u / 4u] = 1u;
    const auto address = static_cast<std::uint32_t>(
        reinterpret_cast<std::uintptr_t>(object.data()));
    Check(arena.AddActiveSceneObject(address, 1.0f) == 1u,
          "last GOB use registers an active scene object");
    const auto begin = Read(image + 0x26cu);
    Check(begin != 0u && Read(image + 0x270u) == begin + 12u &&
              Read(begin) == address &&
              Read(begin + 4u) == std::bit_cast<std::uint32_t>(1.0f) &&
              arena.AddActiveSceneObject(address, 1.0f) == 0u,
          "arena active scene object retains context without duplicates");
    const auto allocations = arena.OutstandingAllocations();
    arena.ClearActiveSceneRecords();
    Check(Read(image + 0x270u) == begin && Read(image + 0x26cu) == begin &&
              arena.OutstandingAllocations() == allocations,
          "scene release retires borrowed object records and preserves capacity");
    // The old object is no longer valid: name lookup must never visit it.
    object[1u] = 1u;
    std::array<std::uint32_t, 98u> replacement{};
    replacement[1u] = static_cast<std::uint32_t>(
        reinterpret_cast<std::uintptr_t>(name));
    replacement[0x184u / 4u] = 1u;
    Check(arena.AddActiveSceneObject(static_cast<std::uint32_t>(
              reinterpret_cast<std::uintptr_t>(replacement.data())), 2.0f) == 1u &&
              Read(image + 0x26cu) == begin &&
              arena.OutstandingAllocations() == allocations,
          "next scene registers the same name without visiting the retired object");
    arena.ClearActiveSceneRecords();
    arena.ClearActiveSceneRecords();
    arena.Destroy();
    Check(arena.OutstandingAllocations() == 0u,
          "arena releases active scene object storage");
}

void AllActiveRangesRetireBeforeOwners() {
    pal4::NativeGameArenaManagerOwner arena;
    const auto image = arena.Construct();
    for (const auto offset : {616u, 632u, 648u, 664u, 680u}) {
        const auto buffer = arena.AllocateQueueBytes(24u);
        auto* range = reinterpret_cast<std::uint32_t*>(
            static_cast<std::uintptr_t>(image + offset));
        range[1] = buffer;
        range[2] = buffer + 24u;
        range[3] = buffer + 24u;
        // Deliberately invalid borrowed pointers: clearing must not visit them.
        auto* records = reinterpret_cast<std::uint32_t*>(buffer);
        records[0] = records[3] = 1u;
    }
    const auto allocations = arena.OutstandingAllocations();
    arena.ClearActiveSceneRecords();
    for (const auto offset : {616u, 632u, 648u, 664u, 680u})
        Check(Read(image + offset + 4u) == Read(image + offset + 8u) &&
                  Read(image + offset + 12u) == Read(image + offset + 4u) + 24u,
              "all five original ranges are empty and keep allocated capacity");
    Check(arena.OutstandingAllocations() == allocations,
          "retiring active ranges does not destroy borrowed owners");
    arena.Destroy();
    Check(arena.OutstandingAllocations() == 0u,
          "arena destruction frees all five retained buffers");
}

void SceneScriptNamesOwnTheirStorage() {
    pal4::NativeGameArenaManagerOwner arena;
    const auto image=arena.Construct();
    auto* words=reinterpret_cast<std::uint32_t*>(image);
    char world[]="Q01";words[2]=reinterpret_cast<std::uint32_t>(world);
    arena.SetSceneName("N01");
    const auto allocations=arena.OutstandingAllocations();
    const auto queue=[&](const pal4::GameWorldSceneScriptRequest& request){
        Check(request.csb_mode && request.file_path=="gamedata\\EditData\\script\\Q01.csb" &&
            request.script_path=="PAL4-script:[Q01_N01]" && request.function_name=="Q01_N01_init" &&
            request.function_path=="PAL4-script:[Q01_N01][Q01_N01_init]" &&
            request.read_callback==0x600d50u && Read(image+236u)==0u &&
            arena.SceneScriptSource()==request.file_path,
            "original scene names are owned and published before deferred SDK queue");
        return true;
    };
    Check(arena.QueueSceneScript(queue) && Read(image+236u)==1u,
        "successful scene stack preparation sets original queued flag");
    world[0]='X';
    Check(arena.SceneScriptSource()=="gamedata\\EditData\\script\\Q01.csb",
        "published countdown source does not borrow loaded world name");
    world[0]='Q';arena.SetSceneName("Q01");
    Check(!arena.QueueSceneScript([&](const auto& request){
        Check(request.function_name=="Q01_Q01_init" && request.script_path=="PAL4-script:[Q01_Q01]",
            "scene replacement refreshes the module and init function");
        return false;
    }) && Read(image+236u)==0u,"duplicate scene module leaves queued flag reset");
    bool failed{};
    try{(void)arena.QueueSceneScript([](const auto&)->bool{throw std::runtime_error("queue failure");});}
    catch(const std::runtime_error&){failed=true;}
    Check(failed && Read(image+236u)==0u && arena.OutstandingAllocations()==allocations,
        "failed stack preparation retains owned names without marking success or leaking allocations");
    words[2]=0u;arena.Destroy();
    Check(arena.OutstandingAllocations()==0u,"scene source and module storage release with arena");
}

void PreviousSceneNamesOwnTheirStorage() {
    pal4::NativeGameArenaManagerOwner arena;
    const auto image = arena.Construct();
    const auto allocations = arena.OutstandingAllocations();
    auto* words = reinterpret_cast<std::uint32_t*>(image);
    arena.CaptureSceneNames();
    Check(arena.PreviousWorldName().empty() && arena.PreviousAreaName().empty(),
          "unloaded arena preserves empty previous names");
    char world[] = "WORLD_NAME_LONGER_THAN_THE_INITIAL_THIRTY_TWO_BYTE_BUFFER";
    char area[] = "AREA_NAME_LONGER_THAN_THE_INITIAL_THIRTY_TWO_BYTE_BUFFER";
    words[2] = reinterpret_cast<std::uint32_t>(world);
    words[3] = reinterpret_cast<std::uint32_t>(area);
    words[12] = 1u;
    arena.CaptureSceneNames();
    Check(arena.PreviousWorldName() == world && arena.PreviousAreaName() == area &&
              Read(image + 240u) != words[2] && Read(image + 252u) != words[3] &&
              arena.OutstandingAllocations() == allocations,
          "previous names grow independent buffers without leaking old allocations");
    world[0] = 'X';
    area[0] = 'Y';
    words[2] = words[3] = words[12] = 0u;
    arena.CaptureSceneNames();
    Check(arena.PreviousWorldName().starts_with("WORLD_") &&
              arena.PreviousAreaName().starts_with("AREA_"),
          "previous names survive source mutation and unloaded repeated capture");
    char next_world[] = "Q01";
    char next_area[] = "N01";
    words[2] = reinterpret_cast<std::uint32_t>(next_world);
    words[3] = reinterpret_cast<std::uint32_t>(next_area);
    words[12] = 1u;
    arena.CaptureSceneNames();
    Check(arena.PreviousWorldName() == "Q01" && arena.PreviousAreaName() == "N01" &&
              arena.OutstandingAllocations() == allocations,
          "second capture replaces both logical lengths without new allocations");
    words[2] = words[3] = words[12] = 0u;
    arena.Destroy();
    Check(arena.OutstandingAllocations() == 0u,
          "arena destruction releases independent previous names");
}

struct SequenceDocuments final : pal4::IArenaSequenceRuntime {
    pal4::NativeEventDocumentRuntime& documents;
    unsigned destroyed{};
    explicit SequenceDocuments(pal4::NativeEventDocumentRuntime& value) : documents(value) {}
    std::uint8_t ReadByte(std::uint32_t) override { throw std::logic_error("unexpected payload memory read"); }
    void WriteByte(std::uint32_t, std::uint8_t) override { throw std::logic_error("unexpected payload memory write"); }
    std::uint32_t ReadWord(std::uint32_t) override { throw std::logic_error("unexpected payload memory read"); }
    void WriteWord(std::uint32_t, std::uint32_t) override { throw std::logic_error("unexpected payload memory write"); }
    std::uint32_t Allocate(std::uint32_t) override { throw std::logic_error("unexpected payload allocation"); }
    std::uint32_t Free(std::uint32_t) override { throw std::logic_error("unexpected payload free"); }
    std::uint32_t Call(pal4::ArenaSequenceCall operation,
        std::initializer_list<std::uint32_t> arguments) override {
        Check(operation == pal4::ArenaSequenceCall::DestroyXmlNode && arguments.size() == 1u,
              "sequence payload adapter receives only owned XML documents");
        documents.DestroyDocument(*arguments.begin());
        (void)documents.Free(*arguments.begin());
        ++destroyed;
        return 0u;
    }
};

void SequenceResetDestroysNativeXmlAndStorage() {
    pal4::NativeGameArenaManagerOwner arena;
    const auto image = arena.Construct();
    const auto baseline = arena.OutstandingAllocations();
    auto* sequence = reinterpret_cast<std::uint32_t*>(image + 448u);
    auto* head = reinterpret_cast<std::uint32_t*>(sequence[2]);
    const auto nil = head[1];
    pal4::NativeBaseObjectMemory xml_memory;
    pal4::NativeEventDocumentRuntime documents(xml_memory);
    SequenceDocuments payloads(documents);
    std::array<std::uint32_t, 3u> nodes{};
    for (unsigned i = 0u; i < nodes.size(); ++i) {
        nodes[i] = arena.AllocateQueueBytes(36u);
        auto* node = reinterpret_cast<std::uint32_t*>(nodes[i]);
        std::fill_n(node, 9u, 0u);
        const auto text = arena.AllocateQueueBytes(3u);
        auto* bytes = reinterpret_cast<char*>(text);
        bytes[0] = 0; bytes[1] = static_cast<char>('A' + i); bytes[2] = 0;
        node[4] = text + 1u; node[5] = node[6] = 1u;
        node[7] = documents.Allocate(60u);
        (void)documents.InitializeDocument(node[7], "sequence", 0u);
        documents.WriteIntAttribute(node[7], "count", static_cast<std::int32_t>(i + 1u));
        node[8] = 1u;
    }
    for (unsigned i = 0u; i < nodes.size(); ++i) {
        auto* node = reinterpret_cast<std::uint32_t*>(nodes[i]);
        node[0] = i == 1u ? nodes[0] : nil;
        node[1] = i == 1u ? sequence[2] : nodes[1];
        node[2] = i == 1u ? nodes[2] : nil;
    }
    head[0] = nodes[0]; head[1] = nodes[1]; head[2] = nodes[2];
    sequence[4] = 3u;
    const auto capacity = arena.AllocateQueueBytes(8u);
    sequence[6] = sequence[7] = capacity; sequence[8] = capacity + 8u;
    const auto path = arena.AllocateQueueBytes(5u);
    auto* path_bytes = reinterpret_cast<char*>(path);
    path_bytes[0] = 0; path_bytes[1] = 'Q'; path_bytes[2] = '0'; path_bytes[3] = '1'; path_bytes[4] = 0;
    sequence[10] = path + 1u; sequence[11] = sequence[12] = 3u;
    sequence[13] = 0x1234u;
    *reinterpret_cast<std::uint8_t*>(sequence) = 1u;
    bool unbound_rejected = false;
    try { arena.ResetSequenceResources(); }
    catch (const std::logic_error&) { unbound_rejected = true; }
    Check(unbound_rejected && documents.DocumentCount() == 3u && sequence[4] == 3u &&
              *reinterpret_cast<std::uint8_t*>(sequence) == 1u,
          "unbound populated sequence rejects before modifying storage");
    arena.ResetSequenceResources(&payloads);
    Check(payloads.destroyed == 3u && documents.DocumentCount() == 0u &&
              xml_memory.OutstandingAllocations() == 0u && sequence[4] == 0u &&
              head[0] == sequence[2] && head[1] == nil && head[2] == sequence[2] &&
              sequence[6] == capacity && sequence[7] == capacity &&
              sequence[8] == capacity + 8u && sequence[10] == 0u &&
              sequence[11] == 0u && sequence[12] == 0u && sequence[13] == 0u &&
              arena.OutstandingAllocations() == baseline + 1u,
          "sequence reset destroys actual native XML, keys, tree and path while retaining array capacity");
    arena.ResetSequenceResources();
    Check(payloads.destroyed == 3u, "repeated reset never destroys retired XML twice");
    arena.Destroy();
    Check(arena.OutstandingAllocations() == 0u,
          "arena destructor releases retained sequence array and sentinels");
}

void FogDefaultsFollowArenaOwnership() {
    for (const bool active : {false, true}) {
        pal4::NativeGameArenaManagerOwner arena;
        const auto image = arena.Construct();
        const auto baseline = arena.OutstandingAllocations();
        const auto allocate = [&] {
            const auto address = arena.AllocateQueueBytes(sizeof(pal4::FogConfigState));
            auto& state = *reinterpret_cast<pal4::FogConfigState*>(address);
            (void)pal4::FogConfig::Initialize(state);
            state.active = active;
            state.enabled = 1u;
            state.word4 = 0x12345678u;
            state.float16 = 42.0f;
            *reinterpret_cast<std::uint32_t*>(image + 504u) = address;
        };
        allocate();
        Check(arena.OutstandingAllocations() == baseline + 1u,
              "fog storage uses the arena allocator");
        const auto neighbor = Read(image + 508u);
        arena.ReleaseFogDefaults();
        Check(!Read(image + 504u) && Read(image + 508u) == neighbor &&
                  arena.OutstandingAllocations() == baseline,
              "scene fog release clears its pointer without touching the cache text");
        arena.ReleaseFogDefaults();
        allocate();
        arena.Destroy();
        Check(!arena.Address() && arena.OutstandingAllocations() == 0u,
              "manager destructor resets and frees remaining active or inactive fog");
        arena.ReleaseFogDefaults();
    }
}

void FogLoadFollowsArenaOwnership() {
    class File final : public pal4::IFogConfigFileRuntime {
    public:
        bool missing = false, fail = false, closed = false;
        std::size_t cursor = 0u;
        std::array<std::uint8_t, 21> bytes{};
        std::uintptr_t Open(std::string_view path, std::string_view mode) override {
            Check(path == "scene_fog" && mode == "r", "fog file request");
            cursor = 0u;
            closed = false;
            return missing ? 0u : 1u;
        }
        void Read(std::uintptr_t file, void* destination, std::uint32_t size,
                  std::uint32_t count) override {
            if (fail) throw std::runtime_error("injected fog read failure");
            Check(file == 1u && count == 1u && cursor + size <= bytes.size(),
                  "fog reads the packed 21-byte record");
            std::memcpy(destination, bytes.data() + cursor, size);
            cursor += size;
        }
        void Close(std::uintptr_t file) override {
            Check(file == 1u, "fog closes its file");
            closed = true;
        }
    } file;
    file.bytes[0] = 1u;
    const std::array<std::uint32_t, 5> values{
        0xffffffffu, 3u, 0x44fd2000u, 0x4705b500u, 0x38d1b717u};
    std::memcpy(file.bytes.data() + 1u, values.data(), 20u);
    pal4::NativeGameArenaManagerOwner arena;
    const auto image = arena.Construct();
    const auto baseline = arena.OutstandingAllocations();
    for (unsigned attempt = 0; attempt < 2u; ++attempt) {
        Check(arena.LoadFogConfig("scene_fog", file), "optional fog loads");
        const auto* fog = reinterpret_cast<const pal4::FogConfigState*>(Read(image + 504u));
        Check(fog && fog->active == 1u && fog->enabled == 1u &&
                  fog->word4 == values[0] && fog->word8 == values[1] &&
                  std::bit_cast<std::uint32_t>(fog->float12) == values[2] &&
                  std::bit_cast<std::uint32_t>(fog->float16) == values[3] &&
                  std::bit_cast<std::uint32_t>(fog->float20) == values[4] &&
                  file.closed && file.cursor == 21u &&
                  arena.OutstandingAllocations() == baseline + 1u,
              "fog replacement preserves packed values and exactly one allocation");
    }
    file.missing = true;
    Check(!arena.LoadFogConfig("scene_fog", file) && !Read(image + 504u) &&
              arena.OutstandingAllocations() == baseline,
          "missing optional fog retires previous scene fog");
    file.missing = false;
    file.fail = true;
    bool failed = false;
    try { (void)arena.LoadFogConfig("scene_fog", file); }
    catch (const std::runtime_error&) { failed = true; }
    Check(failed && !Read(image + 504u) &&
              arena.OutstandingAllocations() == baseline,
          "fog read failure rolls back published arena storage");
    arena.Destroy();
    Check(arena.OutstandingAllocations() == 0u, "fog load leaves no arena leaks");
}

void ScopedReleaseBinding() {
    pal4::NativeGameArenaManagerOwner arena;
    const auto image = arena.Construct();
    class Runtime final : public pal4::IGameArenaReleaseRuntime {
    public:
        explicit Runtime(pal4::NativeGameArenaManagerOwner& owner):owner_(owner) {}
        bool fail = false, recurse = false, rejected = false;
        unsigned reads = 0u;
        std::uint32_t ReadWord(std::uint32_t address) override {
            ++reads;
            Check(address == owner_.Address()+316u || address == owner_.Address()+48u,
                  "empty native release reads actual archive then world gate");
            if (fail) throw std::runtime_error("injected release read failure");
            if (recurse) {
                recurse = false;
                try { (void)owner_.ReleaseScene(*this); }
                catch (const std::logic_error&) { rejected = true; }
            }
            return Read(address);
        }
        void WriteWord(std::uint32_t,std::uint32_t) override { Unexpected(); }
        std::string_view ReadCString(std::uint32_t) override { Unexpected(); }
        void StoreText(std::uint32_t,std::string_view) override { Unexpected(); }
        std::uint32_t Call(pal4::GameArenaReleaseCall,std::uint32_t,std::uint32_t,std::uint32_t) override { Unexpected(); }
    private:
        [[noreturn]] static void Unexpected() { throw std::logic_error("empty release must not reach loaded services"); }
        pal4::NativeGameArenaManagerOwner& owner_;
    } runtime(arena);
    runtime.fail = true;
    bool failed = false;
    try { (void)arena.ReleaseScene(runtime); }
    catch (const std::runtime_error& error) {
        failed = std::string_view(error.what()) == "injected release read failure";
    }
    Check(failed,"scoped native release propagates original failure");
    runtime.fail = false;
    runtime.recurse = true;
    Check(!arena.ReleaseScene(runtime) && runtime.rejected && runtime.reads == 3u,
          "failed binding is restored and recursive release rejects without disturbing outer scope");
    Check(!arena.ReleaseScene(runtime) && runtime.reads == 5u,
          "successful binding is restored for a second empty release");
    arena.StorePreviousSceneText(image+240u,"previous-world-owned-by-arena");
    Check(arena.PreviousWorldName()=="previous-world-owned-by-arena",
          "shared release previous name uses native arena text ownership");
    bool wrong_field=false;
    try { arena.StorePreviousSceneText(image+224u,"wrong"); }
    catch(const std::invalid_argument&) { wrong_field=true; }
    Check(wrong_field,"release text writer rejects unrelated scene name field");
    arena.Destroy();
    Check(arena.OutstandingAllocations()==0u,"scoped release retains no runtime or text allocation");
}

}  // namespace

int main() {
    try {
        EmptyArenaLifecycle();
        EmptyAllocatedRenderQueue();
        ActiveSceneObjectUsesArenaStorage();
        AllActiveRangesRetireBeforeOwners();
        PreviousSceneNamesOwnTheirStorage();
        SceneScriptNamesOwnTheirStorage();
        SequenceResetDestroysNativeXmlAndStorage();
        FogDefaultsFollowArenaOwnership();
        FogLoadFollowsArenaOwnership();
        ScopedReleaseBinding();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
