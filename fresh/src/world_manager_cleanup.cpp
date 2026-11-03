#include "pal4/world_manager_cleanup.h"
#include "pal4/pal_game_iv.h"

#include <initializer_list>

namespace pal4 {

void WorldManagerCleanup::ClearText(
    const std::uint32_t descriptor, IWorldManagerCleanupRuntime& runtime) {
    const auto text = runtime.ReadWord(descriptor + 4u);
    if (text != 0u) {
        const auto header = text - 1u;
        const auto refs = runtime.ReadByte(header);
        if (refs == 0u || refs == 0xffu)
            runtime.FreeTextStorage(header);
        else
            runtime.WriteByte(header, static_cast<std::uint8_t>(refs - 1u));
    }
    for (const auto offset : {4u, 8u, 12u})
        runtime.WriteWord(descriptor + offset, 0u);
}

std::uint32_t WorldManagerCleanup::Run(
    const std::uint32_t manager, IWorldManagerCleanupRuntime& runtime) {
    const auto world = manager + 116u;
    (void)WorldManagerSceneInitializer::ClearPipeline(world, runtime);
    runtime.CompactAtomicList(manager + 312u);
    runtime.CompactAtomicList(manager + 328u);
    runtime.CleanupWorld(world, runtime.GameInstance() +
                                    PalGameIV::kTextureDictionaryOffset);
    for (const auto offset : {44u, 64u, 68u})
        runtime.WriteWord(manager + offset, 0u);
    ClearText(manager + 348u, runtime);
    ClearText(manager + 364u, runtime);

    constexpr std::uint32_t kMatFxVm = 0x8f94c4u;
    const auto vm = runtime.ReadWord(kMatFxVm);
    if (vm != 0u) {
        runtime.ReleaseMatFxVm(vm);
        runtime.WriteWord(kMatFxVm, 0u);
    }
    return 1u;
}

}  // namespace pal4
