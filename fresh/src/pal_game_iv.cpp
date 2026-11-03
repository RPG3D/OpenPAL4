#include "pal4/pal_game_iv.h"

#include <initializer_list>

namespace pal4 {
namespace {

void ReleaseSharedString(std::uint32_t record, IPalGameIVRuntime& runtime) {
    const auto text = runtime.ReadWord(record);
    if (text != 0u) {
        const auto references = runtime.ReadByte(text - 1u);
        if (references == 0u || references == 0xffu)
            runtime.Free(text - 1u);
        else
            runtime.WriteByte(text - 1u,
                              static_cast<std::uint8_t>(references - 1u));
    }
    runtime.WriteWord(record, 0u);
    runtime.WriteWord(record + 4u, 0u);
    runtime.WriteWord(record + 8u, 0u);
}

void CleanupCollection(std::uint32_t object, IPalGameIVRuntime& runtime) {
    const auto begin = runtime.ReadWord(object + 0x1a0u);
    const auto end = runtime.ReadWord(object + 0x1a4u);
    for (auto cursor = begin; cursor != end; cursor += 12u)
        runtime.CleanupString(cursor);
    runtime.Free(begin);
    for (auto offset : {0x1a0u, 0x1a4u, 0x1a8u})
        runtime.WriteWord(object + offset, 0u);
}

}  // namespace

std::uint32_t PalGameIV::GetInstance(
    PalGameIVRegistration& registration, IPalGameIVRuntime& runtime) {
    if ((registration.guard & 1u) == 0u) {
        registration.guard |= 1u;
        const auto manager = runtime.SingletonManager();
        registration.instance = runtime.RegisterSingleton(
            manager, "PALGameIV", 1046u, 0x5b5b30u, 0x5b5b90u);
    }
    return registration.instance;
}

std::uint32_t PalGameIV::Create(IPalGameIVRuntime& runtime) {
    const auto object = runtime.Allocate(444u);
    if (object == 0u) return 0u;
    try {
        return Construct(object, runtime.ConstructorStackByte(), runtime);
    } catch (...) {
        runtime.Free(object);
        throw;
    }
}

void PalGameIV::Destroy(std::uint32_t object, IPalGameIVRuntime& runtime) {
    if (object == 0u) return;
    Destruct(object, runtime);
    runtime.Free(object);
}

std::uint32_t PalGameIV::Construct(
    std::uint32_t object, std::uint8_t stack_byte,
    IPalGameIVRuntime& runtime) {
    runtime.InitializeVariables(object);
    unsigned stage = 0u;
    try {
        runtime.ConstructBone(object + 0x178u);
        stage = 1u;
        runtime.InitializeString(object + 0x190u);
        stage = 3u;
        runtime.WriteWord(object + 0x1a0u, 0u);
        runtime.WriteByte(object + 0x19cu, stack_byte);
        runtime.WriteWord(object + 0x1a4u, 0u);
        runtime.WriteWord(object + 0x1a8u, 0u);
        runtime.ConstructDictionary(object + kTextureDictionaryOffset);
    } catch (...) {
        if (stage == 3u) {
            CleanupCollection(object, runtime);
            runtime.CleanupString(object + 0x190u);
        }
        Cleanup(object, runtime);
        throw;
    }
    runtime.WriteWord(object + 0x174u, 0u);
    runtime.WriteWord(object + 0x188u, 0u);
    runtime.WriteWord(object + 0x18cu, 0u);
    return object;
}

void PalGameIV::Cleanup(std::uint32_t object, IPalGameIVRuntime& runtime) {
    runtime.Free(runtime.ReadWord(object + 344u));
    for (auto offset : {344u, 348u, 352u})
        runtime.WriteWord(object + offset, 0u);
    for (auto offset : {328u, 312u, 296u})
        ReleaseSharedString(object + offset, runtime);
}

void PalGameIV::Destruct(std::uint32_t object,
                         IPalGameIVRuntime& runtime) {
    runtime.DestroyDictionary(object + kTextureDictionaryOffset);
    CleanupCollection(object, runtime);
    runtime.CleanupString(object + 0x190u);
    Cleanup(object, runtime);
}

}  // namespace pal4
