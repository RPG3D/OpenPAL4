#include "pal4/pal_actor_control_dispatch.h"

namespace pal4 {

bool IActorControlDispatchRuntime::HandlePlayerInteraction(
    PalActorControlState& state, std::uint32_t camera_internal) {
    return PalActorControlInteraction::Handle(state, camera_internal, *this);
}
bool IActorControlDispatchRuntime::TryMotion(
    std::uint32_t*, std::uint32_t, float) { return false; }
bool IActorControlDispatchRuntime::DatabaseHasPriority(
    std::uint32_t*) { return false; }
bool IActorControlDispatchRuntime::TryDatabase(
    std::uint32_t*, std::uint32_t, float) { return false; }
void IActorControlDispatchRuntime::BindDispatchSubobject(
    std::uint32_t*, std::uint32_t) {}
std::uint32_t IActorControlDispatchRuntime::SubobjectAddress(
    PalActorControlState&, std::uint32_t word_offset) {
    return word_offset * 4u;
}

bool PalActorControlDispatch::Select(
    PalActorControlState& state, float delta,
    std::uint32_t actor, std::uint32_t camera_internal,
    IActorControlDispatchRuntime& runtime) {
    if (actor == 0u)
        return false;
    auto& words = state.words;
    if (words[183] != 0u ||
        runtime.HandlePlayerInteraction(state, camera_internal))
        return true;

    if (runtime.TryMotion(words.data() + 165u, actor, delta)) {
        runtime.BindDispatchSubobject(words.data() + 35u, words[1]);
        runtime.BindDispatchSubobject(words.data() + 23u, words[1]);
        words[183] = runtime.SubobjectAddress(state, 165u);
        return true;
    }
    if (!runtime.DatabaseHasPriority(words.data() + 35u) &&
        runtime.TryDatabase(words.data() + 35u, actor, delta)) {
        runtime.BindDispatchSubobject(words.data() + 165u, words[1]);
        runtime.BindDispatchSubobject(words.data() + 23u, words[1]);
        words[183] = runtime.SubobjectAddress(state, 35u);
        return true;
    }
    words[183] = 0u;
    return false;
}

}  // namespace pal4
