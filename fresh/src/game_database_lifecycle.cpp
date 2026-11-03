#include "pal4/game_database_lifecycle.h"

#include <array>

namespace pal4 {
namespace {

struct ComponentInitializer {
    std::uint32_t offset;
    std::uint32_t address;
};

constexpr std::array<ComponentInitializer, 12> kInitializers{{
    {0u, 0x7f0190u}, {12u, 0x4eee90u}, {32u, 0x5fc0a0u},
    {52u, 0x4eee90u}, {88u, 0x4eee90u}, {108u, 0x4eee90u},
    {128u, 0x7f0190u}, {140u, 0x40c350u}, {148u, 0x5fc0a0u},
    {164u, 0x40c350u}, {172u, 0x7f0190u}, {184u, 0x4cfa30u},
}};

constexpr std::array<std::uint32_t, 10> kFirstManagerFields{
    0u, 12u, 32u, 52u, 88u, 128u, 140u, 148u, 164u, 172u};

constexpr std::array<std::uint32_t, 12> kDestroyFields{
    184u, 172u, 164u, 148u, 140u, 128u,
    108u, 88u, 52u, 32u, 12u, 0u};

std::uint32_t CreateManager(IGameDatabaseLifecycleRuntime& runtime) {
    const auto memory = runtime.Allocate(GameDatabaseLifecycle::kManagerBytes);
    return memory == 0u ? 0u : runtime.InitializeManager(memory);
}

}  // namespace

std::uint32_t GameDatabaseLifecycle::SetAssertionLoggerCallback(
    std::uint32_t& slot, std::uint32_t callback) {
    slot = callback;
    return callback;
}

std::uint32_t GameDatabaseSingleton::GetInstance(
    IGameDatabaseSingletonRuntime& runtime) {
    if ((state_.initialization_bits & 1u) != 0u) return state_.instance;
    state_.initialization_bits |= 1u;
    const auto manager = runtime.GetSingletonManager();
    state_.instance = runtime.Register(manager, "palGameDB", 1025u,
                                       kFactoryAddress, kDestructorAddress);
    return state_.instance;
}

std::uint32_t GameDatabaseSingleton::GetInstanceAlias(
    IGameDatabaseSingletonRuntime& runtime) {
    return GetInstance(runtime);
}

std::uint32_t GameDatabaseLifecycle::CreateInstance(
    IGameDatabaseLifecycleRuntime& runtime) {
    const auto memory = runtime.Allocate(kObjectBytes);
    return memory == 0u ? 0u : Construct(memory, runtime);
}

std::uint32_t GameDatabaseLifecycle::DestroyInstance(
    const std::uint32_t object, IGameDatabaseLifecycleRuntime& runtime) {
    if (object == 0u) return 0u;
    Destruct(object, runtime);
    return runtime.Free(object);
}

std::uint32_t GameDatabaseLifecycle::Construct(
    const std::uint32_t object, IGameDatabaseLifecycleRuntime& runtime) {
    for (const auto& initializer : kInitializers)
        runtime.InitializeComponent(object + initializer.offset,
                                    initializer.address);

    const auto first_manager = CreateManager(runtime);
    runtime.WriteWord(object + 324u, first_manager);
    const auto second_manager = CreateManager(runtime);
    runtime.WriteWord(object + 328u, second_manager);
    runtime.SetEffectGlobal(0x50cd40u);
    runtime.SetGameDatabaseGlobal(0x50cd70u);

    for (const auto offset : kFirstManagerFields)
        runtime.BindManager(object + offset, first_manager);
    runtime.SetPropertyManager(object + 184u, first_manager);
    runtime.BindManager(object + 108u, second_manager);
    return object;
}

void GameDatabaseLifecycle::Destruct(
    const std::uint32_t object, IGameDatabaseLifecycleRuntime& runtime) {
    for (const auto offset : {324u, 328u}) {
        const auto manager = runtime.ReadWord(object + offset);
        if (manager != 0u) {
            runtime.DestroyManager(manager);
            static_cast<void>(runtime.Free(manager));
        }
    }
    for (const auto offset : kDestroyFields)
        runtime.DestroyComponent(object + offset);
}

}  // namespace pal4
