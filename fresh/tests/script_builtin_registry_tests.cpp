#include "pal4/script_builtin_registry.h"

#include <array>
#include <cstdlib>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool value, const char* message) {
    if (!value) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::ScriptBuiltinRuntimeStub {
    std::uint32_t next_address = 0x1000u;
    std::vector<std::string> events;
    std::map<std::uint32_t, std::string> strings;
    std::map<std::uint32_t, pal4::ScriptBuiltinDescriptor> descriptors;
    std::vector<std::uint32_t> exit_callbacks;

    std::uint32_t Allocate(std::uint32_t size, std::uint32_t kind,
                           std::uint32_t protection) override {
        Check(kind == 0x1000u && protection == 4u,
              "VirtualAlloc uses commit and read-write flags");
        const auto address = next_address;
        next_address += 0x100u;
        events.push_back("alloc:" + std::to_string(address) + ":" +
                         std::to_string(size));
        return address;
    }
    void CopyString(std::uint32_t destination,
                    std::string_view source) override {
        Check(!source.empty() && source.back() == '\0',
              "strncpy includes the terminator");
        strings[destination] = std::string(source);
        events.push_back("copy:" + std::to_string(destination));
    }
    void Release(std::uint32_t address, std::uint32_t size,
                 std::uint32_t kind) override {
        Check(kind == 0x8000u, "VirtualFree uses MEM_RELEASE flag");
        Check(strings.at(address).size() == size,
              "VirtualFree receives the stored string size");
        events.push_back("free:" + std::to_string(address));
    }
    void Register(std::uint32_t address,
                  const pal4::ScriptBuiltinDescriptor& descriptor) override {
        Check(strings.at(descriptor.description).size() ==
                  descriptor.description_size &&
                  strings.at(descriptor.signature).size() ==
                  descriptor.signature_size,
              "registration follows both copies");
        descriptors[address] = descriptor;
        events.push_back("register:" + std::to_string(address));
    }
    int RegisterAtExit(std::uint32_t callback) override {
        exit_callbacks.push_back(callback);
        events.push_back("atexit:" + std::to_string(callback));
        return 7;
    }
};

std::uint32_t Hash(std::string_view bytes) {
    std::uint32_t value = 2166136261u;
    for (unsigned char byte : bytes)
        value = (value ^ byte) * 16777619u;
    return value;
}

void CheckDescriptor(const Runtime& runtime, std::uint32_t address,
                     std::uint32_t handler, const char* signature,
                     std::size_t description_size,
                     std::uint32_t description_hash) {
    const auto& descriptor = runtime.descriptors.at(address);
    Check(descriptor.callback_data[0] == handler,
          "first callback word is the handler");
    for (std::size_t index = 1; index < descriptor.callback_data.size(); ++index)
        Check(descriptor.callback_data[index] == 0u,
              "remaining callback words are zero");
    Check(runtime.strings.at(descriptor.signature) ==
              std::string(signature, std::char_traits<char>::length(signature) + 1u),
          "prototype string matches IDA");
    Check(runtime.strings.at(descriptor.description).size() == description_size,
          "GBK description length includes NUL");
    Check(Hash(runtime.strings.at(descriptor.description)) == description_hash,
          "all original GBK description bytes match IDA");
    Check(descriptor.signature_size ==
              std::char_traits<char>::length(signature) + 1u,
          "prototype length includes NUL");
}

void TestRegistrationAndInitializerOrder() {
    Runtime runtime;
    pal4::ScriptBuiltinRegistry registry(runtime);
    Check(registry.InitializeImmBegin() == 7, "begin atexit result propagates");
    Check(registry.InitializeImmEnd() == 7, "end atexit result propagates");
    Check(registry.InitializeNewGame() == 7, "new-game atexit result propagates");
    Check(registry.InitializeCameraYpr() == 7, "YPR atexit result propagates");
    Check(registry.InitializeCameraDistance() == 7,
          "distance atexit result propagates");
    Check(registry.InitializeCameraYprd() == 7,
          "YPRD atexit result propagates");
    Check(registry.InitializeCameraGetDistance() == 7,
          "get-distance atexit result propagates");
    Check(registry.InitializeCameraGetYaw() == 7,
          "get-yaw atexit result propagates");
    Check(registry.InitializeCameraGetPitch() == 7,
          "get-pitch atexit result propagates");
    Check(registry.InitializeCameraGetRoll() == 7,
          "get-roll atexit result propagates");
    Check(runtime.exit_callbacks == std::vector<std::uint32_t>{
              pal4::ScriptBuiltinRegistry::kImmBeginCleanup,
              pal4::ScriptBuiltinRegistry::kImmEndCleanup,
              pal4::ScriptBuiltinRegistry::kNewGameCleanup,
              pal4::ScriptBuiltinRegistry::kCameraYprCleanup,
              pal4::ScriptBuiltinRegistry::kCameraDistanceCleanup,
              pal4::ScriptBuiltinRegistry::kCameraYprdCleanup,
              pal4::ScriptBuiltinRegistry::kCameraGetDistanceCleanup,
              pal4::ScriptBuiltinRegistry::kCameraGetYawCleanup,
              pal4::ScriptBuiltinRegistry::kCameraGetPitchCleanup,
              pal4::ScriptBuiltinRegistry::kCameraGetRollCleanup},
          "ten static initializers register their matching destructors");
    CheckDescriptor(runtime, pal4::ScriptBuiltinRegistry::kImmBeginDescriptor,
                    pal4::ScriptBuiltinRegistry::kImmBeginHandler,
                    "void giIMMBegin()", 29u, 0x0c749a2au);
    CheckDescriptor(runtime, pal4::ScriptBuiltinRegistry::kImmEndDescriptor,
                    pal4::ScriptBuiltinRegistry::kImmEndHandler,
                    "void giIMMEnd()", 29u, 0x2444d499u);
    CheckDescriptor(runtime, pal4::ScriptBuiltinRegistry::kNewGameDescriptor,
                    pal4::ScriptBuiltinRegistry::kNewGameHandler,
                    "void giNewGame()", 63u, 0x1b941567u);
    CheckDescriptor(runtime, pal4::ScriptBuiltinRegistry::kCameraYprDescriptor,
                    pal4::ScriptBuiltinRegistry::kCameraYprHandler,
                    "void giCameraCtrlYPR( float, float, float, bool )",
                    236u, 0x75d7861au);
    CheckDescriptor(runtime, pal4::ScriptBuiltinRegistry::kCameraDistanceDescriptor,
                    pal4::ScriptBuiltinRegistry::kCameraDistanceHandler,
                    "void giCameraCtrlDist( float, bool )", 146u, 0x2b9b8de4u);
    CheckDescriptor(runtime, pal4::ScriptBuiltinRegistry::kCameraYprdDescriptor,
                    pal4::ScriptBuiltinRegistry::kCameraYprdHandler,
                    "void giCameraCtrlYPRD( float, float, float, float, bool )",
                    274u, 0x0dea65b8u);
    CheckDescriptor(runtime, pal4::ScriptBuiltinRegistry::kCameraGetDistanceDescriptor,
                    pal4::ScriptBuiltinRegistry::kCameraGetDistanceHandler,
                    "float giCameraGetDist()", 89u, 0x20e69d2du);
    CheckDescriptor(runtime, pal4::ScriptBuiltinRegistry::kCameraGetYawDescriptor,
                    pal4::ScriptBuiltinRegistry::kCameraGetYawHandler,
                    "float giCameraGetYaw()", 91u, 0x3fb1fca7u);
    CheckDescriptor(runtime, pal4::ScriptBuiltinRegistry::kCameraGetPitchDescriptor,
                    pal4::ScriptBuiltinRegistry::kCameraGetPitchHandler,
                    "float giCameraGetPitch()", 91u, 0x7dcd7f61u);
    CheckDescriptor(runtime, pal4::ScriptBuiltinRegistry::kCameraGetRollDescriptor,
                    pal4::ScriptBuiltinRegistry::kCameraGetRollHandler,
                    "float giCameraGetRoll()", 91u, 0x6b097829u);
    Check(runtime.events.size() == 60u, "each command has six ordered operations");
    for (std::size_t i = 0; i < 10u; ++i) {
        const auto base = i * 6u;
        Check(runtime.events[base].starts_with("alloc:") &&
                  runtime.events[base + 1u].starts_with("copy:") &&
                  runtime.events[base + 2u].starts_with("alloc:") &&
                  runtime.events[base + 3u].starts_with("copy:") &&
                  runtime.events[base + 4u].starts_with("register:") &&
                  runtime.events[base + 5u].starts_with("atexit:"),
              "description, signature, register and atexit order");
    }
    const auto& begin = registry.ImmBegin();
    const auto& end = registry.ImmEnd();
    const auto& game = registry.NewGame();
    const auto& camera = registry.CameraYpr();
    Check(begin.description != end.description &&
              end.description != game.description &&
              begin.signature != game.signature &&
              game.description != camera.description,
          "global descriptor storage remains independent");
    const auto& begin_text = runtime.strings.at(begin.description);
    const auto& end_text = runtime.strings.at(end.description);
    const auto& game_text = runtime.strings.at(game.description);
    Check(begin_text.substr(0, 7) == end_text.substr(0, 7) &&
              begin_text[7] != end_text[7] &&
              game_text[7] != begin_text[7],
          "GBK command descriptions retain distinct content");
    Check(static_cast<unsigned char>(begin_text[1]) == 0xcbu &&
              static_cast<unsigned char>(game_text[7]) == 0xd0u,
          "source GBK bytes are preserved");
}

void TestCleanupOrderAndZeroing() {
    Runtime runtime;
    pal4::ScriptBuiltinRegistry registry(runtime);
    registry.RegisterImmBegin();
    registry.RegisterImmEnd();
    registry.RegisterNewGame();
    registry.RegisterCameraYpr();
    registry.RegisterCameraDistance();
    registry.RegisterCameraYprd();
    registry.RegisterCameraGetDistance();
    registry.RegisterCameraGetYaw();
    registry.RegisterCameraGetPitch();
    registry.RegisterCameraGetRoll();
    const auto begin = registry.ImmBegin();
    const auto end = registry.ImmEnd();
    const auto game = registry.NewGame();
    const auto camera_ypr = registry.CameraYpr();
    const auto camera_distance = registry.CameraDistance();
    const auto camera_yprd = registry.CameraYprd();
    const auto camera_get_distance = registry.CameraGetDistance();
    const auto camera_get_yaw = registry.CameraGetYaw();
    const auto camera_get_pitch = registry.CameraGetPitch();
    const auto camera_get_roll = registry.CameraGetRoll();
    runtime.events.clear();
    registry.CleanupCameraGetRoll();
    registry.CleanupCameraGetPitch();
    registry.CleanupCameraGetYaw();
    registry.CleanupCameraGetDistance();
    registry.CleanupCameraYprd();
    registry.CleanupCameraDistance();
    registry.CleanupCameraYpr();
    registry.CleanupNewGame();
    registry.CleanupImmEnd();
    registry.CleanupImmBegin();
    Check(runtime.events == std::vector<std::string>{
              "free:" + std::to_string(camera_get_roll.signature),
              "free:" + std::to_string(camera_get_roll.description),
              "free:" + std::to_string(camera_get_pitch.signature),
              "free:" + std::to_string(camera_get_pitch.description),
              "free:" + std::to_string(camera_get_yaw.signature),
              "free:" + std::to_string(camera_get_yaw.description),
              "free:" + std::to_string(camera_get_distance.signature),
              "free:" + std::to_string(camera_get_distance.description),
              "free:" + std::to_string(camera_yprd.signature),
              "free:" + std::to_string(camera_yprd.description),
              "free:" + std::to_string(camera_distance.signature),
              "free:" + std::to_string(camera_distance.description),
              "free:" + std::to_string(camera_ypr.signature),
              "free:" + std::to_string(camera_ypr.description),
              "free:" + std::to_string(game.signature),
              "free:" + std::to_string(game.description),
              "free:" + std::to_string(end.signature),
              "free:" + std::to_string(end.description),
              "free:" + std::to_string(begin.signature),
              "free:" + std::to_string(begin.description)},
          "each destructor releases signature before description");
    Check(registry.ImmBegin().description == 0u &&
              registry.ImmEnd().signature == 0u &&
              registry.NewGame().description_size == 0u &&
              registry.CameraYpr().signature_size == 0u &&
              registry.CameraDistance().description == 0u &&
              registry.CameraYprd().signature == 0u &&
              registry.CameraGetDistance().description_size == 0u &&
              registry.CameraGetYaw().signature == 0u &&
              registry.CameraGetPitch().description == 0u &&
              registry.CameraGetRoll().signature_size == 0u,
          "destructors clear the four string fields");
    Check(registry.ImmBegin().callback_data[0] ==
              pal4::ScriptBuiltinRegistry::kImmBeginHandler,
          "destructor retains the callback payload");
    runtime.events.clear();
    registry.CleanupImmBegin();
    registry.CleanupImmEnd();
    registry.CleanupNewGame();
    registry.CleanupCameraYpr();
    registry.CleanupCameraDistance();
    registry.CleanupCameraYprd();
    registry.CleanupCameraGetDistance();
    registry.CleanupCameraGetYaw();
    registry.CleanupCameraGetPitch();
    registry.CleanupCameraGetRoll();
    Check(runtime.events.empty(), "repeated cleanup does not release twice");
}

void TestDirectEntryAndReinitialization() {
    Runtime runtime;
    pal4::ScriptBuiltinRegistry registry(runtime);
    registry.InitializeImmBeginDescriptor();
    const auto first = registry.ImmBegin();
    runtime.events.clear();
    registry.RegisterImmBegin();
    Check(runtime.events.size() == 5u &&
              runtime.events[0].starts_with("alloc:"),
          "the original constructor resets fields before checking old pointers");
    Check(registry.ImmBegin().description != first.description &&
              runtime.exit_callbacks.empty(),
          "direct registration does not register a destructor");
}

void TestArenaAndLeaderRegistrations() {
    Runtime runtime;
    pal4::ScriptBuiltinRegistry registry(runtime);
    Check(registry.InitializeArenaLoad() == 7, "arena load atexit result");
    Check(registry.InitializeArenaReady() == 7, "arena ready atexit result");
    Check(registry.InitializeArenaReadyRestore() == 7,
          "arena ready restore atexit result");
    Check(registry.InitializeArenaHint() == 7, "arena hint atexit result");
    Check(registry.InitializeArenaComeFromHere() == 7,
          "arena origin atexit result");
    Check(registry.InitializePlayerSetLeader() == 7,
          "set leader atexit result");
    Check(runtime.exit_callbacks == std::vector<std::uint32_t>{
              pal4::ScriptBuiltinRegistry::kArenaLoadCleanup,
              pal4::ScriptBuiltinRegistry::kArenaReadyCleanup,
              pal4::ScriptBuiltinRegistry::kArenaReadyRestoreCleanup,
              pal4::ScriptBuiltinRegistry::kArenaHintCleanup,
              pal4::ScriptBuiltinRegistry::kArenaComeFromHereCleanup,
              pal4::ScriptBuiltinRegistry::kPlayerSetLeaderCleanup},
          "arena and leader destructors follow initializer order");
    CheckDescriptor(runtime, pal4::ScriptBuiltinRegistry::kArenaLoadDescriptor,
                    pal4::ScriptBuiltinRegistry::kArenaLoadHandler,
                    "void giArenaLoad( string &in, string &in, string &in, bool )",
                    214u, 0xb8dfc6c5u);
    CheckDescriptor(runtime, pal4::ScriptBuiltinRegistry::kArenaReadyDescriptor,
                    pal4::ScriptBuiltinRegistry::kArenaReadyHandler,
                    "void giArenaReady()", 88u, 0x1d3c2c71u);
    CheckDescriptor(runtime,
                    pal4::ScriptBuiltinRegistry::kArenaReadyRestoreDescriptor,
                    pal4::ScriptBuiltinRegistry::kArenaReadyRestoreHandler,
                    "void giArenaReadyRestore()", 137u, 0xff7dd426u);
    CheckDescriptor(runtime, pal4::ScriptBuiltinRegistry::kArenaHintDescriptor,
                    pal4::ScriptBuiltinRegistry::kArenaHintHandler,
                    "void giArenaHint()", 72u, 0xf944cce1u);
    CheckDescriptor(runtime,
                    pal4::ScriptBuiltinRegistry::kArenaComeFromHereDescriptor,
                    pal4::ScriptBuiltinRegistry::kArenaComeFromHereHandler,
                    "int giArenaComeFromHere( string &in, string &in )",
                    188u, 0x774918bfu);
    CheckDescriptor(runtime,
                    pal4::ScriptBuiltinRegistry::kPlayerSetLeaderDescriptor,
                    pal4::ScriptBuiltinRegistry::kPlayerSetLeaderHandler,
                    "void giPlayerSetLeader( int )", 148u, 0x44cbe2bfu);
    Check(runtime.events.size() == 36u, "six commands each perform six calls");
    for (std::size_t index = 0; index < 6u; ++index) {
        const auto base = index * 6u;
        Check(runtime.events[base].starts_with("alloc:") &&
                  runtime.events[base + 1u].starts_with("copy:") &&
                  runtime.events[base + 2u].starts_with("alloc:") &&
                  runtime.events[base + 3u].starts_with("copy:") &&
                  runtime.events[base + 4u].starts_with("register:") &&
                  runtime.events[base + 5u].starts_with("atexit:"),
              "arena descriptor registration preserves native call order");
    }
    const std::vector<pal4::ScriptBuiltinDescriptor> snapshots{
        registry.ArenaLoad(), registry.ArenaReady(),
        registry.ArenaReadyRestore(), registry.ArenaHint(),
        registry.ArenaComeFromHere(), registry.PlayerSetLeader()};
    runtime.events.clear();
    registry.CleanupPlayerSetLeader();
    registry.CleanupArenaComeFromHere();
    registry.CleanupArenaHint();
    registry.CleanupArenaReadyRestore();
    registry.CleanupArenaReady();
    registry.CleanupArenaLoad();
    Check(runtime.events.size() == 12u,
          "six destructors release both strings");
    for (std::size_t index = 0; index < snapshots.size(); ++index) {
        const auto& original = snapshots[snapshots.size() - 1u - index];
        Check(runtime.events[2u * index] ==
                  "free:" + std::to_string(original.signature) &&
                  runtime.events[2u * index + 1u] ==
                  "free:" + std::to_string(original.description),
              "each cleanup releases signature before description");
    }
    Check(registry.ArenaLoad().description == 0u &&
              registry.ArenaReady().signature_size == 0u &&
              registry.ArenaReadyRestore().description_size == 0u &&
              registry.ArenaHint().signature == 0u &&
              registry.ArenaComeFromHere().description == 0u &&
              registry.PlayerSetLeader().signature_size == 0u,
          "arena and leader cleanup clears string fields");
}

void TestSharedVirtualMemoryDestructor() {
    Runtime runtime;
    pal4::ScriptBuiltinRegistry registry(runtime);
    pal4::ScriptBuiltinDescriptor descriptor{};
    descriptor.description = 0x2300u;
    descriptor.description_size = 3u;
    descriptor.signature = 0x2400u;
    descriptor.signature_size = 4u;
    descriptor.callback_data[0] = 0xabcdefu;
    runtime.strings[descriptor.description] = "abc";
    runtime.strings[descriptor.signature] = "defg";
    registry.DeallocateVirtualMemory(descriptor);
    Check(runtime.events == std::vector<std::string>{
              "free:9216", "free:8960"},
          "shared destructor releases signature before description");
    Check(descriptor.description == 0u && descriptor.description_size == 0u &&
              descriptor.signature == 0u && descriptor.signature_size == 0u &&
              descriptor.callback_data[0] == 0xabcdefu,
          "shared destructor clears only its four string words");
    runtime.events.clear();
    registry.DeallocateVirtualMemory(descriptor);
    Check(runtime.events.empty(), "shared destructor skips null pointers");
}

void TestPlayerNpcAndSystemRegistrations() {
    Runtime runtime;
    pal4::ScriptBuiltinRegistry registry(runtime);
    Check(registry.InitializePlayerSetVisible() == 7, "player visibility atexit");
    Check(registry.InitializePlayerAttachCollision() == 7,
          "attach collision atexit");
    Check(registry.InitializePlayerDetachCollision() == 7,
          "detach collision atexit");
    Check(registry.InitializePlayerLock() == 7, "player lock atexit");
    Check(registry.InitializePlayerUnlock() == 7, "player unlock atexit");
    Check(registry.InitializeNpcSetVisible() == 7, "NPC visibility atexit");
    Check(registry.InitializeNpcCreate() == 7, "NPC create atexit");
    Check(registry.InitializeNpcDelete() == 7, "NPC delete atexit");
    Check(registry.InitializeSystemExchange() == 7, "system exchange atexit");
    Check(registry.InitializeMonsterStopPursuit() == 7,
          "monster pursuit atexit");

    struct Expected {
        std::uint32_t address;
        std::uint32_t handler;
        const char* signature;
        std::size_t description_size;
        std::uint32_t description_hash;
        std::uint32_t cleanup;
        const pal4::ScriptBuiltinDescriptor* descriptor;
    };
    const std::array<Expected, 10> expected{{
        {pal4::ScriptBuiltinRegistry::kPlayerSetVisibleDescriptor,
         pal4::ScriptBuiltinRegistry::kPlayerSetVisibleHandler,
         "void giPlayerSetVisible( int, bool )", 158u, 0x3247107cu,
         pal4::ScriptBuiltinRegistry::kPlayerSetVisibleCleanup,
         &registry.PlayerSetVisible()},
        {pal4::ScriptBuiltinRegistry::kPlayerAttachCollisionDescriptor,
         pal4::ScriptBuiltinRegistry::kPlayerAttachCollisionHandler,
         "void giPlayerAttachCollision( int )", 207u, 0x2796f4c5u,
         pal4::ScriptBuiltinRegistry::kPlayerAttachCollisionCleanup,
         &registry.PlayerAttachCollision()},
        {pal4::ScriptBuiltinRegistry::kPlayerDetachCollisionDescriptor,
         pal4::ScriptBuiltinRegistry::kPlayerDetachCollisionHandler,
         "void giPlayerDetachCollision( int )", 130u, 0x033362d8u,
         pal4::ScriptBuiltinRegistry::kPlayerDetachCollisionCleanup,
         &registry.PlayerDetachCollision()},
        {pal4::ScriptBuiltinRegistry::kPlayerLockDescriptor,
         pal4::ScriptBuiltinRegistry::kPlayerLockHandler,
         "void giPlayerLock()", 66u, 0xf79f1095u,
         pal4::ScriptBuiltinRegistry::kPlayerLockCleanup,
         &registry.PlayerLock()},
        {pal4::ScriptBuiltinRegistry::kPlayerUnlockDescriptor,
         pal4::ScriptBuiltinRegistry::kPlayerUnlockHandler,
         "void giPlayerUnLock()", 58u, 0x524c9ea4u,
         pal4::ScriptBuiltinRegistry::kPlayerUnlockCleanup,
         &registry.PlayerUnlock()},
        {pal4::ScriptBuiltinRegistry::kNpcSetVisibleDescriptor,
         pal4::ScriptBuiltinRegistry::kNpcSetVisibleHandler,
         "void giSetNpcVisible( string &in, bool visible )",
         138u, 0x7b001dc1u,
         pal4::ScriptBuiltinRegistry::kNpcSetVisibleCleanup,
         &registry.NpcSetVisible()},
        {pal4::ScriptBuiltinRegistry::kNpcCreateDescriptor,
         pal4::ScriptBuiltinRegistry::kNpcCreateHandler,
         "void giNpcCreate( string &in, string &in, float, float, float )",
         226u, 0x0a4650dbu,
         pal4::ScriptBuiltinRegistry::kNpcCreateCleanup,
         &registry.NpcCreate()},
        {pal4::ScriptBuiltinRegistry::kNpcDeleteDescriptor,
         pal4::ScriptBuiltinRegistry::kNpcDeleteHandler,
         "void giNpcDelete( string &in )", 111u, 0x1c7126e0u,
         pal4::ScriptBuiltinRegistry::kNpcDeleteCleanup,
         &registry.NpcDelete()},
        {pal4::ScriptBuiltinRegistry::kSystemExchangeDescriptor,
         pal4::ScriptBuiltinRegistry::kSystemExchangeHandler,
         "void giSystemExchange( int )", 85u, 0x73ea7124u,
         pal4::ScriptBuiltinRegistry::kSystemExchangeCleanup,
         &registry.SystemExchange()},
        {pal4::ScriptBuiltinRegistry::kMonsterStopPursuitDescriptor,
         pal4::ScriptBuiltinRegistry::kMonsterStopPursuitHandler,
         "void giMonsterStopPursuit()", 56u, 0xc0da49e6u,
         pal4::ScriptBuiltinRegistry::kMonsterStopPursuitCleanup,
         &registry.MonsterStopPursuit()},
    }};
    Check(runtime.exit_callbacks.size() == expected.size(),
          "ten matching CRT cleanup callbacks were registered");
    Check(runtime.events.size() == expected.size() * 6u,
          "ten commands each perform six ordered runtime calls");
    for (std::size_t index = 0; index < expected.size(); ++index) {
        const auto& entry = expected[index];
        CheckDescriptor(runtime, entry.address, entry.handler,
                        entry.signature, entry.description_size,
                        entry.description_hash);
        Check(runtime.exit_callbacks[index] == entry.cleanup,
              "static initializer installs matching cleanup");
        const auto base = index * 6u;
        Check(runtime.events[base].starts_with("alloc:") &&
                  runtime.events[base + 1u].starts_with("copy:") &&
                  runtime.events[base + 2u].starts_with("alloc:") &&
                  runtime.events[base + 3u].starts_with("copy:") &&
                  runtime.events[base + 4u] ==
                      "register:" + std::to_string(entry.address) &&
                  runtime.events[base + 5u] ==
                      "atexit:" + std::to_string(entry.cleanup),
              "registration and atexit retain command order");
    }
    std::vector<pal4::ScriptBuiltinDescriptor> snapshots;
    for (const auto& entry : expected)
        snapshots.push_back(*entry.descriptor);
    runtime.events.clear();
    registry.CleanupMonsterStopPursuit();
    registry.CleanupSystemExchange();
    registry.CleanupNpcDelete();
    registry.CleanupNpcCreate();
    registry.CleanupNpcSetVisible();
    registry.CleanupPlayerUnlock();
    registry.CleanupPlayerLock();
    registry.CleanupPlayerDetachCollision();
    registry.CleanupPlayerAttachCollision();
    registry.CleanupPlayerSetVisible();
    Check(runtime.events.size() == snapshots.size() * 2u,
          "each script command releases both strings");
    for (std::size_t index = 0; index < snapshots.size(); ++index) {
        const auto& original = snapshots[snapshots.size() - 1u - index];
        Check(runtime.events[2u * index] ==
                  "free:" + std::to_string(original.signature) &&
                  runtime.events[2u * index + 1u] ==
                  "free:" + std::to_string(original.description),
              "ten cleanup paths release signature before description");
    }
    for (const auto& entry : expected)
        Check(entry.descriptor->description == 0u &&
                  entry.descriptor->description_size == 0u &&
                  entry.descriptor->signature == 0u &&
                  entry.descriptor->signature_size == 0u &&
                  entry.descriptor->callback_data[0] == entry.handler,
              "cleanup clears strings and preserves callback payload");
}

void TestExtendedRegistrations() {
    Runtime runtime;
    pal4::ScriptBuiltinRegistry registry(runtime);
    constexpr std::array<const char*, 158> signatures{
        "void giCameraSetMode( int, bool )",
        "int giGetGoodsOpenCondition( int )",
        "void\tgiSetGoodsOpenCondition( int, int )",
        "void giNpcPauseBeh( string &in )",
        "void giNpcResumeBeh( string &in )",
        "void giOpenWeather( int )",
        "void giCloseWeather()",
        "void giSetMinimapExpmode( int )",
        "int giGetRandnum( int, int )",
        "void giSetTempGameState( int )",
        "void giFlushTailYAngle()",
        "void giAddCombatMonster( int, int )",
        "void giConfigCombatParam( bool, int, int, int )",
        "void giConfigCombatBgm( string &in )",
        "void giConfigCombatCamera( string &in )",
        "void giStartCombat( string &in )",
        "void giSetObjectVisible( string &in, bool visible )",
        "void giAddProperty( int, int, bool )",
        "void giDelProperty( int, int, bool )",
        "void giPlayerInTeam( int, bool )",
        "void giPlayerOutTeam( int, bool )",
        "void giGOMTouch( string &in )",
        "void giCameraSetCollide( bool )",
        "void giCameraSeekToPlayer()",
        "void giCameraAutoSeek( bool )",
        "void giPlayerSetAttr( int, int, int )",
        "int giPlayerGetLeader()",
        "void giSetPlayerLevel( int, int )",
        "void giAddPlayerEquip( int, int )",
        "void giOpenMovieFlag( int )",
        "void giAddQuestComplatePercentage( int )",
        "void giAddEquipment( int, bool )",
        "void giPlayerCurrentSetVisible( bool )",
        "void giSetFullHP()",
        "void giSetFullMP()",
        "int giGetPlayerLevel( int )",
        "void giGotoLogo()",
        "void giPlayerUnHoldAct( int )",
        "void giNpcUnHoldAct( string &in )",
        "void giCameraSetDistOptEnable( bool )",
        "void giMonsterSetHide( string &in, bool )",
        "void giGameObjectSetResearch( string &in, bool )",
        "void giSetPortrait( string &in, bool )",
        "void giBGMConfigSetMusic( string &in )",
        "bool giBGMConfigIsInArea( string &in )",
        "void giScriptMusicMute( bool )",
        "void giScriptMusicPlay( string &in, int, float, float )",
        "void giScriptMusicStop( int, float )",
        "void giArenaMusicStop( float )",
        "void giWorldMapSetState( string &in, int )",
        "int giGetPuzzleGameResult()",
        "void giAlwaysJump( bool )",
        "int gi2DSoundPlay( string &in, int )",
        "void gi2DSoundStop()",
        "void gi2DSoundStopID( int )",
        "void giCGEffPlay( int )",
        "void giCGEffStop()",
        "void giEffectPlay( string &in, int, float, float, float )",
        "void giEffectPlayWithPlayer( string &in, int, int )",
        "void giEffectPlayWithCurrentPlayer( string &in, int )",
        "void giEffectPlayWithNPC( string &in, int, string &in )",
        "void giEffectPlayWithOBJ( string &in, int, string &in )",
        "void giEffectStopWithOBJ( string &in )",
        "void giShowHint( string &in, float, float )",
        "void giPlayerAddSkill( int, int, bool )",
        "void giMonsterSetVisible( string &in, bool )",
        "void giPlayerRandomPosition( int, float, float )",
        "void giPlayerCurrentRandomPosition( float, float )",
        "void giEventVolumeVisible( string &in, bool )",
        "void giAllPlayerGarb2()",
        "void giPlayerGarb2( int )",
        "void giAllPlayerGarb1()",
        "void giPlayerGarb1( int )",
        "bool giGetVisibleObject( string &in )",
        "bool giGetVisibleMonster( string &in )",
        "bool giCheckPackProperty( int, int )",
        "void giGrantSystemUi( int, bool )",
        "void giOpenSystemUi( int )",
        "void giGrantSmithSystem( bool, bool, bool, bool )",
        "void giGrantMagicSystem( bool, bool )",
        "bool giCheckMagicMastered()",
        "void giSelectDialogAddItem( string &in )",
        "int giSelectDialogGetLastSelect()",
        "void giGOBAttachToPlayer( string &in, string &in, int, bool )",
        "void giGOBAttachToCurrentPlayer( string &in, string &in, bool )",
        "void giGOBDetachFromPlayer( int )",
        "void giGOBDetachFromCurrentPlayer()",
        "void giEffectAttachToPlayer( int, string &in, bool )",
        "void giEffectAttachToCurrentPlayer( string &in, bool )",
        "void giEffectDetachFromPlayer( int )",
        "void giEffectDetachFromCurrentPlayer()",
        "void giEffectAttachToNpc( string &in, string &in, bool )",
        "void giEffectDetachFromNpc( string &in )",
        "void giGOBAttachToNpc( string &in, string &in, string &in, bool )",
        "void giGOBDetachFromNPC( string &in )",
        "void giGOBSetPosition( string &in, float, float, float )",
        "void giScriptClearCTXButCurrent()",
        "void giAddMoney( int, bool )",
        "void giPayMoney( int, bool )",
        "void giHideGASkillObject()",
        "void giShowSignpost()",
        "void giPlayerSetEmotion( int, string &in )",
        "void giPlayerResetEmotion( int )",
        "void giPlayerCurrentSetEmotion( string &in )",
        "void giPlayerCurrentResetEmotion()",
        "void giLINGSHALegsInjured( string &in )",
        "void giLINGSHALegsHealing()",
        "void giNpcSetEmotion( string &in, string &in )",
        "void giNpcResetEmotion( string &in )",
        "int giGetPropertyNumb( int )",
        "float giPlayerCurrentGetPosX()",
        "float giPlayerCurrentGetPosY()",
        "float giPlayerCurrentGetPosZ()",
        "string giArenaGetName()",
        "string giArenaGetArea()",
        "void giArenaSkillEnable( bool )",
        "void giShowInnDialog( bool )",
        "int giGetInnDialogResult()",
        "void giPlayerTakeARest()",
        "bool giIsNightTime()",
        "void giPlayerSetPosRot( int, float, float, float, float )",
        "void giPlayerSetPosRot( string& in, float, float, float, float )",
        "void giStartUiTimer( int, string& in )",
        "void giPlayerForbidenSkill( int, int, bool )",
        "int giGetMoney()",
        "void giSelectDialogSetDefaultSelect( int )",
        "void giShowQuestDialog( string& in )",
        "int giGetQuestDialogResult()",
        "void giResetPlayerToJumpStart()",
        "void giGOBReset( string &in )",
        "bool giCheckEquipInInventory( int )",
        "void giRemoveEquipment( int, bool )",
        "void giAddPrescription( int, bool )",
        "void giEnableShadow( bool )",
        "void giAddRoundTimes()",
        "void giTimeScript( float, string &in )",
        "void giTimeScriptTerminate()",
        "void giConfigCombatGroundCamera( string &in )",
        "void giAddPlayerFavor( int, int, int )",
        "int giGetPalTestResult( int )",
        "void giSetMinimapLevel( int )",
        "void giPetShow( bool )",
        "void giNpcAttachEffect( string &in, string &in, int )",
        "void giNpcDetachEffect( string &in )",
        "void giMstAttachEffect( string &in, string &in, int )",
        "void giMstDetachEffect( string &in )",
        "void giPlayerHookEffect( int, string &in, int )",
        "void giPlayerDetachEffect( int )",
        "int giCommonDialogGetLastSelect()",
        "void giConfigCombatVipMonster( int )",
        "void giEnableSTS( int )",
        "void giClearUiTimer()",
        "void giPauseUiTimer()",
        "void giResumeUiTimer()",
        "void giSetFullRage()",
        "int giUiTimerGetSaveData()",
        "void giScriptMusicPause()",
        "void giScriptMusicResume()"};
    constexpr std::array<std::size_t, 158> description_sizes{
        186u, 136u, 159u, 38u, 38u, 131u, 39u, 92u, 161u, 100u,
        49u, 118u, 293u, 81u, 103u, 71u, 141u, 157u, 157u, 115u,
        115u, 83u, 85u, 65u, 107u, 224u, 117u, 155u, 183u, 85u,
        85u, 115u, 89u, 63u, 63u, 157u, 51u, 93u, 92u, 58u,
        53u, 61u, 144u, 94u, 128u,
        82u, 205u, 172u, 87u, 173u,
        112u, 128u, 139u, 68u, 55u,
        240u, 51u, 203u, 186u, 126u,
        151u, 153u, 117u, 124u, 206u,
        127u, 226u, 153u, 123u, 63u,
        136u, 63u, 136u, 111u, 111u,
        179u, 204u, 142u, 291u, 185u,
        105u, 96u, 132u, 260u, 187u,
        132u, 63u, 210u, 137u, 61u,
        61u, 167u, 60u, 217u, 89u,
        87u, 205u, 133u, 133u, 65u,
        57u, 162u, 128u, 89u, 55u,
        83u, 51u, 118u, 82u, 93u,
        82u, 82u, 82u, 55u, 55u,
        59u, 85u, 120u, 64u, 97u,
        75u, 74u, 151u, 175u, 83u,
        225u, 69u, 118u, 63u, 112u,
        97u, 115u, 115u, 90u, 43u,
        255u, 28u, 95u, 159u, 163u,
        89u, 97u, 184u, 77u, 186u,
        79u, 227u, 120u, 104u, 101u,
        142u, 61u, 61u, 61u, 63u,
        96u, 61u, 61u};
    constexpr std::array<std::uint32_t, 158> description_hashes{
        0xbdfb7510u, 0xe1c4ad3du, 0xb00155c0u, 0x3de10e5fu,
        0xded4563bu, 0xec5314e3u, 0xcf1d023du, 0x01801fc7u,
        0x61154f57u, 0x8ce45eefu, 0x99f3bb5fu, 0x3d3eb6fcu,
        0xec819e12u, 0x7402c7ebu, 0xf70e083bu, 0x0f662ff8u,
        0x0dc0a9f5u, 0x25a2b365u, 0xe4a710ddu, 0xd497d9dfu,
        0xd497d9dfu, 0x3c012759u, 0x9b48a3acu, 0x56097323u,
        0x77b008ffu, 0xa1202890u, 0x3fa882d6u, 0xbed8ec20u,
        0x9de8cf7bu, 0x57b878d1u, 0xc6311cdcu, 0x8564f13bu,
        0xa6691953u, 0xcc62beb3u, 0xe29c3676u, 0xbd7a7932u,
        0x13aec200u, 0x23a963bfu, 0x7441c794u, 0xe2b508c6u,
        0xaabbf94eu, 0x34aa9aacu, 0x5949b963u, 0xe5b59c50u,
        0xf1cd8523u, 0x9c2535bcu, 0xdac3eecau, 0xb8dc0917u,
        0xf25a70aeu, 0x22e3d58eu, 0x7323b55du, 0x2c49362fu,
        0xcb4b96f1u, 0xfe30aba0u, 0x72118854u, 0x6db26f75u,
        0x724bc28bu, 0xda2318c5u, 0x18371feeu, 0xc212ffbau,
        0x4ba1f462u, 0x63247fa0u, 0x5f64c809u, 0x2f02c125u,
        0x81621626u, 0x694d029du, 0x19dc95eau, 0x195f713au,
        0xaaa266e5u, 0x76dff8fu, 0x19f00bf2u, 0x62789dccu,
        0x14441973u, 0xedac3c1cu, 0x8f1b869u, 0x9c1122c8u,
        0x35b09580u, 0x900692e2u, 0xa4ac3253u, 0xf9d55fb3u,
        0x788f8faeu, 0xa6a82170u, 0x161774e6u, 0xa763f6e8u,
        0xb5f4d1aeu,
        0x292a2784u, 0x57e6da30u, 0xa2e677deu, 0x23454dc4u,
        0x51b96d81u, 0xa64ace38u, 0x1f61c7e8u, 0x33ca06a4u,
        0x771291ccu, 0xf75ccaa6u, 0x2e07c184u, 0x9f361ffau,
        0x3e60acc3u, 0x598dafaeu, 0xc59fcabu, 0x4dcdcbd9u,
        0x82e47caau, 0x5fe30921u, 0x4994f4acu, 0xa0ac750au,
        0x9b0c568bu, 0xea677efau, 0x63b69232u, 0xd88108f4u,
        0xaa6e264du,
        0xab4053e3u, 0x834d2bb5u, 0xa7358ee7u, 0x300caf77u,
        0x91e55f67u, 0xded74319u, 0x1066a78eu, 0x8e586cd0u,
        0x76acc512u, 0x8cb71a3au, 0xf870e997u, 0x91b31beeu,
        0xb669ef49u, 0x2642ef79u, 0x4f60ec6u, 0x4c4a6a66u,
        0x34a3ac4fu, 0x7803c045u, 0xc369eee1u, 0x1bc0ae3bu,
        0x7dd095d4u, 0xe52d4467u, 0xdf63a9bu, 0x6b7e8fb2u,
        0xc5bfe4b7u, 0x506b226bu, 0x1d1721bdu, 0xdcf602c3u,
        0x231dff6u, 0x5ae0c5c6u, 0xb36d81b6u, 0x8a4e4562u,
        0x76c7c5aeu, 0x336824c6u, 0xa7dce17au, 0xa3777c26u,
        0x3a821890u, 0x28516f6au, 0x35219373u, 0x239f41a2u,
        0xf83072c2u, 0xed4b7aa8u, 0x806fd543u, 0xcb916475u,
        0x63f2e04du, 0x9535ec75u, 0xf9c1b5d8u, 0x79eaf1feu};

    for (std::size_t index = 0; index < signatures.size(); ++index) {
        const auto command = static_cast<pal4::ScriptBuiltinExtension>(index);
        Check(registry.InitializeExtension(command) == 7,
              "extended initializer propagates atexit result");
        CheckDescriptor(runtime,
                        pal4::ScriptBuiltinRegistry::kExtensionDescriptors[index],
                        pal4::ScriptBuiltinRegistry::kExtensionHandlers[index],
                        signatures[index], description_sizes[index],
                        description_hashes[index]);
        Check(runtime.exit_callbacks[index] ==
                  pal4::ScriptBuiltinRegistry::kExtensionCleanups[index],
              "extended command installs matching cleanup callback");
        const auto base = index * 6u;
        Check(runtime.events[base].starts_with("alloc:") &&
                  runtime.events[base + 1u].starts_with("copy:") &&
                  runtime.events[base + 2u].starts_with("alloc:") &&
                  runtime.events[base + 3u].starts_with("copy:") &&
                  runtime.events[base + 4u] ==
                      "register:" + std::to_string(
                          pal4::ScriptBuiltinRegistry::kExtensionDescriptors[index]) &&
                  runtime.events[base + 5u] ==
                      "atexit:" + std::to_string(
                          pal4::ScriptBuiltinRegistry::kExtensionCleanups[index]),
              "extended command registration precedes atexit");
    }
    Check(runtime.events.size() == signatures.size() * 6u,
          "each extended initializer performs six runtime calls");
    const auto& in_team = registry.Extension(
        pal4::ScriptBuiltinExtension::PlayerInTeam);
    const auto& out_team = registry.Extension(
        pal4::ScriptBuiltinExtension::PlayerOutTeam);
    Check(in_team.description != out_team.description &&
              runtime.strings.at(in_team.description) ==
                  runtime.strings.at(out_team.description),
          "team entry and exit copy the same original description source");

    std::array<pal4::ScriptBuiltinDescriptor, 158> snapshots{};
    for (std::size_t index = 0; index < snapshots.size(); ++index)
        snapshots[index] = registry.Extension(
            static_cast<pal4::ScriptBuiltinExtension>(index));
    runtime.events.clear();
    for (std::size_t index = snapshots.size(); index > 0u; --index)
        registry.CleanupExtension(
            static_cast<pal4::ScriptBuiltinExtension>(index - 1u));
    Check(runtime.events.size() == snapshots.size() * 2u,
          "each extended cleanup releases two strings");
    for (std::size_t index = 0; index < snapshots.size(); ++index) {
        const auto& original = snapshots[snapshots.size() - 1u - index];
        Check(runtime.events[2u * index] ==
                  "free:" + std::to_string(original.signature) &&
                  runtime.events[2u * index + 1u] ==
                  "free:" + std::to_string(original.description),
              "extended cleanup releases signature before description");
    }
    for (std::size_t index = 0; index < snapshots.size(); ++index) {
        const auto& descriptor = registry.Extension(
            static_cast<pal4::ScriptBuiltinExtension>(index));
        Check(descriptor.description == 0u &&
                  descriptor.description_size == 0u &&
                  descriptor.signature == 0u &&
                  descriptor.signature_size == 0u &&
                  descriptor.callback_data[0] ==
                      pal4::ScriptBuiltinRegistry::kExtensionHandlers[index],
              "extended cleanup clears strings but retains callback");
    }
    runtime.events.clear();
    for (std::size_t index = 0; index < snapshots.size(); ++index)
        registry.CleanupExtension(
            static_cast<pal4::ScriptBuiltinExtension>(index));
    Check(runtime.events.empty(), "repeated extended cleanup is a no-op");
}

void TestExtensionDirectEntryAndBounds() {
    Runtime runtime;
    pal4::ScriptBuiltinRegistry registry(runtime);
    constexpr auto command = pal4::ScriptBuiltinExtension::CameraSetMode;
    registry.RegisterExtension(command);
    Check(runtime.exit_callbacks.empty() && runtime.events.size() == 5u,
          "direct extended registration does not install CRT cleanup");
    registry.CleanupExtension(command);
    runtime.events.clear();
    for (const auto invalid : {158u, 0xffu}) {
        const auto extension = static_cast<pal4::ScriptBuiltinExtension>(invalid);
        bool threw = false;
        try {
            (void)registry.InitializeExtension(extension);
        } catch (const std::out_of_range&) {
            threw = true;
        }
        Check(threw && runtime.events.empty(),
              "invalid extension initializer has no side effects");
        threw = false;
        try {
            (void)registry.Extension(extension);
        } catch (const std::out_of_range&) {
            threw = true;
        }
        Check(threw, "invalid extension descriptor lookup is rejected");
    }
}

void TestStaticInitializerCluster() {
    using Command = pal4::ScriptBuiltinStaticCommand;
    Runtime runtime;
    pal4::ScriptBuiltinRegistry registry(runtime);
    registry.RegisterImmBegin();
    const auto original_begin = registry.ImmBegin();
    runtime.events.clear();

    constexpr std::array commands{
        Command::ImmBegin, Command::ImmEnd, Command::NewGame,
        Command::CameraYpr, Command::CameraDistance, Command::CameraYprd,
        Command::CameraGetDistance, Command::CameraGetYaw,
        Command::CameraGetPitch, Command::CameraGetRoll,
        Command::ArenaLoad, Command::ArenaReady, Command::ArenaReadyRestore,
        Command::ArenaHint, Command::ArenaComeFromHere,
        Command::PlayerSetLeader, Command::PlayerSetVisible,
        Command::PlayerAttachCollision, Command::PlayerDetachCollision,
        Command::PlayerLock, Command::PlayerUnlock,
        Command::NpcSetVisible, Command::NpcCreate, Command::NpcDelete,
        Command::SystemExchange, Command::MonsterStopPursuit};
    constexpr std::array handlers{
        pal4::ScriptBuiltinRegistry::kImmBeginHandler,
        pal4::ScriptBuiltinRegistry::kImmEndHandler,
        pal4::ScriptBuiltinRegistry::kNewGameHandler,
        pal4::ScriptBuiltinRegistry::kCameraYprHandler,
        pal4::ScriptBuiltinRegistry::kCameraDistanceHandler,
        pal4::ScriptBuiltinRegistry::kCameraYprdHandler,
        pal4::ScriptBuiltinRegistry::kCameraGetDistanceHandler,
        pal4::ScriptBuiltinRegistry::kCameraGetYawHandler,
        pal4::ScriptBuiltinRegistry::kCameraGetPitchHandler,
        pal4::ScriptBuiltinRegistry::kCameraGetRollHandler,
        pal4::ScriptBuiltinRegistry::kArenaLoadHandler,
        pal4::ScriptBuiltinRegistry::kArenaReadyHandler,
        pal4::ScriptBuiltinRegistry::kArenaReadyRestoreHandler,
        pal4::ScriptBuiltinRegistry::kArenaHintHandler,
        pal4::ScriptBuiltinRegistry::kArenaComeFromHereHandler,
        pal4::ScriptBuiltinRegistry::kPlayerSetLeaderHandler,
        pal4::ScriptBuiltinRegistry::kPlayerSetVisibleHandler,
        pal4::ScriptBuiltinRegistry::kPlayerAttachCollisionHandler,
        pal4::ScriptBuiltinRegistry::kPlayerDetachCollisionHandler,
        pal4::ScriptBuiltinRegistry::kPlayerLockHandler,
        pal4::ScriptBuiltinRegistry::kPlayerUnlockHandler,
        pal4::ScriptBuiltinRegistry::kNpcSetVisibleHandler,
        pal4::ScriptBuiltinRegistry::kNpcCreateHandler,
        pal4::ScriptBuiltinRegistry::kNpcDeleteHandler,
        pal4::ScriptBuiltinRegistry::kSystemExchangeHandler,
        pal4::ScriptBuiltinRegistry::kMonsterStopPursuitHandler};
    constexpr std::array signatures{
        "void giIMMBegin()", "void giIMMEnd()", "void giNewGame()",
        "void giCameraCtrlYPR( float, float, float, bool )",
        "void giCameraCtrlDist( float, bool )",
        "void giCameraCtrlYPRD( float, float, float, float, bool )",
        "float giCameraGetDist()", "float giCameraGetYaw()",
        "float giCameraGetPitch()", "float giCameraGetRoll()",
        "void giArenaLoad( string &in, string &in, string &in, bool )",
        "void giArenaReady()", "void giArenaReadyRestore()",
        "void giArenaHint()",
        "int giArenaComeFromHere( string &in, string &in )",
        "void giPlayerSetLeader( int )",
        "void giPlayerSetVisible( int, bool )",
        "void giPlayerAttachCollision( int )",
        "void giPlayerDetachCollision( int )", "void giPlayerLock()",
        "void giPlayerUnLock()",
        "void giSetNpcVisible( string &in, bool visible )",
        "void giNpcCreate( string &in, string &in, float, float, float )",
        "void giNpcDelete( string &in )",
        "void giSystemExchange( int )",
        "void giMonsterStopPursuit()"};
    constexpr std::array<std::size_t, 26> description_sizes{
        29u, 29u, 63u, 236u, 146u, 274u, 89u, 91u, 91u, 91u,
        214u, 88u, 137u, 72u, 188u, 148u, 158u, 207u, 130u, 66u,
        58u, 138u, 226u, 111u, 85u, 56u};
    constexpr std::array<std::uint32_t, 26> description_hashes{
        0x0c749a2au, 0x2444d499u, 0x1b941567u, 0x75d7861au,
        0x2b9b8de4u, 0x0dea65b8u, 0x20e69d2du, 0x3fb1fca7u,
        0x7dcd7f61u, 0x6b097829u, 0xb8dfc6c5u, 0x1d3c2c71u,
        0xff7dd426u, 0xf944cce1u, 0x774918bfu, 0x44cbe2bfu,
        0x3247107cu, 0x2796f4c5u, 0x033362d8u, 0xf79f1095u,
        0x524c9ea4u, 0x7b001dc1u, 0x0a4650dbu, 0x1c7126e0u,
        0x73ea7124u, 0xc0da49e6u};
    for (std::size_t i = 0; i < commands.size(); ++i) {
        Check(registry.InitializeStatic(commands[i]) == 7,
              "static initializer forwards CRT result");
        CheckDescriptor(runtime,
                        pal4::ScriptBuiltinRegistry::kStaticDescriptors[i],
                        handlers[i], signatures[i], description_sizes[i],
                        description_hashes[i]);
    }
    Check(runtime.exit_callbacks == std::vector<std::uint32_t>(
              pal4::ScriptBuiltinRegistry::kStaticCleanupCallbacks.begin(),
              pal4::ScriptBuiltinRegistry::kStaticCleanupCallbacks.end()),
          "static initializers register matching cleanup thunks");
    Check(runtime.events.size() == 156u,
          "26 static registrations each perform six operations");
    for (std::size_t i = 0; i < commands.size(); ++i) {
        const auto base = 6u * i;
        Check(runtime.events[base].starts_with("alloc:") &&
                  runtime.events[base + 1u].starts_with("copy:") &&
                  runtime.events[base + 2u].starts_with("alloc:") &&
                  runtime.events[base + 3u].starts_with("copy:") &&
                  runtime.events[base + 4u] == "register:" + std::to_string(
                      pal4::ScriptBuiltinRegistry::kStaticDescriptors[i]) &&
                  runtime.events[base + 5u] == "atexit:" + std::to_string(
                      pal4::ScriptBuiltinRegistry::kStaticCleanupCallbacks[i]),
              "static registration and cleanup callback order");
    }
    Check(registry.StaticDescriptor(Command::ImmBegin).description !=
              original_begin.description &&
              registry.ImmBegin().description == original_begin.description,
          "second static descriptor does not replace original registry state");

    std::array<pal4::ScriptBuiltinDescriptor, 26> snapshots{};
    for (std::size_t i = 0; i < commands.size(); ++i)
        snapshots[i] = registry.StaticDescriptor(commands[i]);
    runtime.events.clear();
    for (std::size_t i = commands.size(); i != 0u; --i)
        registry.CleanupStatic(commands[i - 1u]);
    Check(runtime.events.size() == 52u,
          "all 26 static descriptors release two strings");
    for (std::size_t i = 0; i < commands.size(); ++i) {
        const auto& old = snapshots[commands.size() - 1u - i];
        Check(runtime.events[2u * i] ==
                  "free:" + std::to_string(old.signature) &&
                  runtime.events[2u * i + 1u] ==
                  "free:" + std::to_string(old.description),
              "each static cleanup releases signature before description");
        const auto& current = registry.StaticDescriptor(
            commands[commands.size() - 1u - i]);
        Check(current.description == 0u && current.signature == 0u &&
                  current.callback_data[0] ==
                      handlers[commands.size() - 1u - i],
              "static cleanup clears strings and keeps callback payload");
    }
    runtime.events.clear();
    registry.CleanupStatic(Command::ImmBegin);
    Check(runtime.events.empty(), "repeat static cleanup is inert");
    const auto registered_callbacks = runtime.exit_callbacks.size();
    registry.RegisterStatic(Command::ImmBegin);
    Check(runtime.events.size() == 5u &&
              runtime.exit_callbacks.size() == registered_callbacks,
          "direct static registration does not install CRT callback");
    registry.CleanupStatic(Command::ImmBegin);

    runtime.events.clear();
    for (const auto invalid : {26u, 0xffu}) {
        bool threw = false;
        try {
            (void)registry.InitializeStatic(static_cast<Command>(invalid));
        } catch (const std::out_of_range&) {
            threw = true;
        }
        Check(threw && runtime.events.empty(),
              "unknown static command has no side effects");
    }
}

}  // namespace

int main() {
    TestRegistrationAndInitializerOrder();
    TestCleanupOrderAndZeroing();
    TestDirectEntryAndReinitialization();
    TestArenaAndLeaderRegistrations();
    TestSharedVirtualMemoryDestructor();
    TestPlayerNpcAndSystemRegistrations();
    TestExtendedRegistrations();
    TestExtensionDirectEntryAndBounds();
    TestStaticInitializerCluster();
}
