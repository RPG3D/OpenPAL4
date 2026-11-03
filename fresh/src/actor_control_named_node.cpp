#include "pal4/actor_control_named_node.h"

#include <stdexcept>

namespace pal4 {

std::uint8_t ActorControlNamedNodeRuntimeStub::ReadByte(std::uint32_t) {
    return 0u;
}
std::uint32_t ActorControlNamedNodeRuntimeStub::ReadDword(std::uint32_t) {
    return 0u;
}
void ActorControlNamedNodeRuntimeStub::WriteByte(std::uint32_t,
                                                  std::uint8_t) {}
void ActorControlNamedNodeRuntimeStub::WriteDword(std::uint32_t,
                                                   std::uint32_t) {}
std::uint32_t ActorControlNamedNodeRuntimeStub::TextLength(std::uint32_t) {
    return 0u;
}
void ActorControlNamedNodeRuntimeStub::SetErrorFlag(std::uint32_t,
                                                     std::uint8_t) {}
void ActorControlNamedNodeRuntimeStub::ReallocateText(std::uint32_t,
                                                       std::uint32_t) {}
void ActorControlNamedNodeRuntimeStub::CopyBytes(std::uint32_t,
                                                 std::uint32_t,
                                                 std::uint32_t) {}
void ActorControlNamedNodeRuntimeStub::ThrowLength() {
    throw std::length_error("actor control name exceeds string limit");
}
std::uint32_t ActorControlNamedNodeRuntimeStub::FreeAddress(std::uint32_t) {
    return 0u;
}
std::uint32_t ActorControlNamedNodeRuntimeStub::InitializeActorControl() {
    return 0u;
}
std::uint32_t ActorControlNamedNodeRuntimeStub::FloorAttributeInstance() {
    return 0u;
}
std::int32_t ActorControlNamedNodeRuntimeStub::CompareBytes(
    std::uint32_t, std::uint32_t, std::uint32_t) { return 0; }
std::uint32_t ActorControlNamedNodeRuntimeStub::AudioManager() { return 0u; }
void ActorControlNamedNodeRuntimeStub::PlaySound(
    std::uint32_t, std::uint32_t, std::uint32_t, std::uint32_t,
    std::uint32_t, std::uint32_t, std::uint32_t) {}
void ActorControlNamedNodeRuntimeStub::InvokeSoundCallback(
    std::uint32_t, std::uint32_t) {}

std::uint32_t ActorControlNamedNode::Construct(
    std::uint32_t node, std::uint32_t source,
    IActorControlNamedNodeRuntime& runtime) {
    runtime.WriteDword(node, kBaseVtable);
    runtime.WriteByte(node + 4u, static_cast<std::uint8_t>(source));
    runtime.WriteDword(node + 8u, 0u);
    runtime.WriteDword(node + 12u, 0u);
    runtime.WriteDword(node + 16u, 0u);
    const auto length = runtime.TextLength(source);
    if (length > 0xfffffffdu) runtime.ThrowLength();
    runtime.SetErrorFlag(node + 4u, 1u);
    if (length != 0u) {
        runtime.ReallocateText(node + 4u, length);
        const auto data = runtime.ReadDword(node + 8u);
        runtime.CopyBytes(data, source, length);
        runtime.WriteDword(node + 12u, length);
        runtime.WriteByte(data + length, 0u);
    }
    runtime.WriteDword(node + 20u, 0u);
    runtime.WriteDword(node + 24u, 0u);
    runtime.WriteDword(node, kDerivedVtable);
    return node;
}

std::uint32_t ActorControlNamedNode::Destruct(
    std::uint32_t node, IActorControlNamedNodeRuntime& runtime) {
    auto result = runtime.ReadDword(node + 8u);
    if (result != 0u) {
        const auto storage = result - 1u;
        const auto references = runtime.ReadByte(storage);
        if (references == 0u || references == 0xffu) {
            result = runtime.FreeAddress(storage);
        } else {
            runtime.WriteByte(storage,
                              static_cast<std::uint8_t>(references - 1u));
            result = storage;
        }
    }
    runtime.WriteDword(node + 8u, 0u);
    runtime.WriteDword(node + 12u, 0u);
    runtime.WriteDword(node + 16u, 0u);
    runtime.WriteDword(node, kBaseVtable);
    return result;
}

std::uint32_t ActorControlNamedNode::TriggerFloorSound(
    std::uint32_t node, IActorControlNamedNodeRuntime& runtime) {
    if (runtime.ReadDword(runtime.InitializeActorControl()) == 0u)
        return 0u;
    const auto floor_text = runtime.FloorAttributeInstance() + 44u;
    const auto empty_size = runtime.TextLength(kEmptyErrorString);
    const auto sound_size = runtime.ReadDword(floor_text + 8u);
    const auto common_size = sound_size < empty_size
                                 ? sound_size : empty_size;
    const auto comparison = runtime.CompareBytes(
        runtime.ReadDword(floor_text + 4u), kEmptyErrorString,
        common_size);
    if (comparison != 0 || sound_size != empty_size) {
        const auto object = runtime.ReadDword(node + 20u);
        if (object != 0u) {
            auto sound = runtime.ReadDword(floor_text + 4u);
            if (sound == 0u) sound = kEmptyText;
            runtime.PlaySound(runtime.AudioManager(), node, sound,
                              object + 172u, 1u, 0x44160000u,
                              0x43960000u);
            const auto callback = runtime.ReadDword(node + 24u);
            if (callback != 0u) {
                auto callback_sound = runtime.ReadDword(floor_text + 4u);
                if (callback_sound == 0u) callback_sound = kEmptyText;
                runtime.InvokeSoundCallback(callback, callback_sound);
            }
        }
    }
    return 1u;
}

std::uint32_t ActorControlNamedNode::DeleteBase(
    std::uint32_t node, std::uint32_t flags,
    IActorControlNamedNodeRuntime& runtime) {
    runtime.WriteDword(node, kBaseVtable);
    if ((flags & 1u) != 0u)
        static_cast<void>(runtime.FreeAddress(node));
    return node;
}

std::uint32_t ActorControlNamedNode::SetOwner(
    std::uint32_t node, std::uint32_t owner,
    IActorControlNamedNodeRuntime& runtime) {
    runtime.WriteDword(node + 20u, owner);
    return owner;
}

}  // namespace pal4
