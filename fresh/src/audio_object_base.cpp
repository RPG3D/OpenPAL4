#include "pal4/audio_object_base.h"

#include <cstring>

namespace pal4 {

void AudioObjectBase::Initialize(void* object) {
    std::memcpy(object, &kVtable, sizeof(kVtable));
}

void* AudioObjectBase::DestroyAt(
    void* object, const std::uint8_t flags,
    IAudioObjectBaseRuntime& runtime) {
    Initialize(object);
    if ((flags & 1u) != 0u)
        runtime.FreeObject(object);
    return object;
}

void* AudioObjectBase::MemberAt40(void* object) {
    return static_cast<std::byte*>(object) + 40u;
}

void* AudioObjectBase::MemberAt32(void* object) {
    return static_cast<std::byte*>(object) + 32u;
}

std::uint64_t AudioObjectBase::ReadAt24(const void* object) {
    std::uint64_t value = 0u;
    std::memcpy(&value, static_cast<const std::byte*>(object) + 24u,
                sizeof(value));
    return value;
}

std::uint32_t AudioObjectBase::WriteAt16(
    void* object, const std::uint32_t value) {
    std::memcpy(static_cast<std::byte*>(object) + 16u, &value,
                sizeof(value));
    return value;
}

void* AudioObjectBase::MemberAt80(void* object) {
    return static_cast<std::byte*>(object) + 80u;
}

std::uint64_t AudioObjectBase::ReadAt72(const void* object) {
    std::uint64_t value = 0u;
    std::memcpy(&value, static_cast<const std::byte*>(object) + 72u,
                sizeof(value));
    return value;
}

}  // namespace pal4
