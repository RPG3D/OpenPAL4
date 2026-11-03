#include "pal4/id_generator_registry.h"

#include <new>

namespace pal4 {

std::uint32_t IdGeneratorRegistry::CreateFactoryObject(
    IIdGeneratorRegistryRuntime& runtime) {
    const auto object = runtime.AllocateMemory(8u);
    if (object != 0u) {
        runtime.WriteWord(object + 4u, 1u);
        runtime.WriteWord(object, 0x84346cu);
    }
    return object;
}

std::int32_t IdGeneratorRegistry::Size(
    const std::uint32_t registry, IIdGeneratorRegistryRuntime& runtime) {
    const auto begin = runtime.ReadWord(registry + 4u);
    if (begin == 0u) return 0;
    return static_cast<std::int32_t>(
        runtime.ReadWord(registry + 8u) - begin) >> 3;
}

std::uint32_t IdGeneratorRegistry::RegisterViaThunk(
    const std::uint32_t object, IIdGeneratorRegistryRuntime& runtime) {
    return Register(object, runtime);
}

std::uint32_t IdGeneratorRegistry::Register(
    const std::uint32_t object, IIdGeneratorRegistryRuntime& runtime) {
    const auto registry = runtime.InitializeRegistry(object);
    const auto begin = runtime.ReadWord(registry + 4u);
    const auto end = runtime.ReadWord(registry + 8u);
    const auto capacity = runtime.ReadWord(registry + 12u);
    if (static_cast<std::int32_t>(capacity - end) >= 8) {
        runtime.WriteWord(end, kNameAddress);
        runtime.WriteWord(end + 4u, kFactoryAddress);
        runtime.WriteWord(registry + 8u, end + 8u);
        return end + 8u;
    }

    const auto count = begin == 0u ? 0u : (end - begin) / 8u;
    const auto growth = count > 1u ? count : 1u;
    const auto new_capacity = count + growth;
    const auto replacement = runtime.AllocateMemory(new_capacity * 8u);
    if (replacement == 0u) throw std::bad_alloc();
    for (std::uint32_t index = 0u; index < count; ++index) {
        const auto source = begin + index * 8u;
        const auto destination = replacement + index * 8u;
        runtime.WriteWord(destination, runtime.ReadWord(source));
        runtime.WriteWord(destination + 4u,
                          runtime.ReadWord(source + 4u));
    }
    const auto appended = replacement + count * 8u;
    runtime.WriteWord(appended, kNameAddress);
    runtime.WriteWord(appended + 4u, kFactoryAddress);
    runtime.FreeMemory(begin);
    runtime.WriteWord(registry + 12u,
                      replacement + new_capacity * 8u);
    const auto original_count = Size(registry, runtime);
    runtime.WriteWord(registry + 4u, replacement);
    runtime.WriteWord(registry + 8u, appended + 8u);
    return static_cast<std::uint32_t>(original_count);
}

}  // namespace pal4
