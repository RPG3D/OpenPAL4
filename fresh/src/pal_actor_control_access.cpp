#include "pal4/pal_actor_control_access.h"

namespace pal4 {

std::uint32_t PalActorControlAccessRuntimeStub::SingletonManager() {
    return 0u;
}
std::uint32_t PalActorControlAccessRuntimeStub::Register(
    std::uint32_t, std::string_view, std::uint32_t,
    std::uint32_t, std::uint32_t) { return 0u; }
void* PalActorControlAccessRuntimeStub::Allocate(std::uint32_t) {
    return nullptr;
}
void* PalActorControlAccessRuntimeStub::Construct(void* storage) {
    return storage;
}
void PalActorControlAccessRuntimeStub::Destruct(void*) {}
std::uint32_t PalActorControlAccessRuntimeStub::Free(void*) {
    return 0u;
}

std::uint32_t PalActorControlAccess::Get(
    PalActorControlAccessState& state,
    IPalActorControlAccessRuntime& runtime) {
    if ((state.guard & 1u) != 0u) return state.instance;
    state.guard |= 1u;
    const auto manager = runtime.SingletonManager();
    state.instance = runtime.Register(manager, "palActorController",
                                      kSingletonId, kFactoryAddress,
                                      kDestroyAddress);
    return state.instance;
}

void* PalActorControlAccess::Create(
    IPalActorControlAccessRuntime& runtime) {
    void* const memory = runtime.Allocate(kObjectBytes);
    if (memory == nullptr) return nullptr;
    return runtime.Construct(memory);
}

std::uint32_t PalActorControlAccess::Destroy(
    void* object, std::uint32_t null_result,
    IPalActorControlAccessRuntime& runtime) {
    if (object == nullptr) return null_result;
    runtime.Destruct(object);
    return runtime.Free(object);
}

}  // namespace pal4
