#include "pal4/arena_effect_manager.h"

#include <bit>
#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

using Manager = pal4::ArenaEffectManager;

void Check(const bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IArenaEffectRuntime {
    struct Invocation {
        pal4::ArenaEffectCall operation;
        std::vector<std::uint32_t> arguments;
        bool operator==(const Invocation&) const = default;
    };
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::uint32_t> allocation_sizes;
    std::vector<std::uint32_t> allocation_results;
    std::vector<std::uint32_t> frees;
    std::vector<Invocation> calls;
    std::map<pal4::ArenaEffectCall, std::uint32_t> call_results;
    std::string registered_name;
    std::vector<std::uint32_t> registration;

    std::uint32_t ReadWord(const std::uint32_t address) override {
        return words[address];
    }
    void WriteWord(const std::uint32_t address,
                   const std::uint32_t value) override {
        words[address] = value;
    }
    void WriteByte(const std::uint32_t address,
                   const std::uint8_t value) override {
        bytes[address] = value;
    }
    std::uint32_t Allocate(const std::uint32_t size) override {
        allocation_sizes.push_back(size);
        if (allocation_results.empty()) return 0u;
        const auto value = allocation_results.front();
        allocation_results.erase(allocation_results.begin());
        return value;
    }
    void Free(const std::uint32_t pointer) override {
        frees.push_back(pointer);
    }
    std::uint32_t RegisterSingleton(
        const std::string_view name, const std::uint32_t id,
        const std::uint32_t factory,
        const std::uint32_t type_info) override {
        registered_name = name;
        registration = {id, factory, type_info};
        return 0x7000u;
    }
    std::uint32_t Call(
        const pal4::ArenaEffectCall operation,
        const std::initializer_list<std::uint32_t> arguments) override {
        calls.push_back({operation, arguments});
        if (const auto result = call_results.find(operation);
            result != call_results.end())
            return result->second;
        return operation == pal4::ArenaEffectCall::DestroyBase ? 0xbeefu : 0u;
    }
};

void Registration() {
    Runtime runtime;
    std::uint8_t guard = 0x80u;
    std::uint32_t registered = 0u;
    Check(Manager::Register(guard, registered, runtime) == 0x7000u &&
              guard == 0x81u && registered == 0x7000u &&
              runtime.registered_name == "palGameArenaEffMgr" &&
              runtime.registration ==
                  (std::vector<std::uint32_t>{1049u, 0x522730u,
                                               0x7de5d0u}),
          "guarded registration preserves name, ID and callback addresses");
    runtime.registered_name.clear();
    Check(Manager::Register(guard, registered, runtime) == 0x7000u &&
              runtime.registered_name.empty(),
          "already registered singleton returns its cached address");
}

void SharedSentinelLifecycle() {
    Runtime runtime;
    pal4::ArenaEffectSharedState shared;
    runtime.allocation_results = {0x6000u, 0x8000u, 0x9000u,
                                  0x6100u, 0x8100u, 0x9100u};
    Check(Manager::Create(shared, runtime) == 0x6000u &&
              runtime.allocation_sizes ==
                  (std::vector<std::uint32_t>{0x130u, 24u, 24u}) &&
              shared.sentinel == 0x8000u && shared.instance_count == 1u &&
              runtime.words[0x6000u] == Manager::kVtable &&
              runtime.bytes[0x6000u + 260u] == 0u &&
              runtime.bytes[0x6000u + 280u] == 0u &&
              runtime.bytes[0x6000u + 281u] == 0u &&
              runtime.bytes[0x6000u + 288u] == 0u &&
              runtime.words[0x6000u + 284u] == 0x9000u &&
              runtime.words[0x6000u + 292u] == 0u &&
              runtime.words[0x6000u + 296u] == 0u &&
              runtime.words[0x8000u] == 0u &&
              runtime.words[0x8000u + 8u] == 0u &&
              runtime.words[0x9000u] == 0x9000u &&
              runtime.words[0x9000u + 4u] == 0x8000u &&
              runtime.words[0x9000u + 8u] == 0x9000u &&
              runtime.words[0x9000u + 20u] == 0u &&
              runtime.calls == (std::vector<Runtime::Invocation>{
                  {pal4::ArenaEffectCall::InitializeBase, {0x6000u}}}),
          "first instance initializes object, shared sentinel and tree head");
    for (const auto offset : {264u, 268u, 272u, 276u})
        Check(runtime.words[0x6000u + offset] == 0u,
              "effect manager clears each trailing base field");

    Check(Manager::Create(shared, runtime) == 0x6100u &&
              shared.sentinel == 0x8000u && shared.instance_count == 2u &&
              runtime.frees == (std::vector<std::uint32_t>{0x8100u}) &&
              runtime.words[0x9100u + 4u] == 0x8000u,
          "second instance reuses sentinel and frees its candidate node");
    runtime.words[0x6000u + 292u] = 2u;
    runtime.words[0x9000u + 4u] = 0xa000u;
    runtime.words[0xa000u] = 0x8000u;
    runtime.words[0xa000u + 8u] = 0x8000u;
    Check(Manager::Cleanup(0x6000u, shared, runtime) == 0xbeefu &&
              shared.instance_count == 1u && shared.sentinel == 0x8000u &&
              runtime.frees ==
                  (std::vector<std::uint32_t>{0x8100u, 0xa000u,
                                               0x9000u}) &&
              runtime.words[0x6000u + 284u] == 0u &&
              runtime.words[0x6000u + 292u] == 0u &&
              runtime.calls == (std::vector<Runtime::Invocation>{
                  {pal4::ArenaEffectCall::InitializeBase, {0x6000u}},
                  {pal4::ArenaEffectCall::InitializeBase, {0x6100u}},
                  {pal4::ArenaEffectCall::ResetExisting, {0x6000u + 260u}},
                  {pal4::ArenaEffectCall::ResetFirstTreeElement,
                   {0x6000u + 280u}},
                  {pal4::ArenaEffectCall::DestroyBase, {0x6000u}}}),
          "populated tree cleanup releases head but retains shared sentinel");

    runtime.calls.clear();
    Check(Manager::Cleanup(0x6100u, shared, runtime) == 0xbeefu &&
              shared.instance_count == 0u && shared.sentinel == 0u &&
              runtime.frees == (std::vector<std::uint32_t>{
                  0x8100u, 0xa000u, 0x9000u, 0x9100u, 0x8000u}) &&
              runtime.calls == (std::vector<Runtime::Invocation>{
                  {pal4::ArenaEffectCall::ResetExisting, {0x6100u + 260u}},
                  {pal4::ArenaEffectCall::DestroyBase, {0x6100u}}}),
          "last instance releases head and shared sentinel in order");
}

void AllocationFailure() {
    Runtime runtime;
    pal4::ArenaEffectSharedState shared;
    Check(Manager::Create(shared, runtime) == 0u &&
              runtime.allocation_sizes ==
                  (std::vector<std::uint32_t>{0x130u}) &&
              runtime.calls.empty(),
          "factory returns null when object allocation fails");

    runtime = Runtime{};
    runtime.allocation_results = {0x6000u, 0u};
    Check(Manager::Create(shared, runtime) == 0u &&
              runtime.frees == (std::vector<std::uint32_t>{0x6000u}) &&
              shared.sentinel == 0u && shared.instance_count == 0u,
          "failed sentinel allocation frees new object");

    runtime = Runtime{};
    runtime.allocation_results = {0x6000u, 0x8000u, 0u};
    Check(Manager::Create(shared, runtime) == 0u &&
              runtime.frees == (std::vector<std::uint32_t>{
                  0x8000u, 0x6000u}) &&
              shared.sentinel == 0u && shared.instance_count == 0u,
          "failed tree-head allocation rolls back newly shared sentinel");
}

void TreeAndScriptCallbacks() {
    Runtime runtime;
    pal4::ArenaEffectSharedState shared{0x8000u, 1u, 0x7777u};
    constexpr std::uint32_t object = 0x6000u;
    runtime.words[object + 284u] = 0x9000u;
    runtime.words[0x9000u] = 0xa000u;
    Check(Manager::Cleanup(object, shared, runtime) == 0xbeefu &&
              runtime.calls == (std::vector<Runtime::Invocation>{
                  {pal4::ArenaEffectCall::ResetExisting, {object + 260u}},
                  {pal4::ArenaEffectCall::RemoveUncountedTreeNodes,
                   {object + 280u, 0x9000u}},
                  {pal4::ArenaEffectCall::DestroyBase, {object}}}) &&
              runtime.frees ==
                  (std::vector<std::uint32_t>{0x9000u, 0x8000u}),
          "uncounted tree cleanup forwards remaining nodes before head free");

    runtime = Runtime{};
    shared = {0x8000u, 1u, 0u};
    runtime.words[0xa000u] = 0xb000u;
    runtime.words[0xa000u + 8u] = 0xc000u;
    runtime.words[0xb000u] = 0x8000u;
    runtime.words[0xb000u + 8u] = 0x8000u;
    runtime.words[0xc000u] = 0x8000u;
    runtime.words[0xc000u + 8u] = 0x8000u;
    Check(Manager::CleanupTree(0xa000u, shared, runtime) == 0x8000u &&
              runtime.frees == (std::vector<std::uint32_t>{
                  0xc000u, 0xa000u, 0xb000u}),
          "tree cleanup frees right subtree before node and left chain");

    runtime = Runtime{};
    shared = {0u, 0u, 0x1111u};
    runtime.words[object + 264u] = 2u;
    runtime.words[object + 296u] = 0x2222u;
    runtime.words[object + 300u] = 0x3333u;
    Check(Manager::HandleScriptCallback(object, 0x4444u,
                                         shared, runtime) == 0x2222u &&
              runtime.calls == (std::vector<Runtime::Invocation>{
                  {pal4::ArenaEffectCall::HandleScriptExecution,
                   {object + 260u, 0x4444u}}}) &&
              runtime.words[object + 296u] == 0x2222u &&
              shared.script_callback == 0x3333u,
          "active script swaps current and queued callbacks via global slot");
    runtime.calls.clear();
    runtime.words[object + 264u] = 0u;
    Check(Manager::HandleScriptCallback(object, 5u,
                                         shared, runtime) == 0x2222u &&
              runtime.words[object + 296u] == 0u &&
              shared.script_callback == 0x2222u,
          "inactive script parks current callback in the global slot");
    runtime.words[object + 264u] = 1u;
    Check(Manager::HandleScriptCallback(object, 6u,
                                         shared, runtime) == 0u &&
              runtime.words[object + 296u] == 0x2222u &&
              shared.script_callback == 0x3333u,
          "active script restores parked callback even with no current one");

    runtime = Runtime{};
    shared = {0u, 0u, 0x1111u};
    runtime.call_results[pal4::ArenaEffectCall::GetEffectSingleton] = 0x7000u;
    runtime.words[object + 296u] = 0x2222u;
    Check(Manager::CleanupScriptCallback(object, shared, runtime) ==
                  0x2222u &&
              runtime.calls == (std::vector<Runtime::Invocation>{
                  {pal4::ArenaEffectCall::GetEffectSingleton, {}},
                  {pal4::ArenaEffectCall::StopActive, {0x7000u + 260u}}}) &&
              runtime.words[object + 296u] == 0u &&
              shared.script_callback == 0x2222u,
          "callback cleanup stops singleton and saves non-null callback");
}

void WrappersAndShutdown() {
    Runtime runtime;
    pal4::ArenaEffectSharedState shared;
    constexpr std::uint32_t object = 0x6000u;
    runtime.call_results[pal4::ArenaEffectCall::InitializeGameManagers] =
        0x101u;
    Check(Manager::InitializeGameManagers(object, runtime) &&
              runtime.calls.back() == Runtime::Invocation{
                  pal4::ArenaEffectCall::InitializeGameManagers,
                  {object + 260u}},
          "game manager initialization forwards embedded offset");
    runtime.call_results[pal4::ArenaEffectCall::InitializeGameManagers] =
        0x100u;
    Check(!Manager::InitializeGameManagers(object, runtime),
          "initialization wrapper returns only the low byte");

    runtime.calls.clear();
    Manager::RenderPasses(object, 7u, runtime);
    runtime.call_results[pal4::ArenaEffectCall::UpdateRenderParameters] =
        0x1234u;
    Check(Manager::UpdateRenderParams(object, 8u, 9u, runtime) == 0x1234u &&
              runtime.calls == (std::vector<Runtime::Invocation>{
                  {pal4::ArenaEffectCall::RenderPasses,
                   {object + 260u, 7u}},
                  {pal4::ArenaEffectCall::UpdateRenderParameters,
                   {object + 260u, 8u, 9u}}}),
          "render wrappers forward arguments and return value");

    runtime.calls.clear();
    runtime.call_results[pal4::ArenaEffectCall::GetGameArenaManager] = 0xa100u;
    runtime.call_results[pal4::ArenaEffectCall::Enable] = 0x101u;
    runtime.call_results[pal4::ArenaEffectCall::Draw] = 0x100u;
    runtime.call_results[pal4::ArenaEffectCall::RenderTransform] = 0x5678u;
    const auto transform = std::bit_cast<std::uint64_t>(1.25);
    Check(Manager::AddActiveObject(0xb000u, runtime) &&
              Manager::Enable(object, 1u, 2u, runtime) == 1u &&
              Manager::Draw(object, 1u, 2u, 3u, 4u, 5u, 6u,
                            runtime) == 0u &&
              Manager::RenderTransform(1.25, runtime) == 0x5678u &&
              runtime.calls == (std::vector<Runtime::Invocation>{
                  {pal4::ArenaEffectCall::GetGameArenaManager, {}},
                  {pal4::ArenaEffectCall::AddClump,
                   {0xa100u, 0xb000u}},
                  {pal4::ArenaEffectCall::Enable, {object, 1u, 2u}},
                  {pal4::ArenaEffectCall::Draw,
                   {object, 1u, 2u, 3u, 4u, 5u, 6u}},
                  {pal4::ArenaEffectCall::RenderTransform,
                   {static_cast<std::uint32_t>(transform),
                    static_cast<std::uint32_t>(transform >> 32u)}}}),
          "clump callback and effect thunks preserve argument order");

    runtime = Runtime{};
    runtime.call_results[pal4::ArenaEffectCall::GetEffectSingleton] = 0x7000u;
    runtime.words[object + 296u] = 0x8888u;
    Manager::CleanupAndShutdown(object, shared, runtime);
    Check(shared.script_callback == 0x8888u &&
              runtime.calls == (std::vector<Runtime::Invocation>{
                  {pal4::ArenaEffectCall::GetEffectSingleton, {}},
                  {pal4::ArenaEffectCall::StopActive, {0x7000u + 260u}},
                  {pal4::ArenaEffectCall::ResetExisting, {object + 260u}},
                  {pal4::ArenaEffectCall::CleanupEffects, {object}},
                  {pal4::ArenaEffectCall::Disable, {object}}}),
          "full shutdown resets manager before effect cleanup and disable");
    runtime.calls.clear();
    Manager::CleanupAndShutdownEffects(object, shared, runtime);
    Check(runtime.calls == (std::vector<Runtime::Invocation>{
              {pal4::ArenaEffectCall::GetEffectSingleton, {}},
              {pal4::ArenaEffectCall::StopActive, {0x7000u + 260u}},
              {pal4::ArenaEffectCall::CleanupEffects, {object}},
              {pal4::ArenaEffectCall::Disable, {object}}}),
          "second shutdown route skips embedded manager reset");
}

}  // namespace

int main() {
    try {
        Registration();
        SharedSentinelLifecycle();
        AllocationFailure();
        TreeAndScriptCallbacks();
        WrappersAndShutdown();
        pal4::ArenaEffectRuntimeStub stub;
        pal4::ArenaEffectSharedState shared;
        Check(Manager::Create(shared, stub) == 0u,
              "native effect runtime boundary remains linkable");
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
