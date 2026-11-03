#include "pal4/game_arena_manager_lifecycle.h"

#include <initializer_list>

namespace pal4 {
namespace {

void Invoke(IGameArenaLifecycleRuntime& runtime,
            const GameArenaLifecycleCall operation,
            const std::uint32_t address,
            const std::uint32_t auxiliary = 0u) {
    static_cast<void>(runtime.Call(operation, address, auxiliary));
}

void InitializeBlock(const std::uint32_t address,
                     IGameArenaLifecycleRuntime& runtime) {
    runtime.WriteWord(address + 4u, 0u);
    runtime.WriteByte(address, 0u);
    runtime.WriteWord(address + 8u, 0u);
    runtime.WriteWord(address + 12u, 0u);
}

void ReleaseArray(const std::uint32_t address,
                  IGameArenaLifecycleRuntime& runtime) {
    const auto storage = runtime.ReadWord(address + 4u);
    Invoke(runtime, GameArenaLifecycleCall::FreeMemory, storage);
    runtime.WriteWord(address + 4u, 0u);
    runtime.WriteWord(address + 8u, 0u);
    runtime.WriteWord(address + 12u, 0u);
}

void ReleaseRange(const std::uint32_t address,
                  IGameArenaLifecycleRuntime& runtime) {
    const auto begin = runtime.ReadWord(address + 4u);
    const auto end = runtime.ReadWord(address + 8u);
    Invoke(runtime, GameArenaLifecycleCall::ReleaseRange, begin, end);
    Invoke(runtime, GameArenaLifecycleCall::FreeMemory, begin);
    runtime.WriteWord(address + 4u, 0u);
    runtime.WriteWord(address + 8u, 0u);
    runtime.WriteWord(address + 12u, 0u);
}

}  // namespace

std::uint32_t GameArenaManagerSingleton::GetInstance(
    IGameArenaSingletonRuntime& runtime) {
    if ((state_.guard & 1u) != 0u) return state_.instance;
    state_.guard |= 1u;
    state_.instance = runtime.Register(
        runtime.GetSingletonManager(), "palGameArena", 1043u,
        kFactoryAddress, kDestructorAddress);
    return state_.instance;
}

std::uint32_t GameArenaTextPair::Construct(
    const std::uint32_t address, IGameArenaLifecycleRuntime& runtime) {
    Invoke(runtime, GameArenaLifecycleCall::InitializeText, address);
    Invoke(runtime, GameArenaLifecycleCall::InitializeText, address + 12u);
    return address;
}

void GameArenaTextPair::Destroy(
    const std::uint32_t address, IGameArenaLifecycleRuntime& runtime) {
    Invoke(runtime, GameArenaLifecycleCall::CleanupText, address + 12u);
    Invoke(runtime, GameArenaLifecycleCall::CleanupText, address);
}

std::uint32_t GameArenaManagerLifecycle::Create(
    IGameArenaLifecycleRuntime& runtime) {
    const auto object = runtime.Allocate(kObjectBytes);
    return object == 0u ? 0u : Construct(object, runtime);
}

std::uint32_t GameArenaManagerLifecycle::Construct(
    const std::uint32_t object, IGameArenaLifecycleRuntime& runtime) {
    Invoke(runtime, GameArenaLifecycleCall::ConstructWorld, object);
    for (const auto offset : {200u, 212u, 224u, 240u, 252u})
        Invoke(runtime, GameArenaLifecycleCall::InitializeText, object + offset);
    for (const auto offset : {268u, 284u, 300u})
        InitializeBlock(object + offset, runtime);
    Invoke(runtime, GameArenaLifecycleCall::InitializeConfig, object + 320u);
    Invoke(runtime, GameArenaLifecycleCall::ConstructNameNode, object + 408u);
    Invoke(runtime, GameArenaLifecycleCall::ConstructNameNode, object + 416u);
    Invoke(runtime, GameArenaLifecycleCall::InitializeCache, object + 448u);
    for (std::uint32_t i = 0u; i < 3u; ++i)
        static_cast<void>(GameArenaTextPair::Construct(
            object + 508u + i * GameArenaTextPair::kBytes, runtime));
    for (const auto offset : {616u, 632u, 648u, 664u, 680u})
        InitializeBlock(object + offset, runtime);

    runtime.WriteWord(object, kVtableAddress);
    runtime.WriteWord(object + 236u, 0u);
    runtime.WriteWord(object + 316u, 0u);
    runtime.WriteWord(object + 196u, 1u);
    runtime.WriteWord(object + 444u, 0u);
    runtime.WriteWord(object + 264u, 0u);
    runtime.WriteWord(object + 424u, 0u);
    runtime.WriteWord(object + 440u, 0x3e99999au);
    for (const auto offset : {428u, 432u, 436u})
        runtime.WriteWord(object + offset, 0x3f800000u);
    runtime.WriteWord(object + 404u, 0u);
    runtime.WriteWord(object + 504u, 0u);
    Invoke(runtime, GameArenaLifecycleCall::InitializeObjectList, object);
    return object;
}

std::uint32_t GameArenaManagerLifecycle::Destroy(
    const std::uint32_t object, IGameArenaLifecycleRuntime& runtime) {
    runtime.WriteWord(object, kVtableAddress);
    const auto defaults = runtime.ReadWord(object + 504u);
    if (defaults != 0u) {
        Invoke(runtime, GameArenaLifecycleCall::InitializeDefaultValues, defaults);
        Invoke(runtime, GameArenaLifecycleCall::FreeMemory, defaults);
        runtime.WriteWord(object + 504u, 0u);
    }
    const auto script = runtime.ReadWord(object + 316u);
    if (script != 0u) {
        Invoke(runtime, GameArenaLifecycleCall::DestroyScript, script);
        runtime.WriteWord(object + 316u, 0u);
    }
    for (const auto offset : {680u, 664u, 648u, 632u, 616u})
        ReleaseArray(object + offset, runtime);
    for (std::uint32_t i = 3u; i != 0u; --i)
        GameArenaTextPair::Destroy(
            object + 508u + (i - 1u) * GameArenaTextPair::kBytes,
            runtime);
    Invoke(runtime, GameArenaLifecycleCall::DestroyCache, object + 448u);
    Invoke(runtime, GameArenaLifecycleCall::ResetNameNode, object + 416u);
    Invoke(runtime, GameArenaLifecycleCall::ResetNameNode, object + 408u);
    Invoke(runtime, GameArenaLifecycleCall::DestroyConfig, object + 320u);
    ReleaseRange(object + 300u, runtime);
    ReleaseRange(object + 284u, runtime);
    Invoke(runtime, GameArenaLifecycleCall::CleanupMemoryBlock, object + 268u);
    for (const auto offset : {252u, 240u, 224u, 212u, 200u})
        Invoke(runtime, GameArenaLifecycleCall::CleanupText, object + offset);
    return runtime.Call(GameArenaLifecycleCall::DestroyWorld, object);
}

std::uint32_t GameArenaManagerLifecycle::ForwardRelease(
    const std::uint32_t object, IGameArenaLifecycleRuntime& runtime) {
    return runtime.Call(GameArenaLifecycleCall::ReleaseArena, object);
}

}  // namespace pal4
