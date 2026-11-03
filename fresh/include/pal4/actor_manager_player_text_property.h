#pragma once

#include <cstdint>
#include <string_view>

namespace pal4 {

class IActorManagerPlayerTextRuntime {
public:
    virtual ~IActorManagerPlayerTextRuntime() = default;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    virtual void SetErrorFlag(std::uint32_t record,
                              std::uint8_t value) = 0;
    virtual void DetachAndReset(std::uint32_t record) = 0;
    [[nodiscard]] virtual bool AllocateText(std::uint32_t record,
                                             std::uint32_t length,
                                             std::uint8_t mode) = 0;
    virtual void SetTextSize(std::uint32_t record,
                             std::uint32_t length) = 0;
    virtual void CopyBytes(std::uint32_t destination,
                           std::uint32_t source,
                           std::uint32_t count) = 0;
    virtual void WriteLiteral(std::uint32_t destination,
                              std::string_view text) = 0;
    virtual void RegisterCleanup(std::uint32_t function) = 0;
    virtual void Free(std::uint32_t address) = 0;
};

class ActorManagerPlayerTextProperty final {
public:
    static constexpr std::uint32_t kFallback = 0x8f6150u;
    static constexpr std::uint32_t kGuard = 0x8f6160u;
    static constexpr std::uint32_t kEmptyText = 0x841964u;
    static constexpr std::uint32_t kCleanupFunction = 0x539e10u;

    [[nodiscard]] static std::uint32_t Assign(
        std::uint32_t actor, std::int32_t index, std::uint32_t source,
        IActorManagerPlayerTextRuntime& runtime);
    static void Cleanup(IActorManagerPlayerTextRuntime& runtime);
};

}  // namespace pal4
