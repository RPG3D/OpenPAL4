#include "pal4/particle_visual_object.h"

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IParticleVisualRuntime {
    pal4::ParticleVisualState* state{};
    int time_calls{};
    int seed_calls{};
    std::uint32_t seed{};

    std::uint32_t CurrentTime() override {
        ++time_calls;
        Check(state->ReadWord(424u) == 1097859072u &&
                  state->ReadWord(320u) == 0xa5a5a5a5u &&
                  state->ReadWord(212u) == 0xa5a5a5a5u,
              "time call falls between early and late field initialization");
        return 0x12345678u;
    }
    void SeedRandom(std::uint32_t value) override {
        ++seed_calls;
        seed = value;
        Check(time_calls == 1 && seed == 0x12345678u,
              "random seed receives current CRT time");
    }
};

void CheckWords(const pal4::ParticleVisualState& state,
                const std::vector<std::size_t>& offsets,
                std::uint32_t value, const char* message) {
    for (const auto offset : offsets)
        Check(state.ReadWord(offset) == value, message);
}

void TestConstructor() {
    pal4::ParticleVisualState state;
    state.bytes.fill(0xa5u);
    Runtime runtime;
    runtime.state = &state;
    Check(pal4::ParticleVisualObject::Construct(
              state, 0x5au, runtime) == &state &&
              runtime.time_calls == 1 && runtime.seed_calls == 1,
          "constructor returns its 440-byte object and seeds CRT once");
    Check(state.ReadByte(0u) == 0u &&
              state.ReadByte(1u) == 1u && state.ReadByte(2u) == 1u &&
              state.ReadByte(3u) == 0u &&
              state.ReadByte(52u) == 1u &&
              state.ReadByte(54u) == 1u &&
              state.ReadByte(60u) == 0x5au &&
              state.ReadByte(144u) == 1u &&
              state.ReadByte(156u) == 0x5au &&
              state.ReadByte(172u) == 0x5au &&
              state.ReadByte(188u) == 1u &&
              state.ReadByte(316u) == 0u &&
              state.ReadByte(384u) == 0u &&
              state.ReadByte(420u) == 0u,
          "native byte defaults and explicit stack byte match IDA");
    CheckWords(state,
               {8u, 12u, 20u, 40u, 44u, 48u, 56u,
                64u, 68u, 72u, 76u, 80u, 84u, 88u, 92u, 96u,
                112u, 116u, 120u, 124u, 148u, 152u,
                160u, 164u, 168u, 176u, 180u, 184u,
                204u, 208u, 236u, 240u, 244u, 248u,
                252u, 256u, 268u, 272u, 288u, 292u,
                300u, 304u, 308u, 312u,
                336u, 340u, 344u, 348u, 352u, 356u,
                360u, 364u, 368u, 372u, 376u, 380u,
                396u, 400u, 404u, 408u, 412u, 416u,
                428u, 432u, 436u},
               0u, "zero-initialized native words match IDA");
    CheckWords(state,
               {100u, 104u, 108u, 212u, 216u, 220u, 224u,
                228u, 232u, 276u, 284u, 296u,
                320u, 324u, 328u, 332u, 388u, 392u},
               0x3f800000u, "unit-float defaults match IDA");
    Check(state.ReadWord(192u) == 5u &&
              state.ReadWord(196u) == 2u &&
              state.ReadWord(200u) == 100u &&
              state.ReadWord(260u) == 20u &&
              state.ReadWord(264u) == 5u &&
              state.ReadWord(280u) == 0x3f000000u &&
              state.ReadWord(424u) == 1097859072u,
          "integer and half-float defaults match IDA");
    Check(state.ReadByte(6u) == 0xa5u &&
              state.ReadByte(17u) == 0xa5u &&
              state.ReadByte(24u) == 0xa5u &&
              state.ReadByte(53u) == 0xa5u &&
              state.ReadByte(61u) == 0xa5u &&
              state.ReadByte(317u) == 0xa5u &&
              state.ReadByte(385u) == 0xa5u &&
              state.ReadByte(421u) == 0xa5u,
          "unwritten bytes retain their previous contents");
}

struct LifecycleRuntime final : pal4::IParticleVisualLifecycleRuntime,
                                pal4::IParticleTankRuntime,
                                pal4::IParticleCollectionRuntime,
                                pal4::IParticleFrameTransformRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::unordered_map<std::uint32_t, std::uint8_t> collection_bytes;
    std::vector<std::string> calls;
    std::vector<std::string> tank_calls;
    std::vector<std::string> collection_calls;
    std::uint32_t next_allocation = 0x11000u;
    bool fail_first_element{};

    pal4::IParticleTankRuntime& TankRuntime() override { return *this; }
    pal4::IParticleCollectionRuntime& CollectionRuntime() override {
        calls.emplace_back("initialize_collection");
        return *this;
    }
    std::uint32_t CreateTank(std::uint32_t capacity,
                             std::uint32_t flags,
                             std::uint32_t data_flags) override {
        Check(flags == 0x100000a7u && data_flags == 1u,
              "native PTank create flags");
        tank_calls.push_back("create:" + std::to_string(capacity));
        calls.emplace_back("initialize_render");
        return 0xa000u;
    }
    std::uint32_t PluginOffset() override { return 0x10u; }
    std::uint32_t ReadWord(std::uint32_t address) override {
        tank_calls.push_back("read:" + std::to_string(address));
        const auto found = words.find(address);
        return found == words.end() ? 0u : found->second;
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        tank_calls.push_back("write:" + std::to_string(address));
        words[address] = value;
    }
    void SetMaterialTexture(std::uint32_t material,
                            std::uint32_t texture) override {
        tank_calls.push_back("texture:" + std::to_string(material) +
                             ":" + std::to_string(texture));
    }
    std::uint32_t CreateFrame() override {
        tank_calls.emplace_back("frame");
        return 0x6000u;
    }
    void SetAtomicFrame(std::uint32_t atomic,
                        std::uint32_t frame) override {
        Check(atomic == 0xa000u && frame == 0x6000u,
              "atomic frame association");
        tank_calls.emplace_back("attach");
    }
    void SetFrameIdentity(std::uint32_t) override {
        tank_calls.emplace_back("identity");
    }
    void UpdateFrameObjects(std::uint32_t) override {
        tank_calls.emplace_back("update_objects");
    }
    pal4::IParticleFrameTransformRuntime& FrameTransformRuntime() override {
        tank_calls.emplace_back("update_transform");
        return *this;
    }
    void SetIdentity(std::uint32_t) override {}
    void Scale(std::uint32_t, pal4::ParticleVisualPoint,
               std::uint32_t) override {}
    void Rotate(std::uint32_t, pal4::ParticleVisualPoint,
                float, std::uint32_t) override {}
    void Translate(std::uint32_t, pal4::ParticleVisualPoint,
                   std::uint32_t) override {}
    std::uint8_t UpdateObjects(std::uint32_t) override { return 1u; }
    std::uint32_t Allocate(std::uint32_t size) override {
        collection_calls.push_back("allocate:" + std::to_string(size));
        if (size == 52u && fail_first_element) {
            fail_first_element = false;
            return 0u;
        }
        const auto address = next_allocation;
        next_allocation += 0x100u;
        return address;
    }
    void FreeBuffer(std::uint32_t address) override {
        collection_calls.push_back("free:" + std::to_string(address));
    }
    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override {
        collection_bytes[address] = value;
    }
    void InitializeFromAsset(
        std::uint32_t element, std::uint32_t asset,
        std::array<float, 2u> scale, bool mode,
        std::uint8_t option, std::uint32_t enabled) override {
        Check(asset == 0x1234u && scale == std::array<float, 2u>{1.f, 1.f} &&
                  !mode && option == 5u && enabled == 1u,
              "collection asset initializer arguments");
        collection_calls.emplace_back("asset");
        words[element + 20u] = element + 0x80u;
    }
    void InitializeFromObject(
        std::uint32_t element, std::array<std::uint32_t, 3u> source,
        std::uint32_t asset, std::array<float, 2u> scale,
        bool mode, std::uint8_t option,
        std::uint32_t enabled) override {
        Check(source == std::array<std::uint32_t, 3u>{10u, 20u, 30u} &&
                  asset == 0x1234u &&
                  scale == std::array<float, 2u>{1.f, 1.f} &&
                  mode && option == 5u && enabled == 1u,
              "collection object initializer arguments");
        collection_calls.emplace_back("object");
        words[element + 20u] = element + 0x80u;
    }
    void AttachElementFrame(std::uint32_t frame,
                            std::uint32_t child) override {
        Check(frame == 0x6000u && child != 0u,
              "collection child frame attached to root");
        collection_calls.emplace_back("attach_child");
    }

    std::uint32_t ReadExternalWord(std::uint32_t address) override {
        calls.push_back("read:" + std::to_string(address));
        return words.at(address);
    }
    int Free(std::uint32_t address) override {
        calls.push_back("free:" + std::to_string(address));
        return -static_cast<int>(address);
    }
    void RemoveFrameChild(std::uint32_t frame) override {
        calls.push_back("remove_frame:" + std::to_string(frame));
    }
    int DestroyAtomic(std::uint32_t atomic) override {
        calls.push_back("destroy_atomic:" + std::to_string(atomic));
        return -static_cast<int>(atomic);
    }
    int DestroyFrame(std::uint32_t frame) override {
        calls.push_back("destroy_frame:" + std::to_string(frame));
        return -static_cast<int>(frame);
    }
    void DeinitializeElement(std::uint32_t element) override {
        calls.push_back("deinitialize:" + std::to_string(element));
    }
    void AddFrameChild(std::uint32_t parent,
                       std::uint32_t child) override {
        Check(parent == 0x7000u && child == 0x6000u,
              "initialized frame joins external parent");
        calls.emplace_back("add_child");
    }
};

void TestResetAndDestruct() {
    LifecycleRuntime runtime;
    pal4::ParticleVisualState state;
    state.bytes.fill(0xa5u);
    state.WriteByte(0u, 0u);
    pal4::ParticleVisualObject::Reset(state, runtime);
    Check(runtime.calls.empty() && state.ReadByte(6u) == 0xa5u,
          "inactive reset leaves all fields untouched");
    state.WriteByte(0u, 1u);
    state.WriteWord(124u, 1u);
    state.WriteWord(160u, 0x1000u);
    state.WriteWord(164u, 0x1008u);
    state.WriteWord(44u, 0x6000u);
    runtime.words[0x1000u] = 0x2000u;
    runtime.words[0x1004u] = 0x3000u;
    for (const auto item : {0x2000u, 0x3000u}) {
        runtime.words[item + 80u] = item + 0x10u;
        runtime.words[item + 84u] = item + 0x20u;
        runtime.words[item + 88u] = item + 0x30u;
    }
    runtime.words[0x6004u] = 0x7000u;
    pal4::ParticleVisualObject::Reset(state, runtime);
    const std::vector<std::string> expected{
        "read:4096", "read:8272", "free:8208",
        "read:8276", "free:8224", "read:8280", "free:8240",
        "free:8192", "read:4100", "read:12368", "free:12304",
        "read:12372", "free:12320", "read:12376", "free:12336",
        "free:12288", "read:24580", "remove_frame:24576",
        "destroy_frame:24576"};
    Check(runtime.calls == expected && state.ReadByte(0u) == 0u &&
              state.ReadWord(160u) == 0x1000u &&
              state.ReadWord(164u) == 0x1000u &&
              state.ReadWord(124u) == 0u &&
              state.ReadWord(100u) == 0x3f800000u &&
              state.ReadWord(200u) == 100u &&
              state.ReadWord(424u) == 1097859072u,
          "reset frees nested buffers, detaches frame and restores defaults");
    CheckWords(state,
               {88u, 92u, 96u, 112u, 116u, 120u,
                204u, 208u, 148u, 152u, 268u, 272u,
                288u, 236u, 240u, 292u, 300u, 304u,
                308u, 244u, 312u, 248u, 252u, 256u,
                336u, 340u, 344u, 348u, 352u, 356u,
                360u, 364u, 368u, 372u, 376u, 380u,
                396u, 400u, 404u, 408u, 428u, 432u,
                436u, 124u, 48u, 76u, 80u, 84u},
               0u, "reset clears all native scalar and array defaults");
    CheckWords(state,
               {100u, 104u, 108u, 212u, 216u, 220u,
                224u, 228u, 232u, 276u, 284u, 296u,
                320u, 324u, 328u, 332u, 388u, 392u},
               0x3f800000u, "reset restores native unit floats");

    runtime.calls.clear();
    state.WriteWord(176u, 0x8000u);
    state.WriteWord(180u, 0x8004u);
    state.WriteWord(184u, 0x8008u);
    state.WriteWord(160u, 0x9000u);
    state.WriteWord(164u, 0x9004u);
    state.WriteWord(168u, 0x9008u);
    state.WriteWord(64u, 0xa000u);
    state.WriteWord(68u, 0xa004u);
    state.WriteWord(72u, 0xa008u);
    Check(pal4::ParticleVisualObject::Destruct(state, runtime) ==
              -0xa000 &&
              runtime.calls == std::vector<std::string>{
                  "free:32768", "free:36864", "free:40960"} &&
              state.ReadWord(176u) == 0u &&
              state.ReadWord(160u) == 0u &&
              state.ReadWord(64u) == 0u &&
              state.ReadWord(72u) == 0u,
          "destructor frees three owned arrays and clears their triples");
}

void TestResetTypeBranches() {
    LifecycleRuntime runtime;
    pal4::ParticleVisualState state;
    state.WriteByte(0u, 1u);
    pal4::ParticleVisualObject::Reset(state, runtime);
    Check(runtime.calls.empty(),
          "type zero with no atomic or frame has no native destroy calls");
    runtime.calls.clear();
    state.WriteByte(0u, 1u);
    state.WriteWord(124u, 2u);
    pal4::ParticleVisualObject::Reset(state, runtime);
    Check(runtime.calls.empty() && state.ReadByte(0u) == 0u &&
              state.ReadWord(124u) == 0u,
          "unknown type skips both native destruction branches");
}

void TestDestroyObjectBranches() {
    LifecycleRuntime runtime;
    pal4::ParticleVisualState state;
    Check(pal4::ParticleVisualObject::DestroyAtomicAndFrame(
              state, runtime) == 0 && runtime.calls.empty(),
          "empty atomic/frame destroy returns zero");
    state.WriteWord(40u, 0x5000u);
    state.WriteWord(44u, 0x6000u);
    Check(pal4::ParticleVisualObject::DestroyAtomicAndFrame(
              state, runtime) == -0x6000 &&
              runtime.calls == std::vector<std::string>{
                  "destroy_atomic:20480", "destroy_frame:24576"} &&
              state.ReadWord(40u) == 0u && state.ReadWord(44u) == 0u,
          "atomic is destroyed before frame and both fields clear");

    runtime.calls.clear();
    state.WriteWord(176u, 0x1000u);
    state.WriteWord(180u, 0x100cu);
    state.WriteWord(184u, 0x1020u);
    state.WriteWord(44u, 0x4000u);
    runtime.words[0x1000u] = 0x2000u;
    runtime.words[0x1004u] = 0u;
    runtime.words[0x1008u] = 0x3000u;
    Check(pal4::ParticleVisualObject::DestroyCollectionElements(
              state, runtime) == -0x4000 &&
              runtime.calls == std::vector<std::string>{
                  "read:4096", "deinitialize:8192", "free:8192",
                  "read:4100", "read:4104", "deinitialize:12288",
                  "free:12288", "destroy_frame:16384"} &&
              state.ReadWord(176u) == 0x1000u &&
              state.ReadWord(180u) == 0x1000u &&
              state.ReadWord(184u) == 0x1020u &&
              state.ReadWord(44u) == 0u,
          "collection teardown skips null items and retains array storage");
    runtime.calls.clear();
    Check(pal4::ParticleVisualObject::DestroyCollectionElements(
              state, runtime) == 0 && runtime.calls.empty(),
          "empty collection with no frame returns zero");
}

void TestInitializeAndProperties() {
    LifecycleRuntime runtime;
    pal4::ParticleVisualState state;
    Check(pal4::ParticleVisualObject::SetProperties(state, 0u) == 0u &&
              state.ReadWord(192u) == 5u &&
              state.ReadByte(188u) == 1u &&
              state.ReadWord(196u) == 6u &&
              pal4::ParticleVisualObject::SetProperties(state, 9u) == 9u &&
              state.ReadWord(196u) == 2u,
          "property setter selects state 2 or 6 and returns input byte");
    for (std::uint32_t type = 0u; type != 3u; ++type) {
        runtime.calls.clear();
        state.WriteByte(0u, 0u);
        state.WriteWord(124u, type);
        const auto result = pal4::ParticleVisualObject::Initialize(
            state, 7u, 0x1111u, 0x2222u, 0x3333u,
            0x4444u, 0x5555u, 8u, runtime);
        Check(result == (type < 2u ? 1u : 0u) &&
                  state.ReadByte(0u) == 1u &&
                  state.ReadByte(52u) == 7u &&
                  state.ReadByte(144u) == 8u &&
                  state.ReadWord(48u) == 0x1111u &&
                  state.ReadWord(128u) == 0x2222u &&
                  state.ReadWord(132u) == 0x3333u &&
                  state.ReadWord(136u) == 0x4444u &&
                  state.ReadWord(140u) == 0x5555u &&
                  runtime.calls == (type == 0u
                      ? std::vector<std::string>{"initialize_render"}
                      : type == 1u
                          ? std::vector<std::string>{"initialize_collection"}
                          : std::vector<std::string>{}),
              "initializer writes exact arguments and dispatches by type");
    }
}

void TestInitializeWithFrame() {
    LifecycleRuntime runtime;
    runtime.words[0x5004u] = 0x7000u;
    pal4::ParticleVisualState state;
    for (std::uint32_t type = 0u; type != 3u; ++type) {
        runtime.calls.clear();
        state.WriteByte(0u, 0u);
        state.WriteWord(124u, type);
        state.WriteWord(44u, 0x6000u);
        const auto result = pal4::ParticleVisualObject::InitializeWithFrame(
            state, 6u, 0x1111u, 0x5000u, 7u, runtime);
        Check(result == (type < 2u ? 1u : 0u) &&
                  state.ReadByte(0u) == result &&
                  state.ReadByte(52u) == 6u &&
                  state.ReadByte(55u) == 1u &&
                  state.ReadByte(144u) == 7u &&
                  state.ReadWord(48u) == 0x1111u &&
                  state.ReadWord(56u) == 0x5000u &&
                  runtime.calls == (type == 0u
                      ? std::vector<std::string>{
                            "initialize_render", "read:20484", "add_child"}
                      : type == 1u
                          ? std::vector<std::string>{
                                "initialize_collection", "read:20484", "add_child"}
                          : std::vector<std::string>{}),
              "frame initializer attaches successful child and clears invalid type");
    }
}

void TestRenderInitialization() {
    LifecycleRuntime runtime;
    pal4::ParticleVisualState state;
    state.WriteWord(200u, 25u);
    state.WriteWord(48u, 0x1234u);
    state.WriteByte(188u, 7u);
    state.WriteWord(192u, 5u);
    state.WriteWord(196u, 6u);
    runtime.words[0xa010u] = 0xb000u;
    runtime.words[0xb040u] = 0x12u;
    runtime.words[0xa018u] = 0xc000u;
    runtime.words[0xc020u] = 0xd000u;
    runtime.words[0xd000u] = 0xe000u;
    Check(pal4::ParticleVisualObject::InitializeRender(
              state, runtime) == 1u &&
              state.ReadWord(40u) == 0xa000u &&
              state.ReadWord(44u) == 0x6000u &&
              runtime.words[0xb040u] == 0x10800012u &&
              runtime.words[0xb004u] == 0u &&
              runtime.words[0xb0acu] == 7u &&
              runtime.words[0xb0a4u] == 5u &&
              runtime.words[0xb0a8u] == 6u,
          "PTank initialization writes plugin and object fields");
    Check(runtime.tank_calls == std::vector<std::string>{
              "create:25", "read:40976", "read:45120",
              "write:45120", "write:45060", "read:40984",
              "read:49184", "read:53248", "texture:57344:4660",
              "frame", "attach", "identity", "update_objects",
              "write:45228", "write:45220", "write:45224",
              "read:45120", "write:45120", "update_transform"},
          "PTank calls and plugin writes follow IDA order");
}

void TestCollectionInitialization() {
    {
        LifecycleRuntime runtime;
        pal4::ParticleVisualState state;
        state.WriteWord(200u, 0xffffffffu);
        Check(pal4::ParticleVisualObject::InitializeCollection(
                  state, runtime) == 1u &&
                  state.ReadWord(44u) == 0x6000u &&
                  runtime.collection_calls.empty() &&
                  runtime.tank_calls.back() == "update_transform",
              "nonpositive signed count only initializes collection frame");
    }
    {
        LifecycleRuntime runtime;
        pal4::ParticleVisualState state;
        state.WriteWord(200u, 4u);
        state.WriteWord(196u, 6u);
        state.WriteWord(48u, 0x1234u);
        state.WriteByte(52u, 1u);
        state.WriteByte(53u, 1u);
        state.WriteByte(144u, 5u);
        Check(pal4::ParticleVisualObject::InitializeCollection(
                  state, runtime) == 1u &&
                  state.ReadWord(176u) == 0x11500u &&
                  state.ReadWord(180u) == 0x11510u &&
                  state.ReadWord(184u) == 0x11510u,
              "four element collection doubles pointer capacity");
        Check(runtime.words[0x11500u] == 0x11000u &&
                  runtime.words[0x11504u] == 0x11200u &&
                  runtime.words[0x11508u] == 0x11400u &&
                  runtime.words[0x1150cu] == 0x11600u &&
                  runtime.collection_bytes[0x1100eu] == 1u &&
                  runtime.words[0x11014u] == 0x11080u,
              "element defaults, child frames and pointer order");
        Check(runtime.collection_calls == std::vector<std::string>{
                  "allocate:52", "asset", "attach_child",
                  "allocate:4", "free:0", "allocate:52",
                  "asset", "attach_child", "allocate:8",
                  "free:69888", "allocate:52", "asset",
                  "attach_child", "allocate:16", "free:70400",
                  "allocate:52", "asset", "attach_child"},
              "collection allocation growth and reuse order");
    }
    {
        LifecycleRuntime runtime;
        pal4::ParticleVisualState state;
        state.WriteWord(200u, 1u);
        state.WriteWord(196u, 2u);
        state.WriteWord(48u, 0x1234u);
        state.WriteByte(144u, 5u);
        state.WriteWord(112u, 10u);
        state.WriteWord(116u, 20u);
        state.WriteWord(120u, 30u);
        Check(pal4::ParticleVisualObject::InitializeCollection(
                  state, runtime) == 1u &&
                  runtime.words[0x11000u] == 10u &&
                  runtime.words[0x11004u] == 20u &&
                  runtime.words[0x11008u] == 30u &&
                  runtime.collection_calls == std::vector<std::string>{
                      "allocate:52", "object", "allocate:4", "free:0"},
              "object initialization copies source triple without child");
    }
    {
        LifecycleRuntime runtime;
        pal4::ParticleVisualState state;
        runtime.fail_first_element = true;
        state.WriteWord(200u, 1u);
        state.WriteWord(196u, 6u);
        state.WriteWord(48u, 0x1234u);
        state.WriteByte(52u, 1u);
        state.WriteByte(144u, 5u);
        Check(pal4::ParticleVisualObject::InitializeCollection(
                  state, runtime) == 1u &&
                  runtime.collection_bytes.size() == 1u &&
                  runtime.collection_bytes.at(14u) == 0u &&
                  runtime.words.at(state.ReadWord(176u)) == 0u,
              "null element skips defaults but is still initialized and inserted");
    }
}

struct UpdateRuntime final : pal4::IParticleVisualUpdateRuntime,
                             pal4::IParticleSkinRuntime,
                             pal4::IParticleTankUpdateRuntime,
                             pal4::IParticleVisualRenderRuntime,
                             pal4::IParticleEmissionRuntime,
                             pal4::IParticleRenderStateRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::unordered_map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> calls;
    bool shrink_collection_on_render{};
    pal4::IParticleSkinRuntime& SkinRuntime() override {
        calls.emplace_back("skin");
        return *this;
    }
    pal4::IParticleTankUpdateRuntime& TankUpdateRuntime() override {
        return *this;
    }
    pal4::IParticleVisualRenderRuntime& VisualRenderRuntime() override {
        calls.emplace_back("render1");
        return *this;
    }
    pal4::IParticleEmissionRuntime& EmissionRuntime() override {
        calls.emplace_back("update");
        return *this;
    }
    pal4::IParticleRenderStateRuntime& RenderStateRuntime() override {
        calls.emplace_back("event");
        return *this;
    }
    std::uint32_t GetState(std::uint32_t) override { return 0u; }
    int SetState(std::uint32_t, std::uint32_t) override { return 42; }
    void RenderAtomic(std::uint32_t) override {}
    std::int32_t Random() override { return 0; }
    std::uint32_t Allocate(std::uint32_t) override { return 0xa000u; }
    void FreeBuffer(std::uint32_t) override {}
    pal4::ParticleVisualPoint Normalize(
        pal4::ParticleVisualPoint point) override { return point; }
    std::uint32_t PluginOffset() override { return 0x10u; }
    std::uint32_t SourceGeometry(std::uint32_t) override {
        return 0x6000u;
    }
    std::uint32_t Skin(std::uint32_t) override { return 0u; }
    int MorphTargetCount(std::uint32_t) override { return 2; }
    std::uint32_t VertexCount(std::uint32_t) override { return 0u; }
    std::uint32_t Hierarchy(std::uint32_t) override { return 0u; }
    pal4::ParticleVisualPoint GeometryVertex(
        std::uint32_t, std::uint32_t) override { return {}; }
    std::array<std::uint8_t, 4u> BoneIndices(
        std::uint32_t, std::uint32_t) override { return {}; }
    std::array<float, 4u> BoneWeights(
        std::uint32_t, std::uint32_t) override { return {}; }
    pal4::ParticleVisualPoint TransformBoneVertex(
        std::uint32_t, std::uint32_t, std::uint8_t,
        pal4::ParticleVisualPoint vertex) override { return vertex; }
    pal4::ParticleVisualPoint TransformFrameVertex(
        std::uint32_t, pal4::ParticleVisualPoint vertex) override {
        return vertex;
    }
    std::uint32_t AllocatePointBuffer(std::uint32_t) override { return 0u; }
    pal4::ParticleVisualPoint ReadPointBuffer(std::uint32_t) override {
        return {};
    }
    void WritePointBuffer(std::uint32_t,
                          pal4::ParticleVisualPoint) override {}
    void FreePointBuffer(std::uint32_t) override {}
    void PrepareRender(double value) override {
        Check(value == 1.5, "render preparation receives original double");
        calls.emplace_back("prepare");
    }
    pal4::ParticleTankLock Lock(std::uint32_t, std::uint32_t,
                                std::uint32_t) override {
        return {0x10000u, 16u};
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        const auto found = words.find(address);
        return found == words.end() ? 0u : found->second;
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        return bytes.at(address);
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
    }
    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override {
        bytes[address] = value;
    }
    std::uint32_t ObjectTransform(std::uint32_t) override {
        return 0u;
    }
    pal4::ParticleVisualPoint TransformPosition(
        std::uint32_t, pal4::ParticleVisualPoint position) override {
        return position;
    }
    void InitializeTransform(std::uint32_t) override {}
    void SetColor(std::uint32_t,
                  std::array<std::uint8_t, 4u>) override {}
    void UpdateVisual(std::uint32_t, std::uint32_t,
                      std::uint32_t, std::uint32_t) override {}
    int Unlock(std::uint32_t) override {
        calls.emplace_back("render");
        return 41;
    }
    void RenderCollectionElement(std::uint32_t element,
                                 std::uint32_t event,
                                 pal4::ParticleVisualState& state) override {
        Check(event == 0x8888u, "collection render forwards event");
        calls.push_back("element:" + std::to_string(element));
        if (shrink_collection_on_render) state.WriteWord(204u, 1u);
    }
    std::uint32_t ReadExternalWord(std::uint32_t address) override {
        calls.push_back("read_word:" + std::to_string(address));
        return words.at(address);
    }
    std::uint8_t ReadExternalByte(std::uint32_t address) override {
        calls.push_back("read_byte:" + std::to_string(address));
        return bytes.at(address);
    }
    void WriteExternalWord(std::uint32_t address,
                           std::uint32_t value) override {
        calls.push_back("write_word:" + std::to_string(address));
        words[address] = value;
    }
    void WriteExternalByte(std::uint32_t address,
                           std::uint8_t value) override {
        calls.push_back("write_byte:" + std::to_string(address));
        bytes[address] = value;
    }
};

void TestUpdateAndEvent() {
    UpdateRuntime runtime;
    pal4::ParticleVisualState state;
    Check(pal4::ParticleVisualObject::Update(
              state, 1.5, 0x7777u, false, runtime) == 0 &&
              runtime.calls.empty(),
          "inactive update returns before callbacks");
    state.WriteByte(0u, 1u);
    state.WriteByte(55u, 1u);
    state.WriteByte(1u, 1u);
    Check(pal4::ParticleVisualObject::Update(
              state, 1.5, 0x7777u, false, runtime) == 41 &&
              runtime.calls == std::vector<std::string>{
                  "skin", "update", "prepare", "render"},
          "normal update runs skin, simulation and render in order");
    runtime.calls.clear();
    Check(pal4::ParticleVisualObject::Update(
              state, 1.5, 0x7777u, true, runtime) == 41 &&
              runtime.calls == std::vector<std::string>{
                  "prepare", "render"},
          "skip flag omits skin and simulation only");
    runtime.calls.clear();
    state.WriteByte(55u, 0u);
    state.WriteByte(1u, 0u);
    Check(pal4::ParticleVisualObject::Update(
              state, 1.5, 0x7777u, false, runtime) == 41 &&
              runtime.calls == std::vector<std::string>{
                  "prepare", "render"},
          "disabled skin and simulation flags still render");
    runtime.calls.clear();
    Check(pal4::ParticleVisualObject::HandleEvent(
              state, 0x8888u, runtime) == 42 &&
              runtime.calls == std::vector<std::string>{"event"},
          "event thunk forwards result");
}

void TestDispatchUpdate() {
    UpdateRuntime runtime;
    pal4::ParticleVisualState state;
    Check(pal4::ParticleVisualObject::DispatchUpdate(
              state, 1.5, runtime) == 0 &&
              runtime.calls == std::vector<std::string>{"update"},
          "type zero dispatches first update path");
    runtime.calls.clear();
    state.WriteWord(124u, 1u);
    Check(pal4::ParticleVisualObject::DispatchUpdate(
              state, 1.5, runtime) == 0 &&
              runtime.calls == std::vector<std::string>{"update"},
          "type one dispatches second update path");
    runtime.calls.clear();
    state.WriteWord(124u, 3u);
    Check(pal4::ParticleVisualObject::DispatchUpdate(
              state, 1.5, runtime) == 2 && runtime.calls.empty(),
          "unsupported type returns type minus one");
}

void TestDispatchRenderAndEvent() {
    UpdateRuntime runtime;
    pal4::ParticleVisualState state;
    Check(pal4::ParticleVisualObject::DispatchRender(
              state, 0u, 0x3ff80000u, 0x7777u, runtime) == 41 &&
              runtime.calls == std::vector<std::string>{"render"},
          "type zero render dispatch");
    runtime.calls.clear();
    state.WriteWord(124u, 1u);
    Check(pal4::ParticleVisualObject::DispatchRender(
              state, 0u, 0x3ff80000u, 0x7777u, runtime) == 0 &&
              runtime.calls == std::vector<std::string>{"render1"},
          "type one render dispatch");
    runtime.calls.clear();
    state.WriteWord(124u, 3u);
    Check(pal4::ParticleVisualObject::DispatchRender(
              state, 0u, 0x3ff80000u, 0x7777u, runtime) == 2 &&
              pal4::ParticleVisualObject::DispatchEvent(
                  state, 0x8888u, runtime) == 2 && runtime.calls.empty(),
          "other types return type minus one");

    state.WriteWord(124u, 1u);
    state.WriteWord(176u, 0x5000u);
    state.WriteWord(204u, 2u);
    runtime.words[0x5000u] = 0x6000u;
    runtime.words[0x5004u] = 0x7000u;
    runtime.words[0x6030u] = 0x8000u;
    runtime.words[0x7030u] = 0x9000u;
    runtime.bytes[0x801fu] = 1u;
    runtime.bytes[0x901fu] = 0u;
    Check(pal4::ParticleVisualObject::DispatchEvent(
              state, 0x8888u, runtime) == 2 &&
              runtime.calls == std::vector<std::string>{
                  "read_word:20480", "read_word:24624", "read_byte:32799",
                  "element:24576", "read_word:20484", "read_word:28720",
                  "read_byte:36895"},
          "collection event renders enabled elements only");
    runtime.calls.clear();
    runtime.shrink_collection_on_render = true;
    Check(pal4::ParticleVisualObject::DispatchEvent(
              state, 0x8888u, runtime) == 1 &&
              runtime.calls == std::vector<std::string>{
                  "read_word:20480", "read_word:24624", "read_byte:32799",
                  "element:24576"},
          "collection count is reread after each element");
    runtime.calls.clear();
    state.WriteWord(204u, 0xffffffffu);
    Check(pal4::ParticleVisualObject::DispatchEvent(
              state, 0x8888u, runtime) == -1 && runtime.calls.empty(),
          "negative collection count skips traversal");
}

void TestDeactivate() {
    UpdateRuntime runtime;
    pal4::ParticleVisualState state;
    Check(pal4::ParticleVisualObject::Deactivate(
              state, runtime) == 0 && runtime.calls.empty(),
          "inactive object skips deactivation");
    state.WriteByte(0u, 1u);
    state.WriteByte(55u, 1u);
    Check(pal4::ParticleVisualObject::Deactivate(
              state, runtime) == 1 && runtime.calls.empty(),
          "skin mode skips deactivation");
    state.WriteByte(55u, 0u);
    state.WriteByte(16u, 1u);
    state.WriteByte(5u, 1u);
    state.WriteWord(20u, 33u);
    state.WriteWord(204u, 2u);
    state.WriteWord(208u, 2u);
    state.WriteWord(160u, 0x1000u);
    state.WriteWord(148u, 0u);
    state.WriteWord(152u, 0x1234u);
    runtime.words[0x1000u] = 0x2000u;
    runtime.words[0x1004u] = 0x3000u;
    runtime.bytes[0x2000u] = 1u;
    runtime.bytes[0x3000u] = 0u;
    Check(pal4::ParticleVisualObject::Deactivate(
              state, runtime) == 2 &&
              state.ReadByte(0u) == 1u &&
              state.ReadByte(16u) == 0u &&
              state.ReadByte(5u) == 0u &&
              state.ReadWord(20u) == 0u &&
              state.ReadWord(208u) == 0u &&
              state.ReadByte(1u) == 1u &&
              state.ReadByte(3u) == 0u &&
              state.ReadWord(148u) == 0x1234u &&
              runtime.words[0x2004u] == 0u &&
              runtime.bytes[0x2000u] == 0u &&
              runtime.words[0x2028u] == 0u,
          "deactivation clears active items and restores parent state");
    Check(runtime.calls == std::vector<std::string>{
              "read_word:4096", "read_byte:8192",
              "write_word:8196", "write_byte:8192",
              "write_word:8232", "read_word:4100",
              "read_byte:12288"},
          "item reads and writes follow native loop order");
    runtime.calls.clear();
    state.WriteWord(208u, 0xffffffffu);
    Check(pal4::ParticleVisualObject::Deactivate(
              state, runtime) == -1 && runtime.calls.empty() &&
              state.ReadWord(208u) == 0xffffffffu,
          "nonpositive signed count leaves item collection untouched");
}

}  // namespace

int main() {
    TestConstructor();
    TestResetAndDestruct();
    TestResetTypeBranches();
    TestDestroyObjectBranches();
    TestInitializeAndProperties();
    TestInitializeWithFrame();
    TestRenderInitialization();
    TestCollectionInitialization();
    TestUpdateAndEvent();
    TestDispatchUpdate();
    TestDispatchRenderAndEvent();
    TestDeactivate();
}
