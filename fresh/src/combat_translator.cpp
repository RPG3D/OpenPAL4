#include "pal4/combat_translator.h"

namespace pal4 {

std::uint32_t CombatTranslator::Construct(
    std::uint32_t object, ICombatTranslatorRuntime& runtime) {
    const auto storage = runtime.Allocate(32u);
    const auto backend = storage != 0u ?
        runtime.ConstructBackend(storage) : 0u;
    runtime.WriteWord(object, backend);
    runtime.WriteWord(object + 4u, 0u);
    runtime.WriteWord(object + 8u, 0u);
    runtime.WriteWord(object + 12u, 0u);
    return object;
}

std::uint32_t CombatTranslator::DestructThunk(
    std::uint32_t object, ICombatTranslatorRuntime& runtime) {
    return Destruct(object, runtime);
}

std::uint8_t CombatTranslator::Load(
    std::uint32_t object, std::uint32_t input,
    ICombatTranslatorRuntime& runtime) {
    const auto backend = runtime.ReadWord(object);
    const auto loaded = runtime.LoadBackend(
        backend, input, kAdapterVtable);
    if (loaded == 0u) return loaded;
    const auto translation = runtime.FindNamedBackend(
        runtime.ReadWord(object), "CombatTranslation");
    runtime.WriteWord(object + 4u, translation);
    return translation != 0u ? 1u : 0u;
}

std::uint32_t CombatTranslator::Destruct(
    std::uint32_t object, ICombatTranslatorRuntime& runtime) {
    const auto backend = runtime.ReadWord(object);
    if (backend == 0u) return 0u;
    runtime.WriteWord(object + 4u, 0u);
    auto result = runtime.ClearBackend(backend);
    const auto current = runtime.ReadWord(object);
    if (current != 0u) {
        runtime.DestroyBackend(current);
        result = runtime.Free(current);
    }
    runtime.WriteWord(object, 0u);
    return result;
}

}  // namespace pal4
