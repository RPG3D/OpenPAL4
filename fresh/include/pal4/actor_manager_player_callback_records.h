#pragma once

#include <cstdint>

namespace pal4 {

class IActorManagerPlayerCallbackRecordRuntime {
public:
    virtual ~IActorManagerPlayerCallbackRecordRuntime() = default;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    virtual void Free(std::uint32_t address) = 0;
};

class ActorManagerPlayerCallbackRecords final {
public:
    static constexpr std::uint32_t kSetterVtable = 0x845284u;
    static constexpr std::uint32_t kFirstVtable = 0x84528cu;
    static constexpr std::uint32_t kSecondVtable = 0x845294u;

    [[nodiscard]] static std::uint32_t ConstructSetter(
        std::uint32_t object, std::uint32_t callback,
        std::uint32_t context,
        IActorManagerPlayerCallbackRecordRuntime& runtime);
    static void MarkFirst(std::uint32_t object,
                          IActorManagerPlayerCallbackRecordRuntime& runtime);
    static void MarkSecond(std::uint32_t object,
                           IActorManagerPlayerCallbackRecordRuntime& runtime);
    [[nodiscard]] static std::uint32_t DestroyFirst(
        std::uint32_t object, std::uint32_t flags,
        IActorManagerPlayerCallbackRecordRuntime& runtime);
    [[nodiscard]] static std::uint32_t DestroySecond(
        std::uint32_t object, std::uint32_t flags,
        IActorManagerPlayerCallbackRecordRuntime& runtime);
    [[nodiscard]] static std::uint32_t DestroyFirstInline(
        std::uint32_t object, std::uint32_t flags,
        IActorManagerPlayerCallbackRecordRuntime& runtime);
    [[nodiscard]] static std::uint32_t DestroySecondInline(
        std::uint32_t object, std::uint32_t flags,
        IActorManagerPlayerCallbackRecordRuntime& runtime);
};

}  // namespace pal4
