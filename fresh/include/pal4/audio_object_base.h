#pragma once

#include <cstddef>
#include <cstdint>

namespace pal4 {

class IAudioObjectBaseRuntime {
public:
    virtual ~IAudioObjectBaseRuntime() = default;
    virtual void FreeObject(void* object) = 0;
};

class AudioObjectBase final {
public:
    static constexpr std::uint32_t kVtable = 0x84b534u;

    // IDA 0x65c6d0/0x65c6e0: base vtable init and deleting destructor.
    static void Initialize(void* object);
    [[nodiscard]] static void* DestroyAt(
        void* object, std::uint8_t flags, IAudioObjectBaseRuntime& runtime);

    // IDA 0x65c700..0x65c750: shared audio-object vtable accessors.
    [[nodiscard]] static void* MemberAt40(void* object);
    [[nodiscard]] static void* MemberAt32(void* object);
    [[nodiscard]] static std::uint64_t ReadAt24(const void* object);
    [[nodiscard]] static std::uint32_t WriteAt16(
        void* object, std::uint32_t value);
    [[nodiscard]] static void* MemberAt80(void* object);
    [[nodiscard]] static std::uint64_t ReadAt72(const void* object);
};

}  // namespace pal4
