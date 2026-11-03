#include "pal4/inventory_subsystem.h"
#include <bit>

namespace pal4 {

std::uint32_t InventorySubsystem::InitializeSingleton(
    std::uint8_t& guard, std::uint32_t& cached_instance,
    IInventorySubsystemRuntime& runtime) {
    if ((guard & 1u) != 0u) return cached_instance;
    guard = static_cast<std::uint8_t>(guard | 1u);
    cached_instance = runtime.RegisterSingleton(
        kNameAddress, kSingletonId, kFactoryAddress, kCleanupAddress);
    return cached_instance;
}

std::uint32_t InventorySubsystem::Create(
    IInventorySubsystemRuntime& runtime) {
    const auto object = runtime.AllocateMemory(0x138u);
    if (object == 0u) return 0u;
    try {
        return Construct(object, runtime);
    } catch (...) {
        (void)runtime.FreeMemory(object);
        throw;
    }
}

std::uint32_t InventorySubsystem::Destroy(
    const std::uint32_t object, IInventorySubsystemRuntime& runtime) {
    if (object == 0u) return 0u;
    Cleanup(object, runtime);
    return runtime.FreeMemory(object);
}

std::uint32_t InventorySubsystem::Construct(
    const std::uint32_t object, IInventorySubsystemRuntime& runtime) {
    runtime.InitializeBase(object);
    runtime.InitializeSlots(object + 124u);
    runtime.InitializeArray(object + 260u);
    return object;
}

void InventorySubsystem::Cleanup(
    const std::uint32_t object, IInventorySubsystemRuntime& runtime) {
    runtime.CleanupArray(object == 0u ? 0u : object + 260u);
    runtime.CleanupSlots(object == 0u ? 0u : object + 124u);
    runtime.CleanupBase(object);
}

std::int32_t InventorySubsystem::UpdateCounter(
    const std::uint32_t object, const std::int32_t delta,
    IInventorySubsystemRuntime& runtime) {
    const auto address = object + 0x134u;
    const auto clamped=UpdatedCounter(runtime.ReadWord(address),delta);
    runtime.WriteWord(address, static_cast<std::uint32_t>(clamped));
    return clamped;
}

std::int32_t InventorySubsystem::UpdatedCounter(std::uint32_t previous,std::int32_t delta){
    const auto value=std::bit_cast<std::int32_t>(previous+static_cast<std::uint32_t>(delta));
    return value<0?0:value>9999999?9999999:value;
}
std::uint8_t InventorySubsystem::InitializeGameData(
    const std::uint32_t object, IInventorySubsystemRuntime& runtime) {
    for (std::uint32_t id = 8000u; id < 8500u; ++id) {
        const auto database = runtime.GameDbInstance();
        if (runtime.HasFirst(database + 128u, id))
            runtime.ProcessFirst(object + 260u, id);
    }
    for (std::uint32_t id = 3000u; id < 3500u; ++id) {
        const auto database = runtime.GameDbInstance();
        if (runtime.HasSecond(database, id)) {
            runtime.CalculateSecond(runtime.GameDbInstance(), id);
            runtime.ProcessItem(object, id, 99u);
        }
    }
    std::uint8_t result = 0u;
    for (std::uint32_t id = 3500u; id < 4000u; ++id) {
        const auto database = runtime.GameDbInstance();
        result = runtime.HasThird(database + 88u, id);
        if (result != 0u)
            result = static_cast<std::uint8_t>(
                runtime.ProcessThird(object + 124u, id));
    }
    return result;
}

std::uint32_t InventorySubsystem::Reset(
    const std::uint32_t object, IInventoryResetRuntime& runtime) {
    runtime.WriteWord(object + 0x134u, 0u);
    runtime.ResetSlots(object + 0x7cu);
    runtime.WriteWord(object + 0x80u, 1u);
    runtime.ResetArray(object + 0x104u);
    const auto result = runtime.ResetBase(object);
    runtime.WriteWord(object + 4u, 1u);
    return result;
}

}  // namespace pal4
