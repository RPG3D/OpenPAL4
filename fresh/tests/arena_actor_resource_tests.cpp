#include "pal4/arena_actor_resource.h"

#include <bit>
#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <vector>

namespace {

using Actor = pal4::ArenaActorResource;
using Call = pal4::ArenaActorCall;

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IArenaActorRuntime {
    struct Invocation {
        pal4::ArenaActorCall operation;
        std::vector<std::uint32_t> arguments;
        bool operator==(const Invocation&) const = default;
    };
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<Invocation> calls;
    std::vector<std::pair<std::uint32_t, std::uint32_t>> writes;
    std::int32_t marker = -1;
    bool load_succeeds = true;

    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        writes.emplace_back(address, value);
        words[address] = value;
    }
    std::uint32_t Call(pal4::ArenaActorCall operation,
                       std::initializer_list<std::uint32_t> arguments) override {
        calls.push_back({operation, arguments});
        switch (operation) {
        case pal4::ArenaActorCall::GetGameInstance: return 0x9000u;
        case pal4::ArenaActorCall::GetArenaManager: return 0xa000u;
        case pal4::ArenaActorCall::CleanupClump: return 0xbeefu;
        case pal4::ArenaActorCall::ProcessAnimation: return 0xcafeu;
        case pal4::ArenaActorCall::UpdateInterpolator: return 0xd00du;
        case pal4::ArenaActorCall::FindScriptMarker:
            return static_cast<std::uint32_t>(marker);
        case pal4::ArenaActorCall::LoadClump:
            return load_succeeds ? 1u : 0u;
        default: return 0u;
        }
    }
};

void ConstructAndDestroy() {
    Runtime runtime;
    constexpr std::uint32_t object = 0x6000u;
    Check(Actor::Construct(object, runtime) == object &&
              runtime.calls == (std::vector<Runtime::Invocation>{
                  {Call::ConstructResourceManager, {object}},
                  {Call::InitializeObjectState,
                   {object + Actor::kObjectStateOffset}}}) &&
              runtime.writes ==
                  (std::vector<std::pair<std::uint32_t, std::uint32_t>>{
                      {object + Actor::kObjectStateOffset,
                       Actor::kObjectStateVtable},
                      {object, Actor::kResourceVtable}}),
          "construction initializes both objects and their vtables");
    runtime.calls.clear();
    runtime.writes.clear();
    Actor::Destroy(object, runtime);
    Check(runtime.calls == (std::vector<Runtime::Invocation>{
              {Call::CleanupObjectState,
               {object + Actor::kObjectStateOffset}},
              {Call::DestroyResourceManager, {object}}}) &&
              runtime.writes ==
                  (std::vector<std::pair<std::uint32_t, std::uint32_t>>{
                      {object, Actor::kResourceVtable},
                      {object + Actor::kObjectStateOffset,
                       Actor::kObjectStateVtable}}),
          "destruction restores vtables and tears down derived then base");
}

void CleanupRenderAndUpdate() {
    Runtime runtime;
    constexpr std::uint32_t object = 0x6000u;
    Check(Actor::Cleanup(object, runtime) == 0xbeefu &&
              runtime.calls == (std::vector<Runtime::Invocation>{
                  {Call::CleanupObjectData,
                   {object + Actor::kObjectStateOffset}},
                  {Call::GetGameInstance, {}},
                  {Call::CleanupClump, {object, 0x91acu}}}),
          "cleanup passes the game resource host at object offset 0x1ac (IDA 520DB3)");
    runtime.calls.clear();
    Check(Actor::RenderAndAnimate(object, 7u, runtime) == 0xcafeu &&
              runtime.calls == (std::vector<Runtime::Invocation>{
                  {Call::RenderClump, {object, 7u}},
                  {Call::ProcessAnimation,
                   {object + Actor::kObjectStateOffset, 7u}}}),
          "render runs before animation and forwards its result");
    runtime.calls.clear();
    constexpr float delta = 0.375f;
    const auto bits = std::bit_cast<std::uint32_t>(delta);
    Check(Actor::Update(delta, runtime) == 0xd00du &&
              runtime.calls == (std::vector<Runtime::Invocation>{
                  {Call::UpdateAndRenderClump, {bits}},
                  {Call::UpdateInterpolator, {bits}}}),
          "float update passes identical bits to both native stages");
}

void ScriptLoadBranches() {
    Runtime runtime;
    constexpr std::uint32_t object = 0x6000u;
    constexpr std::uint32_t temporary = 0x7000u;
    runtime.words[object + 20u] = 0xb000u;
    runtime.words[0xa000u + 316u] = 0xabcdu;
    Check(Actor::ExecuteScriptAndLoad(object, 9u, 0x8000u, 0x8100u,
                                      temporary, runtime) &&
              runtime.calls == (std::vector<Runtime::Invocation>{
                  {Call::CleanupObjectData,
                   {object + Actor::kObjectStateOffset}},
                  {Call::GetGameInstance, {}},
                  {Call::CleanupClump, {object, 0x91acu}},
                  {Call::InitializeName, {temporary, 0x8000u}},
                  {Call::FindScriptMarker, {temporary}},
                  {Call::GetArenaManager, {}},
                  {Call::GetArenaManager, {}},
                  {Call::ProcessArenaScript, {0xabcdu}},
                  {Call::GetGameInstance, {}},
                  {Call::LoadClump,
                   {object, 9u, 0x8000u, 0x8100u, 0x91acu, 0u, 0u, 1u}},
                  {Call::HideUserDataAtomics, {0xb000u}},
                  {Call::LoadAnimationData,
                   {object + Actor::kObjectStateOffset,
                    0xb000u, 0x8000u, 0x8100u}},
                  {Call::CleanupScripts, {}},
                  {Call::CleanupName, {temporary}}}),
          "missing marker runs arena script and loads animated clump");

    runtime.calls.clear();
    runtime.marker = 0;
    runtime.load_succeeds = false;
    Check(!Actor::ExecuteScriptAndLoad(object, 4u, 0x8200u, 0x8300u,
                                       temporary, runtime) &&
              runtime.calls == (std::vector<Runtime::Invocation>{
                  {Call::CleanupObjectData,
                   {object + Actor::kObjectStateOffset}},
                  {Call::GetGameInstance, {}},
                  {Call::CleanupClump, {object, 0x91acu}},
                  {Call::InitializeName, {temporary, 0x8200u}},
                  {Call::FindScriptMarker, {temporary}},
                  {Call::ProcessEmbeddedScript, {}},
                  {Call::GetGameInstance, {}},
                  {Call::LoadClump,
                   {object, 4u, 0x8200u, 0x8300u, 0x91acu, 0u, 0u, 1u}},
                  {Call::CleanupScripts, {}},
                  {Call::CleanupName, {temporary}}}),
          "found marker uses embedded script and failed load still cleans up");

    runtime.calls.clear();
    runtime.marker = -1;
    runtime.words[0xa000u + 316u] = 0u;
    Check(!Actor::ExecuteScriptAndLoad(object, 4u, 0x8200u, 0x8300u,
                                       temporary, runtime) &&
              runtime.calls[5] == Runtime::Invocation{
                  Call::GetArenaManager, {}} &&
              runtime.calls[6] == Runtime::Invocation{
                  Call::GetGameInstance, {}} &&
              runtime.calls.back() == Runtime::Invocation{
                  Call::CleanupName, {temporary}},
          "missing arena script pointer skips script processing");
}

}  // namespace

int main() {
    try {
        ConstructAndDestroy();
        CleanupRenderAndUpdate();
        ScriptLoadBranches();
        pal4::ArenaActorRuntimeStub stub;
        Check(Actor::Construct(0x6000u, stub) == 0x6000u,
              "native actor boundary stays linkable");
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
