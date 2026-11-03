#include "pal4/pal_texture_dictionary.h"

namespace pal4 {

std::uint32_t PalTextureDictionaryAccess::Get(
    PalTextureDictionaryAccessState& state,
    IPalTextureDictionaryAccessRuntime& runtime) {
    if ((state.guard & 1u) != 0u) {
        return state.instance;
    }
    state.guard = static_cast<std::uint8_t>(state.guard | 1u);
    state.instance = runtime.Register(
        runtime.GetSingletonManager(), "palTextureDict", 1000u,
        kFactoryAddress, kDestroyAddress);
    return state.instance;
}

std::uint32_t PalTextureDictionaryRuntimeStub::Allocate(
    std::uint32_t /*bytes*/) {
    const auto address = next_address_;
    next_address_ += sizeof(PalTextureDictionaryState);
    objects_.emplace(address, PalTextureDictionaryState{});
    return address;
}

void PalTextureDictionaryRuntimeStub::Free(
    const std::uint32_t address) {
    objects_.erase(address);
}

PalTextureDictionaryState& PalTextureDictionaryRuntimeStub::Access(
    const std::uint32_t address) {
    return objects_.at(address);
}

void PalTextureDictionaryRuntimeStub::BaseConstruct(
    std::uint32_t /*address*/) {}

void PalTextureDictionaryRuntimeStub::BaseDestruct(
    std::uint32_t /*address*/) {}

std::uint32_t PalTextureDictionaryRuntimeStub::BaseAddTexture(
    std::uint32_t /*dictionary*/, std::uint32_t /*texture*/) {
    return 0u;
}

std::uint32_t PalTextureDictionary::Create(
    IPalTextureDictionaryRuntime& runtime) {
    const auto address = runtime.Allocate(sizeof(PalTextureDictionaryState));
    if (address == 0u) {
        return 0u;
    }
    try {
        return Construct(address, runtime);
    } catch (...) {
        runtime.Free(address);
        throw;
    }
}

std::uint32_t PalTextureDictionary::Construct(
    const std::uint32_t address,
    IPalTextureDictionaryRuntime& runtime) {
    runtime.BaseConstruct(address);
    runtime.Access(address).vtable = kVtableAddress;
    return address;
}

std::uint32_t PalTextureDictionary::Delete(
    const std::uint32_t address, const std::uint8_t flags,
    IPalTextureDictionaryRuntime& runtime) {
    Destruct(address, runtime);
    if ((flags & 1u) != 0u) {
        runtime.Free(address);
    }
    return address;
}

void PalTextureDictionary::Destruct(
    const std::uint32_t address,
    IPalTextureDictionaryRuntime& runtime) {
    runtime.Access(address).vtable = kVtableAddress;
    runtime.BaseDestruct(address);
}

std::uint32_t PalTextureDictionary::AddTexture(
    const std::uint32_t dictionary, const std::uint32_t texture,
    IPalTextureDictionaryRuntime& runtime) {
    return runtime.BaseAddTexture(dictionary, texture);
}

}  // namespace pal4
