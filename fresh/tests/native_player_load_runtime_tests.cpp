#include "pal4/native_skill_system_owner.h"
#include "pal4/native_player_combat_events.h"
#include "pal4/native_player_load_runtime.h"
#include "pal4/native_player_collection_values.h"
#include "pal4/native_player_slot_collection_runtime.h"
#include "pal4/native_game_save_load.h"
#include "pal4/native_game_save_write.h"
#include "pal4/native_scene_record_catalog.h"
#include "pal4/native_game_data_persistence_runtime.h"
#include "pal4/native_pal_game_iv_texture_storage.h"
#include "pal4/dialog_manager_lifecycle.h"
#include "pal4/native_inventory_session.h"
#include "pal4/native_inventory_load_runtime.h"
#include "pal4/native_game_database.h"
#include "pal4/native_game_timer_queue.h"
#include "pal4/native_player_manager_load_runtime.h"
#include "pal4/native_player_manager_save_runtime.h"
#include "pal4/player_manager_operations.h"
#include "pal4/game_resource_manager.h"
#include "pal4/native_arena_cache_owner.h"
#include "pal4/native_arena_config_load_runtime.h"
#include "pal4/native_arena_scene_load_services.h"
#include "pal4/native_camera_manager_owner.h"
#include "pal4/native_actor_control_config_load_runtime.h"
#include "pal4/native_actor_control_config_save_runtime.h"
#include "pal4/pal_actor_control_config_initializer.h"
#include "pal4/native_base_object_memory.h"
#include "pal4/native_game_save_archive_reader.h"
#include "pal4/native_host_server_save_runtime.h"
#include "pal4/native_player_basic_save_runtime.h"
#include "pal4/native_player_slot_copy_runtime.h"
#include "pal4/native_player_script_property_runtime.h"
#include "pal4/native_player_field_schema.h"
#include "pal4/native_player_collection_script_runtime.h"
#include "pal4/native_player_script_refresh_runtime.h"
#include "pal4/native_player_skill_runtime.h"
#include "pal4/native_player_skill_callbacks.h"
#include "pal4/native_skill_record_catalog.h"
#include "pal4/random_number_generator.h"
#include "pal4/player_manager_object.h"
#include "pal4/player_manager_slot.h"
#include "pal4/player_manager_slot_collection_processor.h"
#include "pal4/player_manager_slot_script_data.h"
#include "pal4/script_process_and_cleanup.h"

#include <windows.h>
#include <array>
#include <bit>
#include <cmath>
#include <cstring>
#include <fstream>
#include <functional>
#include <iostream>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <vector>

namespace {
void Check(bool value, const char* message) { if (!value) throw std::runtime_error(message); }
std::uint32_t Address(const void* p) { return static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(p)); }
auto Root() {
    return std::filesystem::path("H:/PAL4/objs/player-load") / std::to_string(GetCurrentProcessId());
}
void Fixture(std::span<const std::byte> data) {
    std::filesystem::create_directories(Root() / "save");
    std::ofstream file(Root() / "save/0.dat", std::ios::binary);
    file.write(reinterpret_cast<const char*>(data.data()), data.size());
    Check(static_cast<bool>(file), "write player fixture");
}
// Observes scene service boundaries; it does not load a RenderWare world.
struct ArenaServices : pal4::INativeArenaSceneLoadServices {
    std::array<std::uint32_t, 112> image{};
    pal4::NativeBaseObjectMemory memory;
    std::vector<std::string> calls;
    std::string world, area, diff;
    bool fail_runtime{};
    std::function<void()> after_initialize_players;
    std::uint32_t pending{};
    void LoadSceneResources(std::string_view w, std::string_view a) override {
        Check(image[49] == 0u && image[111] == 1u, "arena loading flags precede scene resources");
        calls.push_back("resources"); world = w; area = a;
        image[2u]=Address(world.c_str());image[3u]=Address(area.c_str());
    }
    void LoadSceneRuntime(std::string_view d) override {
        calls.push_back("runtime"); diff = d;
        image[56u]=Address(diff.c_str());
        if (fail_runtime) throw std::runtime_error("scene service failure");
    }
    void InitializePlayerScene() override {
        calls.push_back("players"); image[59] = pending;
        if (after_initialize_players) after_initialize_players();
    }
    bool Load(pal4::NativeGameSaveArchiveReader& input) {
        return pal4::NativeArenaConfigLoadRuntime::Deserialize(Address(image.data()), memory, input, *this);
    }
};
// Matrix/SDK resource boundary double. Actual owner, mode selection and
// parameter setters execute; this does not prove RenderWare camera visuals.
struct CameraResources : pal4::INativeCameraObjectResources {
    std::uint32_t updates{};
    pal4::CameraRenderRect RenderRect() override { return {0, 0, 800, 600}; }
    void LogInitialization(bool) override {}
    bool Initialize(pal4::CameraSystemObjectState&, pal4::CameraDataState& data,
                    const pal4::CameraSetupParameters&) override { data.At(0u) = 0x9000u; return true; }
    void Destroy(pal4::CameraSystemObjectState&, pal4::CameraDataState&) noexcept override {}
    void SetFarClip(std::uint32_t, float) override {}
    float MinimumDistance() override { return 1.0f; }
    float MaximumDistance() override { return 5000.0f; }
    std::uint32_t UpdateMatrix(pal4::CameraDataState&, std::uint32_t) override { return ++updates; }
};
struct ControlServices : pal4::IActorControlConfigInitRuntime {
    std::vector<std::string> calls;
    bool active = true, throw_movement = false;
    std::uint32_t ReadWord(std::uint32_t a) override { Check(a == 0x31f0u, "actor active field"); return active; }
    std::uint32_t MinimapManager() override { return 5u; }
    void AddIcon(std::uint32_t m, std::string_view n, std::uint32_t i, std::string_view t,
                 std::uint32_t f, std::uint32_t s) override {
        Check(m == 5u && n == "LeadingRole" && i == 0u && t == "actor.png" && !f && !s, "loaded role icon arguments");
        calls.push_back("icon");
    }
    void SetActiveIcon(std::uint32_t m, std::string_view n, std::uint32_t i) override {
        Check(m == 5u && n == "LeadingRole" && !i, "loaded role active icon"); calls.push_back("active");
    }
    void UpdateGrid(std::uint32_t m, std::uint32_t p, std::uint32_t d) override {
        Check(m == 5u && p == 0x30acu && d == 0x30a0u, "loaded role grid arguments"); calls.push_back("grid");
    }
    long double ActorScale(std::uint32_t a) override { Check(a == 0x3000u, "scale selected actor"); return 100.0L; }
    std::uint32_t MovementAdapter() override { return 6u; }
    void UpdateMovement(std::uint32_t a, std::uint32_t p, float first, float second, std::uint32_t f) override {
        Check(a == 6u && p == 0x30acu && std::abs(first - 35.0f) < 0.001f &&
            std::abs(second - 53.0f) < 0.001f && f == 1u, "loaded role movement dimensions");
        calls.push_back("movement"); if (throw_movement) throw std::runtime_error("movement service failure");
    }
    std::uint32_t FloorAttributeManager() override { return 7u; }
    void UpdateFloorAttributes(std::uint32_t m) override { Check(m == 7u, "floor owner"); calls.push_back("floor"); }
    std::uint32_t CameraSubsystem() override { return 8u; }
    std::uint32_t ActiveCamera(std::uint32_t m) override { Check(m == 8u, "camera manager"); return 0x4000u; }
    pal4::ActorControlAudioPoint ReadPoint(std::uint32_t p) override {
        Check(p == 0x4074u || p == 0x4050u, "loaded actor reads camera target/orientation");
        return p == 0x4074u ? pal4::ActorControlAudioPoint{1.0f, 2.0f, 3.0f} : pal4::ActorControlAudioPoint{4.0f, 5.0f, 6.0f};
    }
    std::uint32_t AudioManager() override { return 9u; }
    std::uint32_t AudioDriver(std::uint32_t m) override { Check(m == 9u, "audio manager"); return 10u; }
    void SetAudioPosition(std::uint32_t d, const pal4::ActorControlAudioPoint& p) override {
        Check(d == 10u && p == pal4::ActorControlAudioPoint{1.0f, 2.0f, -3.0f}, "listener position flips Z"); calls.push_back("audio-position");
    }
    void SetAudioOrientation(std::uint32_t d, const pal4::ActorControlAudioPoint& p,
                             const pal4::ActorControlAudioPoint& up) override {
        Check(d == 10u && p == pal4::ActorControlAudioPoint{4.0f, 5.0f, -6.0f} &&
            up == pal4::ActorControlAudioPoint{0.0f, 1.0f, -0.0f} && std::signbit(up[2]), "listener orientation flips Z");
        calls.push_back("audio-orientation");
    }
    void ProcessInput(pal4::PalActorControlState&) override { calls.push_back("input"); }
    void ProcessInputAndClearVector(pal4::PalActorControlState&) override { calls.push_back("clear"); }
    std::uint32_t ControllerAccess() override { return 11u; }
    void ResetAndSync(std::uint32_t c, std::uint32_t f) override {
        Check(c == 11u && f == 1u, "loaded control reset flag"); calls.push_back("sync");
    }
};
void ControlOutput() {
    pal4::PalActorControlState state;
    state.words.fill(0x13579bdfu);
    state.words[1u]=0x3000u;
    pal4::NativeGameSaveArchiveBuffer output;
    Check(output.WritePrefix("CONTROL_SAVE_TEST",{}),"control output follows an existing archive prefix");
    constexpr std::array values{0u,1u,0xfffffff9u,0x80000000u};
    for(const auto value:values){
        state.words[3u]=value;
        const auto before=state;
        Check(pal4::NativeActorControlConfigSaveRuntime::Serialize(state,output)==1u &&
                  output.WriteMarker("AFTER_CONTROL") && state.words==before.words,
            "control output appends XML without changing any control word");
    }
    Fixture(output.Bytes());
    pal4::NativeGameSaveArchiveReader input;
    pal4::GameSaveHeader header;
    Check(input.OpenSlot(Root(),0) && input.ReadMarker()=="CONTROL_SAVE_TEST" && input.ReadHeader(header),
        "control roundtrip opens continuous stream");
    for(unsigned i=0;i<values.size();++i){
        const auto value=values[i];
        if(i==0u){
            pal4::BinaryXmlConfig xml;
            Check(input.ReadConfig(xml) && xml.Root().name=="Actor-Controller" &&
                      xml.MajorVersion()==1u && xml.MinorVersion()==1u && xml.Root().attributes.size()==1u,
                "control output writes original root and version1.1 with one attribute");
            const auto* flag=xml.Root().FindAttribute("m_isNeedBlendTailAngle");
            Check(flag && flag->type==1u && flag->ToInt(99u)==value,
                "control output stores integer flag without boolean conversion");
        }else{
            state.words[3u]=value^0x1234u;
            const auto before=state;
            ControlServices scene;
            Check(pal4::NativeActorControlConfigLoadRuntime::Deserialize(state,input,scene)==1u && state.words[3u]==value &&
                      scene.calls==std::vector<std::string>{"icon","active","grid","movement","floor",
                          "audio-position","audio-orientation","input","clear","sync"},
                "native saved control restores all32 flag bits and forwards actual active-actor load sequence");
            auto expected=before;expected.words[3u]=value;
            Check(state.words==expected.words,"control read changes only the stored flag with observing scene services");
        }
        Check(input.ReadMarker()=="AFTER_CONTROL","control write preserves following archive marker");
    }
    Check(!input.Remaining(),"repeated control documents end at exact EOF");input.Close();
    std::cout<<"Control archive output: nonmutating writes, signed32-bit flag, continuous documents and active-service roundtrip pass\n";
}

void ControlGenerated() {
    pal4::NativeGameSaveArchiveReader input;
    pal4::PalActorControlState state{};
    auto write = [&](pal4::BinaryXmlNode node, std::uint8_t major = 1u, bool truncated = false) {
        input.Close(); pal4::NativeGameSaveArchiveBuffer output;
        std::array<std::byte, sizeof(pal4::GameSaveHeader)> prefix{};
        auto xml = pal4::BinaryXmlConfig::Serialize(node); Check(xml.has_value(), "encode actor XML");
        (*xml)[0] = major;
        Check(output.WriteBytes(prefix) && output.WriteBytes(std::as_bytes(std::span(xml->data(), truncated ? 7u : xml->size()))) &&
            output.WriteMarker("AFTER_CONTROL"), "write actor config fixture");
        Fixture(output.Bytes()); Check(input.OpenSlot(Root(), 0) && input.ReadBytes(prefix), "actor input at nonzero cursor");
    };
    pal4::BinaryXmlNode node{"Actor-Controller"};
    node.attributes = {{"m_isNeedBlendTailAngle", 3u, std::string("-7")}};
    for (unsigned branch = 0u; branch < 3u; ++branch) {
        write(node); state.words[1u] = branch == 0u ? 0u : 0x3000u;
        ControlServices services; services.active = branch == 2u;
        Check(pal4::NativeActorControlConfigLoadRuntime::Deserialize(state, input, services) == 1u &&
            state.words[3u] == 0xfffffff9u, "actor config restores integer/text blend flag");
        auto expected = std::vector<std::string>{"input", "clear", "sync"};
        if (branch == 2u) expected = {"icon", "active", "grid", "movement", "floor", "audio-position", "audio-orientation", "input", "clear", "sync"};
        Check(services.calls == expected && input.ReadMarker() == "AFTER_CONTROL" && input.Remaining() == 0u,
            "active/null/inactive actor branches preserve service order and exact cursor");
    }
    write(pal4::BinaryXmlNode{"Actor-Controller"}); state.words[1u] = 0u;
    ControlServices missing;
    Check(pal4::NativeActorControlConfigLoadRuntime::Deserialize(state, input, missing) && state.words[3u] == 1u,
        "missing blend flag defaults to one");
    for (unsigned failure = 0u; failure < 4u; ++failure) {
        write(failure == 0u ? pal4::BinaryXmlNode{"WRONG"} : node,
              failure == 1u ? 2u : (failure == 2u ? 0u : 1u), failure == 3u);
        const auto before = state.words; ControlServices services;
        Check(!pal4::NativeActorControlConfigLoadRuntime::Deserialize(state, input, services) &&
            state.words == before && services.calls.empty(), "bad actor XML/root/version cannot finalize control");
    }
    write(node); state.words[1u] = 0x3000u; ControlServices throwing; throwing.throw_movement = true;
    bool failed = false;
    try { (void)pal4::NativeActorControlConfigLoadRuntime::Deserialize(state, input, throwing); }
    catch (const std::runtime_error&) { failed = true; }
    Check(failed && throwing.calls == std::vector<std::string>{"icon", "active", "grid", "movement"} &&
        input.ReadMarker() == "AFTER_CONTROL", "actor service exception stops finalization and retains next cursor");
    input.Close(); failed = false;
    try { (void)pal4::NativeActorControlConfigLoadRuntime::Deserialize(state, input, throwing); }
    catch (const std::invalid_argument&) { failed = true; }
    Check(failed, "closed actor input rejected");
}
void ArenaGenerated() {
    pal4::BinaryXmlNode node{"ARENA"};
    node.attributes = {{"world", 3u, std::string("Q01\0unused", 10)},
        {"area", 1u, std::int32_t{17}}, {"diff", 2u, 1.5f}};
    auto write = [&](std::string_view marker, const pal4::BinaryXmlNode& document,
                     std::uint32_t major = 1u) {
        pal4::NativeGameSaveArchiveBuffer output;
        auto xml = pal4::BinaryXmlConfig::Serialize(document);
        // The writer correctly rejects unsupported versions. Mutate the
        // serialized header to exercise malformed and legacy input versions.
        if (xml) (*xml)[0] = static_cast<std::uint8_t>(major);
        Check(xml && output.WriteMarker(marker) && output.WriteBytes(std::as_bytes(std::span(*xml))) &&
            output.WriteMarker("AFTER_ARENA"), "write arena input fixture");
        Fixture(output.Bytes());
    };
    pal4::NativeGameSaveArchiveReader input;
    for (const auto pending : {0u, 7u}) {
        write("ARENA_SERIALIZE_FLAG", node);
        ArenaServices services; services.pending = pending;
        Check(input.OpenSlot(Root(), 0) && services.Load(input), "load generated arena");
        Check(services.calls == std::vector<std::string>{"resources", "runtime", "players"} &&
            services.world == "Q01" && services.area == "17" && services.diff == "1.500000",
            "arena uses shared text conversion, CString boundary and original scene service order");
        Check(services.image[49] == (pending ? 0u : 1u) && services.image[111] == (pending ? 1u : 0u),
            "post-initialization arena pending state controls ready flags");
        Check(input.IsOpen() && input.ReadMarker() == "AFTER_ARENA" && input.Remaining() == 0u,
            "arena releases local input without closing continuous stream"); input.Close();
    }
    write("ARENA_SERIALIZE_FLAG", pal4::BinaryXmlNode{"ARENA"});
    ArenaServices missing;
    Check(input.OpenSlot(Root(), 0) && missing.Load(input) && missing.world.empty() &&
        missing.area.empty() && missing.diff.empty(), "missing scene attributes default to empty"); input.Close();
    for (unsigned failure = 0; failure < 4; ++failure) {
        write(failure == 0 ? "WRONG" : "ARENA_SERIALIZE_FLAG",
            failure == 1 ? pal4::BinaryXmlNode{"WRONG"} : node,
            failure == 2 ? 2u : (failure == 3 ? 0u : 1u));
        ArenaServices bad;
        Check(input.OpenSlot(Root(), 0) && !bad.Load(input) && bad.calls.empty() &&
            bad.image[49] == 0u && bad.image[111] == 1u && input.IsOpen(),
            "invalid arena marker/root/version never invokes scene services"); input.Close();
    }
    write("ARENA_SERIALIZE_FLAG", node);
    ArenaServices throwing; throwing.fail_runtime = true;
    Check(input.OpenSlot(Root(), 0), "open throwing arena fixture");
    bool failed = false;
    try { (void)throwing.Load(input); } catch (const std::runtime_error&) { failed = true; }
    Check(failed && throwing.calls == std::vector<std::string>{"resources", "runtime"} &&
        throwing.image[49] == 0u && throwing.image[111] == 1u && input.ReadMarker() == "AFTER_ARENA",
        "scene exception propagates without player init, false readiness or cursor loss"); input.Close();
    ArenaServices closed;
    failed = false;
    try { (void)closed.Load(input); } catch (const std::invalid_argument&) { failed = true; }
    Check(failed && closed.calls.empty(), "closed archive rejected before arena changes");
    pal4::NativeGameSaveArchiveBuffer truncated;
    std::array<std::byte, sizeof(pal4::GameSaveHeader)> padding{};
    Check(truncated.WriteBytes(padding) && truncated.WriteMarker("ARENA_SERIALIZE_FLAG"),
        "write archive prefix and arena without XML");
    Fixture(truncated.Bytes());
    ArenaServices incomplete;
    Check(input.OpenSlot(Root(), 0) && input.ReadBytes(padding) &&
        !incomplete.Load(input) && incomplete.calls.empty(),
        "truncated arena XML never invokes scene services"); input.Close();
}
// These are deliberate test service doubles. Shared post-load algorithms run,
// while game skill effects/inventory/script execution are observed at their
// service boundaries. They are never installed in the production game host.
struct Collections : pal4::PlayerManagerSlotCollectionRuntimeStub {
    pal4::NativeBaseObjectMemory& memory;
    std::vector<std::string>& trace;
    std::function<void()> inspect = [] {};
    Collections(pal4::NativeBaseObjectMemory& m, std::vector<std::string>& t) : memory(m), trace(t) {}
    std::uint32_t Allocate(std::uint32_t n) override { return memory.Allocate(n); }
    std::uint32_t Free(std::uint32_t p) override { return memory.Free(p); }
    void ProcessInventory(std::uint32_t, pal4::PlayerManagerResultArray&) override {
        trace.push_back("collections"); inspect();
    }
    void FinalizeScript(std::uint32_t) override { trace.push_back("collection-finalize"); }
};
struct Refresh : pal4::PlayerManagerSlotScriptRuntimeStub {
    pal4::NativeBaseObjectMemory& memory;
    std::vector<std::string>& trace;
    Refresh(pal4::NativeBaseObjectMemory& m, std::vector<std::string>& t) : memory(m), trace(t) {}
    std::uint32_t ReadDword(std::uint32_t a) override { return memory.ReadDword(a); }
    void WriteDword(std::uint32_t a, std::uint32_t v) override { memory.WriteDword(a, v); }
    std::uint32_t GetValue(std::uint32_t s, std::uint32_t i) override { return ReadDword(s + (136u + i) * 4u); }
    std::uint32_t GetMaximum(std::uint32_t s, std::uint32_t i) override { return ReadDword(s + (86u + i) * 4u); }
    std::uint32_t SetValue(std::uint32_t s, std::uint32_t i, std::uint32_t v) override {
        return pal4::NativePlayerScriptPropertyRuntime::Set(s, 0x44u, i, v);
    }
    void ProcessInventory(std::uint32_t, std::uint32_t, std::uint32_t) override { trace.push_back("refresh"); }
};
struct Scripts : pal4::ScriptProcessRuntimeStub {
    pal4::NativeBaseObjectMemory& memory;
    std::vector<std::string>& trace;
    bool fail{};
    Scripts(pal4::NativeBaseObjectMemory& m, std::vector<std::string>& t) : memory(m), trace(t) {}
    std::uint32_t ReadDword(std::uint32_t a) override { return memory.ReadDword(a); }
    std::int32_t CompareCaseInsensitive(std::uint32_t, std::uint32_t) override {
        trace.push_back("script");
        if (fail) throw std::runtime_error("test script service failure");
        return 0;
    }
};
struct Session {
    pal4::PlayerManagerSlotState player{};
    pal4::NativeBaseObjectMemory memory;
    pal4::NativePlayerSlotCopyRuntime text;
    pal4::NativeGameSaveArchiveReader input;
    std::vector<std::string> trace;
    Collections collections{memory, trace};
    Refresh refresh{memory, trace};
    Scripts scripts{memory, trace};
    bool Load() {
        return pal4::NativePlayerLoadRuntime::Deserialize(Address(&player), memory, text, input,
            collections, refresh, scripts);
    }
    std::uint32_t Word(unsigned offset) { return memory.ReadDword(Address(&player) + offset); }
    void CheckOrder() {
        Check(trace == std::vector<std::string>{"collections", "collection-finalize", "refresh", "script"},
              "three shared post-load algorithms execute in original order");
    }
};
pal4::NativeGameSaveArchiveBuffer GeneratedArchive(bool forbidden = true, const char* host_marker = nullptr,
                                                 const char* player_root = "Player") {
    pal4::NativeGameSaveArchiveBuffer output;
    pal4::PlayerManagerSlotState source{};
    *reinterpret_cast<std::uint32_t*>(Address(&source) + 0x654u + 8u) = 100u;
    *reinterpret_cast<std::uint32_t*>(Address(&source) + 0x654u + 16u) = 20u;
    Check(output.WriteMarker("PR_SERIALIZE_FLAG"), "write PR marker");
    pal4::NativePlayerBasicSaveRuntime basic(output);
    Check(basic.Serialize(Address(&source)), "write BASIC");
    pal4::BinaryXmlNode node{player_root};
    node.attributes = {{"CA_HP", 1u, std::int32_t{23}}, {"CA_MP", 1u, std::int32_t{17}}};
    for (const auto& field : pal4::NativePlayerFieldSchema::Build()) {
        if (field.slot_offset == 0x654u + 86u * 4u)
            node.attributes.push_back({field.name, 1u, std::int32_t{100}});
        if (field.slot_offset == 0x654u + 88u * 4u)
            node.attributes.push_back({field.name, 1u, std::int32_t{20}});
    }
    if (forbidden) {
        pal4::BinaryXmlNode list{"ForbidenSkill"};
        for (const char* value : {"42", "-7", "  +19suffix", "invalid", "42"})
            list.children.push_back(pal4::BinaryXmlNode{value});
        node.children.push_back(std::move(list));
    }
    Check(output.WriteConfig(node), "write Player");
    if (host_marker) {
        Check(output.WriteMarker(host_marker), "write bad HostServer marker");
    } else {
        pal4::NativeHostServerSaveRuntime host(output);
        Check(host.Serialize(Address(&source)), "write HostServer");
    }
    return output;
}
void Generated() {
    Session s;
    const auto output = GeneratedArchive();
    s.collections.inspect = [&] {
        Check(s.Word(0x654u + 143u * 4u) == 23u && s.Word(0x654u + 145u * 4u) == 17u,
              "HP/MP are reapplied after all Player attributes before post-load");
        Check(s.Word(0x9f0u) - s.Word(0x9ecu) == 20u,
              "forbidden skill input completes before collection processing");
    };
    Fixture(output.Bytes());
    auto restore = [&] {
        s.trace.clear();
        Check(s.input.OpenSlot(Root(), 0) && s.Load(), "read complete generated player");
        Check(s.input.Remaining() == 0u, "continuous reader ends exactly after HostServer");
        s.input.Close(); s.CheckOrder();
        const std::uint32_t expected[]{42u, std::bit_cast<std::uint32_t>(std::int32_t{-7}), 19u, 0u, 42u};
        Check(s.Word(0x9f0u) - s.Word(0x9ecu) == sizeof(expected) &&
                  std::memcmp(reinterpret_cast<const void*>(s.Word(0x9ecu)), expected, sizeof(expected)) == 0,
              "forbidden skills preserve order, duplicate IDs and atoi semantics");
    };
    restore();
    const auto allocations = s.memory.OutstandingAllocations();
    const auto buffer = s.Word(0x9ecu);
    restore();
    Check(s.memory.OutstandingAllocations() == allocations && s.Word(0x9ecu) == buffer,
          "repeated load clears skills and reuses session capacity");
    const auto empty = GeneratedArchive(false);
    s.collections.inspect = [] {};
    Fixture(empty.Bytes());
    Check(s.input.OpenSlot(Root(), 0) && s.Load() && s.Word(0x9f0u) == s.Word(0x9ecu),
          "absent forbidden node clears previous logical entries");
    s.input.Close();

    auto bad = output.Bytes(); bad[4] = std::byte{'X'};
    Fixture(bad);
    s.trace.clear();
    const auto before = s.player;
    Check(s.input.OpenSlot(Root(), 0) && !s.Load() && s.trace.empty() &&
              std::memcmp(&before, &s.player, sizeof(before)) == 0,
          "wrong PR marker rejects before role mutation or post-load services");
    s.input.Close();
    for (const auto& invalid : {GeneratedArchive(true, "WRONG_HOST_FLAG"),
                               GeneratedArchive(true, nullptr, "WrongRoot")}) {
        Fixture(invalid.Bytes()); s.trace.clear();
        Check(s.input.OpenSlot(Root(), 0), "open malformed role");
        bool failed = false;
        try { (void)s.Load(); } catch (const std::runtime_error&) { failed = true; }
        Check(failed && s.trace.empty(), "bad XML root/HostServer rejects before post-load");
        s.input.Close();
    }
    Fixture(std::span(output.Bytes()).first(output.Bytes().size() - 1u)); s.trace.clear();
    Check(s.input.OpenSlot(Root(), 0), "open truncated role");
    bool failed = false;
    try { (void)s.Load(); } catch (const std::runtime_error&) { failed = true; }
    Check(failed && s.trace.empty(), "truncation prevents post-load calls"); s.input.Close();
    Fixture(output.Bytes()); s.scripts.fail = true; s.trace.clear();
    Check(s.input.OpenSlot(Root(), 0), "open service failure fixture"); failed = false;
    try { (void)s.Load(); } catch (const std::runtime_error&) { failed = true; }
    Check(failed && s.input.Remaining() == 0u, "post-load service failure propagates after exact archive read");
    s.input.Close(); s.scripts.fail = false;
    restore();
    (void)s.memory.Free(s.Word(0x9ecu));
    Check(s.memory.OutstandingAllocations() == 0u, "skill buffer and temporary collection storage release");
}
void Original(const std::filesystem::path& root) {
    std::ifstream file(root / "save/0.dat", std::ios::binary);
    Check(static_cast<bool>(file), "open original role fixture");
    const std::string bytes{std::istreambuf_iterator<char>(file), {}};
    pal4::NativeGameSaveArchiveBuffer marker;
    Check(marker.WriteMarker("PR_SERIALIZE_FLAG"), "encode PR marker");
    const std::string needle(reinterpret_cast<const char*>(marker.Bytes().data()), marker.Bytes().size());
    const auto begin = bytes.find(needle);
    Check(begin != std::string::npos, "find original first role for fixture extraction");
    // Only fixture extraction discovers the first section. Production loads all
    // four roles with one continuous reader, without scanning for later markers.
    Fixture(std::as_bytes(std::span(bytes.data() + begin, bytes.size() - begin)));
    pal4::NativeGameSaveArchiveReader input;
    Check(input.OpenSlot(Root(), 0), "open original continuous role sections");
    for (unsigned i = 0; i < 4u; ++i) {
        Session s;
        const auto before = input.Position();
        s.collections.inspect = [&] {
            Check(input.Position() > before, "all role input precedes post-load");
            pal4::NativeGameSaveArchiveBuffer host;
            pal4::NativeHostServerSaveRuntime writer(host);
            Check(writer.Serialize(Address(&s.player)), "reserialize loaded HostServer");
            const auto end = begin + static_cast<std::size_t>(input.Position());
            Check(host.Bytes().size() <= end &&
                      std::memcmp(host.Bytes().data(), bytes.data() + end - host.Bytes().size(), host.Bytes().size()) == 0,
                  "each loaded HostServer exactly matches original section ending");
        };
        Check(pal4::NativePlayerLoadRuntime::Deserialize(Address(&s.player), s.memory, s.text, input,
                  s.collections, s.refresh, s.scripts), "load original role at continuous cursor");
        s.CheckOrder();
    }
    pal4::BinaryXmlConfig manager;
    Check(input.ReadConfig(manager) && manager.Root().name == "P_MGR",
          "four complete roles leave cursor at original P_MGR document");
    input.Close();
}
struct Prepare : pal4::PlayerManagerOperationsRuntimeStub {
    pal4::PlayerManagerObjectState& manager;
    std::vector<std::string>& trace;
    Prepare(pal4::PlayerManagerObjectState& m, std::vector<std::string>& t) : manager(m), trace(t) {}
    unsigned Index(std::uint32_t slot) {
        const auto index = (slot - Address(&manager)) / pal4::PlayerManagerObject::kElementBytes;
        Check(index < 4u && slot == Address(&manager.slots[index]), "prepare receives valid owned slot");
        return index;
    }
    std::uint32_t UpdateInventory(std::uint32_t slot, std::uint32_t value) override {
        trace.push_back("inventory" + std::to_string(Index(slot)) + ":" + std::to_string(value)); return 0u;
    }
    void ProcessGameData(std::uint32_t slot) override {
        const auto index = Index(slot);
        // Fake RenderWare clump token, used only by the test transform runtime.
        manager.slots[index].base.Write(0x14u, 10u + index);
        trace.push_back("prepare" + std::to_string(index));
    }
    void CallScriptIndex(std::uint32_t script, std::uint32_t method, std::uint32_t index) override {
        Check(Index(script - 0x654u) == index && method == 0x18u, "prepare calls original script-index slot");
        trace.push_back("index" + std::to_string(index));
    }
};
struct Transforms : pal4::GameResourceManagerRuntimeStub {
    std::vector<std::string>& trace;
    explicit Transforms(std::vector<std::string>& t) : trace(t) {}
    std::uint32_t ClumpFrame(std::uint32_t clump) override {
        trace.push_back("transform" + std::to_string(clump - 10u)); return clump;
    }
};
struct ManagerSession {
    pal4::PlayerManagerObjectState manager{};
    pal4::NativeBaseObjectMemory memory;
    pal4::NativePlayerSlotCopyRuntime text;
    pal4::NativeGameSaveArchiveReader input;
    std::vector<std::string> trace;
    Collections collections{memory, trace};
    Refresh refresh{memory, trace};
    Scripts scripts{memory, trace};
    Prepare prepare{manager, trace};
    Transforms transforms{trace};
    bool Load() {
        return pal4::NativePlayerManagerLoadRuntime::Deserialize(manager, memory, text, input,
            {prepare, collections, refresh, scripts, transforms});
    }
    std::uint32_t Word(unsigned offset) { return memory.ReadDword(Address(&manager) + offset); }
    void CheckOrder(unsigned leading) {
        std::vector<std::string> expected;
        for (unsigned i = 0; i < 4u; ++i) {
            expected.push_back("inventory" + std::to_string(i) + ":0");
            expected.push_back("prepare" + std::to_string(i));
            expected.push_back("index" + std::to_string(i));
        }
        expected.push_back("inventory0:1");
        for (unsigned i = 0; i < 4u; ++i)
            for (const auto* stage : {"collections", "collection-finalize", "refresh", "script"})
                expected.push_back(stage);
        expected.push_back("transform" + std::to_string(leading));
        expected.push_back("transform" + std::to_string(leading));
        Check(trace == expected, "manager preparation precedes all four roles, then selected leader transforms");
    }
};
pal4::NativeGameSaveArchiveBuffer ManagerArchive(const pal4::BinaryXmlNode& document) {
    pal4::NativeGameSaveArchiveBuffer output;
    Check(output.WriteMarker("P_MGR_SERIALIZE_FLAG"), "write manager flag");
    const auto role = GeneratedArchive();
    for (unsigned i = 0; i < 4u; ++i) Check(output.WriteBytes(role.Bytes()), "write four role sections");
    Check(output.WriteConfig(document) && output.WriteMarker("NEXT_SECTION"), "write manager document and next cursor sentinel");
    return output;
}
void ManagerGenerated() {
    pal4::BinaryXmlNode node{"P_MGR"};
    node.attributes = {{"Leading", 1u, std::int32_t{2}}, {"Valid0", 1u, std::int32_t{1}},
        {"Valid2", 3u, std::string("-7")}, {"CameraAutoSeek", 1u, std::int32_t{9}},
        {"LeaderPos_X", 2u, 12.5f}, {"LeaderPos_Y", 3u, std::string("-3.25")},
        {"LeaderPos_Z", 1u, std::int32_t{17}}, {"LeaderRot_X", 2u, 45.0f},
        {"LeaderRot_Y", 2u, 90.0f}, {"LeaderRot_Z", 2u, 180.0f}};
    const auto output = ManagerArchive(node);
    Fixture(output.Bytes()); ManagerSession s;
    Check(s.input.OpenSlot(Root(), 0) && s.Load(), "load generated full manager");
    s.CheckOrder(2u);
    Check(s.Word(0x2c50u) == 2u && s.Word(0x2c54u) == 1u && s.Word(0x2c58u) == 0u &&
              s.Word(0x2c5cu) == 0xfffffff9u && s.Word(0x2c60u) == 0u,
          "leading/Valid scalar conversion and missing defaults match original loader");
    const auto leader = 2u * pal4::PlayerManagerObject::kElementBytes;
    for (const auto [offset, value] : {std::pair{0xacu, 12.5f}, {0xb0u, -3.25f}, {0xb4u, 17.0f},
            {0xa0u, 45.0f}, {0xa4u, 90.0f}, {0xa8u, 180.0f}})
        Check(s.Word(leader + offset) == std::bit_cast<std::uint32_t>(value), "shared transform restores leader position/rotation");
    for (unsigned i = 0; i < 4u; ++i)
        Check(s.Word(i * pal4::PlayerManagerObject::kElementBytes + 0xb0cu) == (i == 2u ? 9u : 0u),
              "camera auto-seek clears other slots and applies to selected leader");
    Check(s.input.ReadMarker() == "NEXT_SECTION" && s.input.Remaining() == 0u, "manager ends at exact next archive marker");
    s.input.Close();
    // Re-serialize all12 manager attributes from the actual restored image.
    pal4::NativeGameSaveArchiveBuffer restored;
    pal4::NativePlayerManagerSaveRuntime save(restored);
    Check(save.SerializeManagerDocument(Address(&s.manager)), "write restored manager document");
    pal4::BinaryXmlConfig parsed;
    Check(parsed.Parse(std::span(reinterpret_cast<const std::uint8_t*>(restored.Bytes().data()), restored.Bytes().size())) &&
              parsed.Root().attributes.size() == 12u, "restored manager emits all12 native fields");

    const auto defaults = ManagerArchive(pal4::BinaryXmlNode{"P_MGR"});
    Fixture(defaults.Bytes()); s.trace.clear();
    Check(s.input.OpenSlot(Root(), 0) && s.Load(), "repeat manager load with missing metadata");
    s.CheckOrder(0u);
    Check(s.Word(0x2c50u) == 0u && s.Word(0x2c54u) == 0u && s.Word(0xb0cu) == 1u &&
              s.Word(0xacu) == 0u && s.Word(0xa0u) == 0u, "manager missing scalar/vector/camera defaults");
    s.input.Close();
    auto bad = output.Bytes(); bad[4] = std::byte{'X'}; Fixture(bad); s.trace.clear();
    const auto before = s.manager;
    Check(s.input.OpenSlot(Root(), 0) && !s.Load() && s.trace.empty() &&
              std::memcmp(&before, &s.manager, sizeof(before)) == 0, "bad manager flag rejects before preparation or mutation");
    s.input.Close();
    // The first child marker immediately follows the20-byte manager flag.
    bad = output.Bytes(); bad[4u + 20u + 4u] = std::byte{'X'};
    Fixture(bad); s.trace.clear(); Check(s.input.OpenSlot(Root(), 0), "open bad child marker");
    bool failed = false;
    try { (void)s.Load(); } catch (const std::runtime_error&) { failed = true; }
    Check(failed && s.trace.size() == 13u, "bad child stops after preparation without later roles or transforms");
    s.input.Close();
    node.attributes[0].value = std::int32_t{-1};
    const auto invalid = ManagerArchive(node); Fixture(invalid.Bytes()); s.trace.clear();
    Check(s.input.OpenSlot(Root(), 0), "open invalid leader"); failed = false;
    try { (void)s.Load(); } catch (const std::out_of_range&) { failed = true; }
    Check(failed && s.trace.size() == 29u, "invalid leader rejected before out-of-bounds transform"); s.input.Close();
    const auto wrong_root = ManagerArchive(pal4::BinaryXmlNode{"wrong"}); Fixture(wrong_root.Bytes()); s.trace.clear();
    Check(s.input.OpenSlot(Root(), 0) && !s.Load() && s.trace.size() == 29u,
          "bad manager XML fails after role input and before metadata transforms"); s.input.Close();
}
void ManagerOriginal(const std::filesystem::path& root) {
    std::ifstream file(root / "save/0.dat", std::ios::binary);
    const std::string bytes{std::istreambuf_iterator<char>(file), {}};
    pal4::NativeGameSaveArchiveBuffer marker;
    Check(marker.WriteMarker("P_MGR_SERIALIZE_FLAG"), "encode original manager marker");
    const std::string needle(reinterpret_cast<const char*>(marker.Bytes().data()), marker.Bytes().size());
    const auto begin = bytes.find(needle); Check(begin != std::string::npos, "locate manager fixture prefix");
    Fixture(std::as_bytes(std::span(bytes.data() + begin, bytes.size() - begin)));
    ManagerSession s;
    Check(s.input.OpenSlot(Root(), 0) && s.Load(), "read original full manager continuously");
    s.CheckOrder(s.Word(0x2c50u));
    pal4::NativeGameSaveArchiveBuffer rebuilt;
    pal4::NativePlayerManagerSaveRuntime writer(rebuilt);
    Check(writer.SerializeManagerDocument(Address(&s.manager)), "serialize restored original manager metadata");
    const auto end = begin + static_cast<std::size_t>(s.input.Position());
    Check(rebuilt.Bytes().size() <= end &&
              std::memcmp(rebuilt.Bytes().data(), bytes.data() + end - rebuilt.Bytes().size(), rebuilt.Bytes().size()) == 0,
          "all12 restored P_MGR values reserialize byte for byte with original document");
    pal4::NativeArenaCacheOwner cache;
    Check(cache.LoadSection(s.input), "same reader restores original scene cache after PlayerManager");
    unsigned gob_restores = 0u, minimap_restores = 0u;
    cache.BindRestoreCallbacks([](std::string_view name) { return name == "effect001" ? 0x1111u : 0u; },
        [&](std::uint32_t object, const pal4::NativeArenaCacheChild& child) {
            Check(object == 0x1111u && child.id == 8u && child.document.Root().name == "soGOBEffect",
                  "original cached N01 effect snapshot reaches restore callback"); ++gob_restores;
        }, [&](std::uint32_t header, const auto& elements) {
            Check(header == 0xffff0000u && elements.empty(), "original cached N01 minimap header"); ++minimap_restores;
        });
    Check(cache.Restore("Q01", "N01") == 1u && gob_restores == 1u && minimap_restores == 1u,
          "continuously loaded cache retains original named snapshots");
    ArenaServices arena;
    Check(arena.Load(s.input) && arena.calls == std::vector<std::string>{"resources", "runtime", "players"} &&
        !arena.world.empty() && !arena.area.empty() && arena.image[49] == 1u && arena.image[111] == 0u,
        "same archive cursor deserializes original ARENA through shared scene service sequence");
    std::cout << "Original ARENA world=" << arena.world << " area=" << arena.area << " diff=" << arena.diff << '\n';
    const auto camera_begin = begin + static_cast<std::size_t>(s.input.Position());
    pal4::BinaryXmlConfig expected_camera;
    std::size_t camera_bytes{};
    Check(expected_camera.ParsePrefix(std::span(
        reinterpret_cast<const std::uint8_t*>(bytes.data() + camera_begin), bytes.size() - camera_begin), camera_bytes) &&
        expected_camera.Root().name == "CAM_MGR", "original camera XML directly follows ARENA without a marker");
    const auto* mode = expected_camera.Root().FindAttribute("current-camera-mode");
    const auto* yaw = expected_camera.Root().FindAttribute("current-camera-YAW");
    const auto* pitch = expected_camera.Root().FindAttribute("current-camera-PIT");
    Check(mode && yaw && pitch, "original camera stores mode/yaw/pitch");
    CameraResources resources;
    pal4::NativeCameraManagerOwner cameras;
    (void)cameras.Construct();
    Check(cameras.InitializeCameras(resources), "initialize camera owner for original archive restore");
    const auto updates = resources.updates;
    auto normalized = [](float value) {
        const auto remainder = std::fmod(value, 360.0f);
        return remainder < 0.0f ? remainder + 360.0f : remainder;
    };
    Check(cameras.LoadConfig(s.input) && cameras.State().At(0xc0u) == mode->ToInt(0u) &&
        cameras.Data(0u).At(0x3cu) == std::bit_cast<std::uint32_t>(
            normalized(yaw->ToFloat(0.0f))) &&
        cameras.Data(0u).At(0x40u) == std::bit_cast<std::uint32_t>(
            normalized(pitch->ToFloat(0.0f))) &&
        cameras.Data(0u).At(0x44u) == 0u && resources.updates == updates + 7u &&
        begin + s.input.Position() == camera_begin + camera_bytes,
        "same reader restores original camera fields through owned mode/setter path at exact cursor");
    ControlServices control_services;
    pal4::PalActorControlState control{};
    control.words[3u] = 99u;
    const auto control_begin=begin+static_cast<std::size_t>(s.input.Position());
    Check(pal4::NativeActorControlConfigLoadRuntime::Deserialize(control, s.input, control_services) == 1u &&
        control.words[3u] == 1u && control_services.calls == std::vector<std::string>{"input", "clear", "sync"} &&
        s.input.Remaining() == 0u, "same reader applies original final Actor-Controller config through EOF");
    pal4::NativeGameSaveArchiveBuffer control_output;
    Check(pal4::NativeActorControlConfigSaveRuntime::Serialize(control,control_output)==1u &&
              control_output.Bytes().size()==bytes.size()-control_begin &&
              std::memcmp(control_output.Bytes().data(),bytes.data()+control_begin,control_output.Bytes().size())==0,
        "native control output matches original final Actor-Controller bytes exactly");
    std::cout<<"Original final Actor-Controller document rewrites byte for byte\n";
    cameras.Destroy();
    cache.Destroy();
    Check(cache.OutstandingAllocations() == 0u, "continuously restored cache releases owned resources");
    s.input.Close();
}
void FullOriginal(const std::filesystem::path& root) {
    struct Host final : pal4::INativeGameSaveLoadHost {
        std::vector<std::string> calls;
        std::function<std::uint8_t(pal4::NativeGameSaveArchiveReader&)> load_control;
        void CleanupScriptCache() override { calls.push_back("cleanup"); }
        void ResetBackgroundMusic() override { calls.push_back("music-reset"); }
        void PlayMusic(std::string_view path, std::uint32_t loop) override {
            Check(path == "wa10.wav" && loop == 1u, "original load music request");
            calls.push_back("music");
        }
        void SetActorControlMode(std::uint32_t mode) override {
            Check(mode == 1u, "original load actor mode"); calls.push_back("mode");
        }
        std::uint8_t LoadActorControlConfig(pal4::NativeGameSaveArchiveReader& archive) override {
            return load_control(archive);
        }
    } host;
    std::array<std::uint32_t, 0x224u / 4u> ui_manager{};
    pal4::NativeGameTimerQueue timers([](std::uint32_t, std::uint32_t) {});
    ui_manager[0x1a0u / 4u] = timers.Construct();
    pal4::NativePalGameIVTextureStorage game([](std::uint32_t) {});
    (void)game.Construct({&timers, Address(ui_manager.data()), {}});
    pal4::DialogManagerState globals;
    alignas(4) std::array<std::byte, 0x658u> pet{};
    pal4::NativeGameDatabase database;
    Check(database.Load(root), "load original item database for complete restore");
    pal4::NativeInventorySession inventory;
    inventory.BindDatabase(database); (void)inventory.Start();
    ManagerSession players;
    pal4::NativePlayerCollectionCatalog collection_catalog(database);
    pal4::NativeBaseObjectMemory collection_memory;
    pal4::NativePlayerCollectionValues magic(collection_catalog, collection_memory,
        pal4::NativePlayerCollectionValues::Kind::Magic);
    pal4::NativePlayerCollectionValues stunt(collection_catalog, collection_memory,
        pal4::NativePlayerCollectionValues::Kind::Stunt);
    pal4::NativePlayerCollectionInventory equipment(collection_catalog, collection_memory, inventory);
    pal4::NativeSkillRecordCatalog skill_catalog(database);
    pal4::NativePlayerScriptRefreshRuntime script_refresh(players.memory,inventory);
    pal4::NativePlayerSkillCallbacks skill_callbacks(skill_catalog,script_refresh);
    pal4::RandomNumberGenerator random;
    random.Seed(123u);
    pal4::NativePlayerCombatEvents property_events;
    pal4::NativeSkillSystemOwner system(random,property_events);
    (void)system.Construct("H:/PAL4/objs");
    pal4::NativePlayerSkillRuntime skills(players.memory,skill_catalog,skill_callbacks,system);
    script_refresh.BindSkills(&skills);
    pal4::NativePlayerCollectionScriptRuntime collection_script(script_refresh);
    collection_script.BindSkills(&skills);
    // Scene construction is a deliberate fixture boundary. These four script
    // facets use the exact Player identity reviewed by the native skill adapter.
    for(auto& slot:players.manager.slots)
        players.memory.WriteDword(Address(&slot)+pal4::PlayerManagerSlot::kScriptOffset,
                                  pal4::PlayerManagerSlot::kScriptVtable);
    pal4::PlayerManagerSlotProcessRuntimeStub unused_slot_services;
    pal4::NativePlayerSlotProcessRuntime skill_cleanup(unused_slot_services,skill_callbacks);
    struct SkillPrepare final : pal4::PlayerManagerOperationsRuntimeStub {
        Prepare& host;
        pal4::NativePlayerSlotProcessRuntime& cleanup;
        SkillPrepare(Prepare& host,pal4::NativePlayerSlotProcessRuntime& cleanup) : host(host),cleanup(cleanup) {}
        std::uint32_t UpdateInventory(std::uint32_t slot,std::uint32_t value) override {
            return host.UpdateInventory(slot,value);
        }
        void ProcessGameData(std::uint32_t slot) override {
            cleanup.CleanupScriptLists(slot+pal4::PlayerManagerSlot::kScriptOffset);
            host.ProcessGameData(slot);
        }
        void CallScriptIndex(std::uint32_t script,std::uint32_t method,std::uint32_t index) override {
            host.CallScriptIndex(script,method,index);
        }
    } skill_prepare(players.prepare,skill_cleanup);
    pal4::NativePlayerSlotCollectionRuntime collections(collection_memory, equipment,
        magic, stunt, collection_script);
    pal4::NativeArenaCacheOwner cache;
    ArenaServices arena;
    CameraResources resources;
    pal4::NativeCameraManagerOwner cameras;
    (void)cameras.Construct();
    Check(cameras.InitializeCameras(resources), "full load initializes camera owner");
    pal4::PalActorControlState control{};
    ControlServices control_services;
    host.load_control = [&](pal4::NativeGameSaveArchiveReader& archive) {
        return pal4::NativeActorControlConfigLoadRuntime::Deserialize(control, archive, control_services);
    };
    pal4::NativeGameSaveLoadServices services{game, globals, pet, inventory,
        players.manager, players.memory, players.text,
        {skill_prepare, collections, script_refresh, players.scripts, players.transforms},
        cache, Address(arena.image.data()), arena, cameras, host};
    pal4::NativeGameSaveArchiveReader expected;
    pal4::GameSaveHeader expected_header{}, header{};
    Check(expected.OpenSlot(root, 0), "open untouched original save");
    const auto expected_marker = expected.ReadMarker();
    Check(expected_marker && expected.ReadHeader(expected_header), "original save header reference");
    std::string marker;
    Check(pal4::NativeGameSaveLoad::LoadSlot(root, 0, true, &header, &marker, services) == 1u &&
        marker == *expected_marker && std::memcmp(&header, &expected_header, sizeof(header)) == 0 &&
        host.calls == std::vector<std::string>{"cleanup"} && players.trace.empty(),
        "quick load reads original header without restoring subsystems");
    host.calls.clear();
    Check(pal4::NativeGameSaveLoad::LoadSlot(root, 999999, false, nullptr, nullptr, services) == 0u &&
        host.calls == std::vector<std::string>{"cleanup"}, "missing save still cleans script cache");
    host.calls.clear();
    std::memset(&header, 0x5a, sizeof(header));
    const auto untouched_header = header;
    Check(pal4::NativeGameSaveLoad::LoadSlot(root, 0, false, &header, &marker, services) == 1u &&
        host.calls == std::vector<std::string>{"cleanup", "music-reset", "music", "mode"} &&
        control_services.calls == std::vector<std::string>{"input", "clear", "sync"} &&
        arena.world == "Q01" && arena.area == "Q01" && control.words[3u] == 1u,
        "whole original save restores every native section through shared GameSaveReader");
    Check(collection_memory.OutstandingAllocations() == 0u,
        "complete original save releases all temporary native passive collection lists");
    unsigned loaded_skills{};
    for(unsigned index=0;index<4u;++index) {
        const auto script=Address(&players.manager.slots[index])+pal4::PlayerManagerSlot::kScriptOffset;
        const auto begin=players.memory.ReadDword(script+696u),end=players.memory.ReadDword(script+700u);
        for(auto entry=begin;entry!=end;entry+=8u) {
            const auto record=players.memory.ReadDword(entry);
            if(!record) continue;
            Check(skill_catalog.ResolveRecord(record)==record,"original save retains real native Skill records");
            ++loaded_skills;
            std::cout << "Full save Player=" << index << " passive=" << players.memory.ReadDword(record) << '\n';
        }
    }
    Check(loaded_skills==0u,"initial original save has no passive skills to fabricate");
    Check(std::memcmp(&header, &untouched_header, sizeof(header)) == 0,
        "full load preserves original header-output semantics");
    // Save the actually restored original data through the shared top-level
    // writer, then restore the generated file through the full native reader.
    // Scene capture, audio/thumbnail/settings and SDK resources remain explicit
    // observing boundaries; all archive sections use real native serializers.
    struct SaveHost final : pal4::INativeGameSaveWriteHost {
        ArenaServices& arena;
        std::uint32_t ui;
        std::vector<std::string> calls;
        bool fail_capture{};
        SaveHost(ArenaServices& arena,std::uint32_t ui):arena(arena),ui(ui){}
        void PlaySaveSound(std::string_view path,std::uint32_t repeat) override {
            Check(path=="wa11.wav" && repeat==1u,"full save keeps original save sound request");calls.push_back("sound");
        }
        std::uint32_t TimerFrameDuration(std::uint32_t manager) override {
            Check(manager==ui,"save timer uses borrowed current UI manager");calls.push_back("timer");return 1234u;
        }
        pal4::NativeArenaCacheEntry CaptureCurrentArena() override {
            calls.push_back("capture");if(fail_capture)throw std::runtime_error("save capture service failure");
            pal4::NativeArenaCacheEntry entry;entry.names={arena.world,arena.diff};return entry;
        }
        void CaptureThumbnail(std::int32_t slot) override {
            Check(slot==42 || slot==43 || slot==44,"save thumbnail receives selected slot");calls.push_back("thumbnail");
        }
        void SaveDialogSettings() override {calls.push_back("settings");}
    } save_host(arena,Address(ui_manager.data()));
    const auto save_root=Root()/"native-save";
    std::filesystem::create_directories(save_root);
    const auto original_directory=std::filesystem::current_path();
    pal4::NativeGameSaveWriteServices save_services{game,globals,pet,inventory,players.manager,cache,
        Address(arena.image.data()),cameras,control,Address(ui_manager.data()),save_host};
    Check(cameras.SetMainCameraYaw(123.0f,1u) && cameras.SetMainCameraPitch(345.0f,1u),
        "prepare nondefault saved camera");
    control.words[3u]=0xfffffff9u;
    (void)inventory.UpdateCounter(17);
    const auto saved_money=players.memory.ReadDword(inventory.Address()+308u);
    const auto saved_control=control;
    const auto saved_camera=cameras.Data(0u);
    pal4::NativeSceneRecordCatalog save_scenes(database);
    std::string save_scene_name;
    std::uint32_t save_scene_id{};
    for (std::size_t index=0u;index<save_scenes.Records().size();++index) {
        const auto& record=save_scenes.Records()[index];
        if(record.world==arena.world && record.scene==arena.diff) {
            arena.image[0x108u/4u]=record.address;
            save_scene_name=std::get<std::string>(database.Tables().at("Scene").rows[index][3u]);
            save_scene_id=save_scenes.Image(index)[0u];
            break;
        }
    }
    Check(!save_scene_name.empty(),"full save uses actual original Scene NAME string object");
    Check(pal4::NativeGameSaveWrite::SaveSlot(save_root,42,save_services)==1u &&
              save_host.calls==std::vector<std::string>{"sound","timer","capture","thumbnail","settings"} &&
              std::filesystem::current_path()==original_directory && control.words==saved_control.words &&
              cameras.Data(0u).words==saved_camera.words,
        "shared full writer serializes all real sections, orders required effects and preserves process cwd/control/camera");
    pal4::NativeGameSaveArchiveReader written;
    pal4::GameSaveHeader written_header;
    Check(written.OpenSlot(save_root,42) && written.ReadMarker()==save_scene_name && written.ReadHeader(written_header) &&
              written_header.version==1u && written_header.player_models[0]==players.manager.Word(0xb08u/4u) &&
              written_header.player_properties==expected_header.player_properties &&
              written_header.arena_record_id==save_scene_id &&
              written.ReadMarker()=="GA_SERIALIZE_FLAG",
        "native full file has original marker/header followed immediately by GameData");
    written.Close();
    (void)inventory.UpdateCounter(200);control.words[3u]=1u;
    Check(cameras.SetMainCameraYaw(5.0f,1u) && cameras.SetMainCameraPitch(6.0f,1u),"change state before native save roundtrip");
    host.calls.clear();control_services.calls.clear();
    Check(pal4::NativeGameSaveLoad::LoadSlot(save_root,42,false,nullptr,nullptr,services)==1u &&
              players.memory.ReadDword(inventory.Address()+308u)==saved_money && control.words[3u]==0xfffffff9u &&
              cameras.Data(0u).At(0x3cu)==std::bit_cast<std::uint32_t>(123.0f) &&
              cameras.Data(0u).At(0x40u)==std::bit_cast<std::uint32_t>(345.0f) &&
              game.GameDataEntry(70)==1234u && arena.world=="Q01" && arena.area=="Q01" &&
              control_services.calls==std::vector<std::string>{"input","clear","sync"},
        "full native saved file restores inventory, signed control flag, nondefault camera and timer through last section");
    save_host.calls.clear();save_host.fail_capture=true;
    bool capture_failed=false;
    try{(void)pal4::NativeGameSaveWrite::SaveSlot(save_root,43,save_services);}catch(const std::runtime_error&){capture_failed=true;}
    Check(capture_failed && save_host.calls==std::vector<std::string>{"sound","timer","capture"},
        "throwing scene capture propagates before thumbnail/settings and releases the native file");
    save_host.calls.clear();save_host.fail_capture=false;
    Check(pal4::NativeGameSaveWrite::SaveSlot(save_root,43,save_services)==1u &&
              save_host.calls==std::vector<std::string>{"sound","timer","capture","thumbnail","settings"},
        "native full save retries the same slot after capture exception");
    save_host.calls.clear();
    const auto blocked_root=Root()/"save-blocked-root";
    {std::ofstream blocked(blocked_root);blocked<<"file";}
    Check(pal4::NativeGameSaveWrite::SaveSlot(blocked_root,42,save_services)==0u && save_host.calls.empty() &&
              std::filesystem::current_path()==original_directory,
        "failed native file open produces no game/audio/thumbnail/settings effects or process cwd changes");
    std::cout<<"Full native save: original restored owners -> shared writer -> all section serializers -> complete reader roundtrip; capture failure/retry and open failure pass\n";
    // Restore the original baseline required by the remaining failure/late-save probes.
    Check(pal4::NativeGameSaveLoad::LoadSlot(root,0,false,nullptr,nullptr,services)==1u,"restore baseline after full save roundtrip");
    // A throwing scene service must close the file and permit a subsequent load.
    arena.fail_runtime = true;
    bool threw = false;
    try { (void)pal4::NativeGameSaveLoad::LoadSlot(root, 0, false, nullptr, nullptr, services); }
    catch (const std::runtime_error&) { threw = true; }
    Check(threw, "full load propagates scene failure");
    arena.fail_runtime = false;
    control_services.calls.clear();
    Check(pal4::NativeGameSaveLoad::LoadSlot(root, 0, false, nullptr, nullptr, services) == 1u &&
        control_services.calls.size() == 3u, "full load retries after scene exception");
    // ARENA replaces the old scene service before the final controller read.
    // A captured reference from before LoadSlot would address a destroyed owner.
    auto current_control_scene = std::make_unique<ControlServices>();
    unsigned scene_generation{}, final_generation{};
    auto previous_camera_updates = resources.updates;
    arena.after_initialize_players = [&] {
        current_control_scene = std::make_unique<ControlServices>();
        ++scene_generation;
    };
    host.load_control = [&](pal4::NativeGameSaveArchiveReader& archive) {
        Check(scene_generation > final_generation && resources.updates == previous_camera_updates + 7u,
              "final controller resolves scene after ARENA and camera restore");
        final_generation = scene_generation;
        previous_camera_updates = resources.updates;
        const auto result = pal4::NativeActorControlConfigLoadRuntime::Deserialize(
            control, archive, *current_control_scene);
        Check(archive.Remaining() == 0u, "resolved controller consumes the same archive through EOF");
        return result;
    };
    for (unsigned pass = 0u; pass < 2u; ++pass) {
        Check(pal4::NativeGameSaveLoad::LoadSlot(root, 0, false, nullptr, nullptr, services) == 1u &&
                  scene_generation == pass + 1u && final_generation == scene_generation &&
                  current_control_scene->calls == std::vector<std::string>{"input", "clear", "sync"},
              "consecutive loads restore the current scene instead of retaining the previous owner");
    }
    arena.after_initialize_players = {};
    host.load_control = [&](pal4::NativeGameSaveArchiveReader& archive) {
        return pal4::NativeActorControlConfigLoadRuntime::Deserialize(control, archive, control_services);
    };
    // Read unchanged late save from byte zero, retaining the same archive
    // through header, GameData, Inventory, players and cache.
    pal4::NativeGameSaveArchiveReader late_input;
    Check(late_input.OpenSlot(root,74),"open unchanged late original save");
    pal4::GameSaveHeader late_header{};
    Check(late_input.ReadMarker().has_value() && late_input.ReadHeader(late_header),
          "late original header reads continuously from byte zero");
    pal4::NativeGameDataPersistenceRuntime late_data(game,globals,late_input,pet);
    Check(late_data.Deserialize(),"late original GameData restores without marker scanning");
    pal4::NativeInventoryLoadRuntime late_inventory(inventory,late_input);
    Check(late_inventory.Deserialize()!=0u &&
        pal4::NativePlayerManagerLoadRuntime::Deserialize(players.manager,players.memory,players.text,
            late_input,services.player_services),"late original inventory and four players restore with native skill services");
    unsigned late_passives{};
    const std::array<std::vector<unsigned>,4u> expected_passives{{
        {7069u,7029u,7400u},{7067u,7032u},{7067u,7029u},{7029u}
    }};
    for(unsigned index=0u;index<4u;++index) {
        const auto script=Address(&players.manager.slots[index])+pal4::PlayerManagerSlot::kScriptOffset;
        std::vector<unsigned> active_passives;
        for(auto entry=players.memory.ReadDword(script+696u);entry!=players.memory.ReadDword(script+700u);entry+=8u) {
            const auto record=players.memory.ReadDword(entry);
            if(!record) continue;
            Check(skill_catalog.ResolveRecord(record)==record,"late original save retains owned native Skill records");
            ++late_passives;
            const auto id=players.memory.ReadDword(record);
            active_passives.push_back(id);
            std::cout << "Late save Player=" << index << " passive=" << id << '\n';
        }
        Check(active_passives==expected_passives[index],"late original role preserves its exact passive IDs and collection order");
        std::array<unsigned,84u> before{};
        for(unsigned word=0;word<before.size();++word) before[word]=players.memory.ReadDword(script+0x154u+word*4u);
        (void)script_refresh.Refresh(script);
        for(unsigned word=0;word<before.size();++word)
            Check(players.memory.ReadDword(script+0x154u+word*4u)==before[word],
                  "late-save native inventory/passive refresh preserves all84 current BASIC words without compounding");
    }
    Check(late_passives==8u && collection_memory.OutstandingAllocations()==0u,
          "late original restore actually applies nonempty native passives and releases temporary collections");
    std::cout << "Late original native passive count=" << late_passives << '\n';
    pal4::BinaryXmlConfig late_arena;
    std::cout << "Late cache input offset=" << late_input.Position() << '\n';
    const auto late_cache_loaded=cache.LoadSection(late_input);
    std::cout << "Late cache loaded=" << late_cache_loaded << " offset=" << late_input.Position()
              << " error=" << late_input.LastError() << '\n';
    Check(late_cache_loaded && late_input.ReadMarker()=="ARENA_SERIALIZE_FLAG" &&
          late_input.ReadConfig(late_arena) && late_arena.Root().name=="ARENA",
          "late inventory/player/cache input retains the exact next ARENA document boundary");
    late_input.Close();
    host.calls.clear(); control_services.calls.clear();
    Check(pal4::NativeGameSaveLoad::LoadSlot(root,74,false,nullptr,nullptr,services)==1u &&
          host.calls==std::vector<std::string>{"cleanup","music-reset","music","mode"} &&
          control_services.calls==std::vector<std::string>{"input","clear","sync"},
          "whole late original save loads continuously through final actor control");
    pal4::NativeGameSaveArchiveBuffer late_players_before;
    Check(pal4::NativePlayerManagerSaveRuntime(late_players_before).Serialize(Address(&players.manager),inventory),
        "capture complete late-player archive before full native save");
    save_host.calls.clear();
    Check(pal4::NativeGameSaveWrite::SaveSlot(save_root,44,save_services)==1u &&
              pal4::NativeGameSaveLoad::LoadSlot(save_root,44,false,nullptr,nullptr,services)==1u,
        "late original data writes and restores through the complete native archive chain");
    pal4::NativeGameSaveArchiveBuffer late_players_after;
    Check(pal4::NativePlayerManagerSaveRuntime(late_players_after).Serialize(Address(&players.manager),inventory) &&
              late_players_before.Bytes()==late_players_after.Bytes(),
        "full native late-save roundtrip preserves all four serialized players including equipped and passive data");
    unsigned saved_passives{};
    for(unsigned index=0;index<4u;++index){
        const auto script=Address(&players.manager.slots[index])+pal4::PlayerManagerSlot::kScriptOffset;
        std::vector<unsigned> ids;
        const auto end=players.memory.ReadDword(script+700u);
        for(auto entry=players.memory.ReadDword(script+696u);entry!=end;entry+=8u){
            const auto record=players.memory.ReadDword(entry);if(!record)continue;
            Check(skill_catalog.ResolveRecord(record)==record,"roundtrip passive remains owned by native Skill catalog");
            ids.push_back(players.memory.ReadDword(record));++saved_passives;
        }
        Check(ids==expected_passives[index],"late full save/read retains exact passive IDs and order per role");
    }
    Check(saved_passives==8u && collection_memory.OutstandingAllocations()==0u,
        "late complete save/read restores eight actual native passive records without temporary leaks");
    std::cout<<"Late full native save/read: all four player archives stable, eight real passive skills and equipped data preserved\n";
    std::array<std::byte, 80> bad{};
    const std::int32_t invalid_length = 1000;
    std::memcpy(bad.data(), &invalid_length, sizeof(invalid_length));
    Fixture(bad);
    host.calls.clear(); threw = false;
    try { (void)pal4::NativeGameSaveLoad::LoadSlot(Root(), 0, false, nullptr, nullptr, services); }
    catch (const std::runtime_error&) { threw = true; }
    Check(threw && host.calls == std::vector<std::string>{"cleanup"},
        "malformed file header stops before music or subsystem effects");
    const auto exclusive = CreateFileW((Root() / "save/0.dat").c_str(), GENERIC_READ,
        0u, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    Check(exclusive != INVALID_HANDLE_VALUE, "exception releases archive file handle");
    CloseHandle(exclusive);
    std::ifstream original(root / "save/0.dat", std::ios::binary);
    std::vector<char> prefix(static_cast<std::size_t>(expected.Position()) + 8u);
    original.read(prefix.data(), prefix.size());
    Fixture(std::as_bytes(std::span(prefix)));
    host.calls.clear(); threw = false;
    try { (void)pal4::NativeGameSaveLoad::LoadSlot(Root(), 0, false, nullptr, nullptr, services); }
    catch (const std::runtime_error&) { threw = true; }
    Check(threw && host.calls == std::vector<std::string>{"cleanup", "music-reset", "music"},
        "truncated GameData stops complete load before actor mode or scene effects");
    Check(pal4::NativeGameSaveLoad::LoadSlot(root, 0, true, nullptr, nullptr, services) == 1u,
        "valid quick load still works after malformed input");
    for(auto& slot:players.manager.slots) {
        const auto script=Address(&slot)+pal4::PlayerManagerSlot::kScriptOffset;
        skill_cleanup.CleanupScriptLists(script);
        for(const auto offset:{680u,696u}) {
            (void)players.memory.Free(players.memory.ReadDword(script+offset));
            for(unsigned word=0;word<3u;++word) players.memory.WriteDword(script+offset+word*4u,0u);
        }
    }
    collection_script.BindSkills(nullptr); script_refresh.BindSkills(nullptr);
    cache.Destroy(); cameras.Destroy(); inventory.Close(); game.Destroy();
}
}  // namespace
int main(int argc, char** argv) {
    try {
        if (argc > 1) { Original(argv[1]); ManagerOriginal(argv[1]); FullOriginal(argv[1]); }
        else { Generated(); ManagerGenerated(); ArenaGenerated(); ControlGenerated(); ControlOutput(); }
        std::filesystem::remove_all(Root());
        std::cout << "PASS: native continuous player loading\n";
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n'; return 1;
    }
}
