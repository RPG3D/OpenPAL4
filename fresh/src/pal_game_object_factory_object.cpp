#include "pal4/pal_game_object_factory_object.h"

namespace pal4 {
namespace {

std::uint32_t CreateHandle(const std::uint32_t vtable,
                           IPalGameObjectFactoryObjectRuntime& runtime) {
    const auto object = runtime.Allocate(8u);
    if (object == 0u) return 0u;
    runtime.WriteWord(object + 4u, 0u);
    runtime.WriteWord(object, vtable);
    return object;
}

}  // namespace

std::uint32_t PalGameObjectFactoryObject::ReturnZero(
    std::uint32_t) noexcept {
    return 0u;
}

std::uint32_t PalGameObjectFactoryObject::ReturnOne(
    std::uint32_t) noexcept {
    return 1u;
}

std::uint32_t PalGameObjectFactoryObject::CreateClimb(
    IPalGameObjectFactoryObjectRuntime& runtime) {
    return CreateHandle(0x849318u, runtime);
}

std::uint32_t PalGameObjectFactoryObject::ConstructNormal(
    const std::uint32_t object,
    IPalGameObjectFactoryObjectRuntime& runtime) {
    runtime.WriteWord(object + 4u, 0u);
    runtime.WriteWord(object, 0x849340u);
    return object;
}

std::uint32_t PalGameObjectFactoryObject::CreateNormal(
    IPalGameObjectFactoryObjectRuntime& runtime) {
    const auto object = runtime.Allocate(8u);
    return object != 0u ? ConstructNormal(object, runtime) : 0u;
}

std::uint32_t PalGameObjectFactoryObject::SetMovementFlagOne(
    std::uint32_t,
    IPalGameObjectFactoryControlRuntime& runtime) {
    runtime.WriteWord(runtime.MovementAdapter() + 16u, 1u);
    return 0u;
}

std::uint32_t PalGameObjectFactoryObject::SetMovementFlagZero(
    IPalGameObjectFactoryControlRuntime& runtime) {
    runtime.WriteWord(runtime.MovementAdapter() + 16u, 0u);
    return 1u;
}

std::uint32_t PalGameObjectFactoryObject::CreateSound(
    IPalGameObjectFactoryObjectRuntime& runtime) {
    return CreateHandle(0x849354u, runtime);
}

std::uint32_t PalGameObjectFactoryObject::SetActorPropertyZero(
    std::uint32_t,
    IPalGameObjectFactoryControlRuntime& runtime) {
    runtime.InvokeActorControlProperty(runtime.ActorControl() + 140u,
                                       32u, 0u);
    return 0u;
}

std::uint32_t PalGameObjectFactoryObject::SetActorPropertyOne(
    IPalGameObjectFactoryControlRuntime& runtime) {
    runtime.InvokeActorControlProperty(runtime.ActorControl() + 140u,
                                       32u, 1u);
    return 1u;
}

std::uint32_t PalGameObjectFactoryObject::CreateLocalFloat(
    IPalGameObjectFactoryObjectRuntime& runtime) {
    return CreateHandle(0x849368u, runtime);
}

std::uint32_t PalGameObjectFactoryObject::CreateLocalFloor(
    IPalGameObjectFactoryObjectRuntime& runtime) {
    return CreateHandle(0x8493c0u, runtime);
}

std::uint32_t PalGameObjectFactoryObject::CreateMachine(
    IPalGameObjectFactoryObjectRuntime& runtime) {
    return CreateHandle(0x8493d4u, runtime);
}

void PalGameObjectFactoryObject::DestructBase(
    const std::uint32_t object,
    IPalGameObjectFactoryObjectRuntime& runtime) {
    runtime.WriteWord(object, 0x84932cu);
    runtime.WriteWord(object + 4u, 0u);
}

std::uint32_t PalGameObjectFactoryObject::Delete(
    const std::uint32_t object, const std::uint8_t flags,
    IPalGameObjectFactoryObjectRuntime& runtime) {
    DestructBase(object, runtime);
    if ((flags & 1u) != 0u) runtime.FreeMemory(object);
    return object;
}

}  // namespace pal4
