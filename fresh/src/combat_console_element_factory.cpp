#include "pal4/combat_console_element_factory.h"

namespace pal4 {
namespace {

std::uint32_t CreateWithVtable(
    const std::uint32_t size, const std::uint32_t vtable,
    ICombatConsoleElementFactoryRuntime& runtime) {
    const auto object = runtime.Allocate(size);
    if (object != 0u) {
        runtime.ConstructBase(object);
        runtime.WriteWord(object, vtable);
    }
    return object;
}

}  // namespace

std::uint32_t CombatConsoleElementFactory::Create(
    ICombatConsoleElementFactoryRuntime& runtime) {
    return CreateWithVtable(kSize, kVtable, runtime);
}

std::uint32_t CombatResultIconFactory::Create(
    ICombatConsoleElementFactoryRuntime& runtime) {
    return CreateWithVtable(kSize, kVtable, runtime);
}

std::uint32_t CombatResultSoundFactory::Create(
    ICombatConsoleElementFactoryRuntime& runtime) {
    return CreateWithVtable(kSize, kVtable, runtime);
}

std::uint32_t CombatResultMoveFactory::Create(
    ICombatConsoleElementFactoryRuntime& runtime) {
    return CreateWithVtable(kSize, kVtable, runtime);
}

std::uint32_t CombatResultAudioFactory::Create(
    ICombatConsoleElementFactoryRuntime& runtime) {
    return CreateWithVtable(kSize, kVtable, runtime);
}

std::uint32_t CombatResultStateFactory::Create(
    ICombatConsoleElementFactoryRuntime& runtime) {
    return CreateWithVtable(kSize, kVtable, runtime);
}

std::uint32_t CombatResultPanelFactory::Create(
    ICombatConsoleElementFactoryRuntime& runtime) {
    return CreateWithVtable(kSize, kVtable, runtime);
}

std::uint32_t CombatResultEndFactory::Create(
    ICombatConsoleElementFactoryRuntime& runtime) {
    return CreateWithVtable(kSize, kVtable, runtime);
}

std::uint32_t CombatResultElementDestructor::DestroyBase(
    const std::uint32_t object,
    ICombatResultElementDestructorRuntime& runtime) {
    return runtime.DestroyBase(object);
}

std::uint32_t CombatResultElementDestructor::Delete(
    const std::uint32_t object, const std::uint8_t flags,
    ICombatResultElementDestructorRuntime& runtime) {
    (void)DestroyBase(object, runtime);
    if ((flags & 1u) != 0u)
        runtime.Free(object);
    return object;
}

}  // namespace pal4
