#include "pal4/actor_message_adapter.h"

namespace pal4 {

std::uint32_t ActorMessageAccess::Get(
    ActorMessageAccessState& state, IActorMessageAccessRuntime& runtime) {
    if ((state.guard & 1u) != 0u) return state.instance;
    state.guard = static_cast<std::uint8_t>(state.guard | 1u);
    state.instance = runtime.Register(
        runtime.GetSingletonManager(), "palActorMessageAdapter", 0x3ebu,
        kFactoryAddress, kDestroyAddress);
    return state.instance;
}

std::uint32_t ActorMessageAdapter::Create(
    IActorMessageAdapterRuntime& runtime) {
    const auto address = runtime.Allocate(kBytes);
    if (address == 0u) return 0u;
    try {
        return Construct(address, runtime);
    } catch (...) {
        runtime.Free(address);
        throw;
    }
}

std::uint32_t ActorMessageAdapter::Construct(
    const std::uint32_t address, IActorMessageAdapterRuntime& runtime) {
    auto& object = runtime.Access(address);
    auto& collection = object.collection_;
    for (const auto offset : {0x0cu, 0x10u, 0x14u, 0x18u,
                              0x1cu, 0x20u, 0x24u}) {
        collection.Write(offset, 0u);
    }
    object.trailing_owned_object_ = 0u;
    collection.Write(0x08u, kCopyCallback);
    collection.Write(0x00u, kCreateCallback);
    collection.Write(0x04u, kFreeCallback);
    static_cast<void>(NilStringDataObject::InitializeSentinel(
        collection, address, runtime));
    CallbackCollectionCursor::Clear(collection, runtime);
    return address;
}

void ActorMessageAdapter::Clear(
    const std::uint32_t address, IActorMessageAdapterRuntime& runtime) {
    CallbackCollectionCursor::Clear(runtime.Access(address).collection_,
                                    runtime);
}

void ActorMessageAdapter::Destroy(
    const std::uint32_t address, IActorMessageAdapterRuntime& runtime) {
    auto& object = runtime.Access(address);
    CallbackCollectionCursor::Clear(object.collection_, runtime);
    CallbackCollectionCursor::DestroyStorage(
        object.collection_, object.trailing_owned_object_, runtime);
}

void ActorMessageAdapter::DestroyAndFree(
    const std::uint32_t address, IActorMessageAdapterRuntime& runtime) {
    if (address == 0u) return;
    Destroy(address, runtime);
    runtime.Free(address);
}

}  // namespace pal4
