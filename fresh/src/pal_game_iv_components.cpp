#include "pal4/pal_game_iv_components.h"

#include <initializer_list>
#include <new>

namespace pal4 {
namespace {

std::uint32_t Allocate(IPalGameIVComponentsRuntime& runtime,
                       std::uint32_t bytes) {
    const auto object = runtime.Allocate(bytes);
    if (object == 0u) throw std::bad_alloc();
    return object;
}

void Register(std::uint32_t game, std::uint32_t component,
              IPalGameIVComponentsRuntime& runtime) {
    (void)PalGameIVStateRegistry::AddEntry(game, component, runtime);
}

std::uint32_t Construct(IPalGameIVComponentsRuntime& runtime,
                        PalGameIVComponentKind kind, std::uint32_t bytes) {
    return runtime.ConstructComponent(kind, Allocate(runtime, bytes));
}

}  // namespace

std::uint32_t PalGameIVComponents::ConstructCombatState(
    std::uint32_t object, IPalGameIVComponentsRuntime& runtime) {
    runtime.WriteWord(object, 0x8485c8u);
    (void)runtime.ConstructComponent(PalGameIVComponentKind::CombatXmlDocument,
                                    object + 12u);
    runtime.WriteWord(object, 0x849480u);
    return object;
}

bool PalGameIVComponents::Initialize(
    std::uint32_t game, IPalGameIVComponentsRuntime& runtime) {
    Register(game, Construct(runtime, PalGameIVComponentKind::World, 4u),
             runtime);

    auto object = Allocate(runtime, 16u);
    runtime.WriteWord(object, 0x8485c8u);
    (void)runtime.ConstructComponent(PalGameIVComponentKind::SceneNamedNode,
                                     object + 4u);
    runtime.WriteWord(object, 0x849498u);
    Register(game, object, runtime);

    object = Allocate(runtime, 72u);
    (void)ConstructCombatState(object, runtime);
    Register(game, object, runtime);

    for (auto vtable : {0x849468u, 0x849450u, 0x849438u}) {
        object = Allocate(runtime, 4u);
        runtime.WriteWord(object, vtable);
        Register(game, object, runtime);
    }

    Register(game, Construct(runtime, PalGameIVComponentKind::WorldLoading,
                             56u), runtime);
    Register(game, Construct(runtime, PalGameIVComponentKind::GameOver, 20u),
             runtime);

    object = Allocate(runtime, 4u);
    runtime.WriteWord(object, 0x849420u);
    Register(game, object, runtime);

    Register(game, Construct(runtime, PalGameIVComponentKind::Save, 4u),
             runtime);
    Register(game, Construct(runtime, PalGameIVComponentKind::ModalNamedNode,
                             8u), runtime);
    Register(game, Construct(runtime, PalGameIVComponentKind::WorldMap, 4u),
             runtime);
    const auto bink = Construct(runtime, PalGameIVComponentKind::Bink, 8u);
    return PalGameIVStateRegistry::AddEntry(game, bink, runtime);
}

}  // namespace pal4
