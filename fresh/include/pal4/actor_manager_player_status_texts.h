#pragma once

#include <cstdint>
#include <string_view>

namespace pal4 {

class IActorManagerPlayerStatusTextRuntime {
public:
    virtual ~IActorManagerPlayerStatusTextRuntime() = default;
    [[nodiscard]] virtual std::int32_t ReadSignedWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    virtual void SetExceptionMessage(std::uint32_t record,
                                     std::string_view text) = 0;
    virtual void SetErrorFlag(std::uint32_t record, std::uint8_t value) = 0;
    [[nodiscard]] virtual bool AllocateText(std::uint32_t record,
                                             std::uint32_t length,
                                             std::uint8_t mode) = 0;
    virtual void CopyText(std::uint32_t destination,
                          std::string_view text) = 0;
    virtual void RegisterExit(std::uint32_t callback) = 0;
    virtual void ReleaseText(std::uint32_t record) = 0;
};

class ActorManagerPlayerStatusTexts final {
public:
    static constexpr std::uint32_t kRecords = 0x8f6168u;
    static constexpr std::uint32_t kCleanupEntry = 0x538310u;
    static constexpr std::uint32_t kEmptyText = 0x841964u;

    // Host text view over the same original table, without legacy static
    // string initialization/atexit ownership.
    [[nodiscard]] static std::string_view TextForRole(std::uint32_t index);

    [[nodiscard]] std::uint32_t Lookup(
        std::uint32_t actor, IActorManagerPlayerStatusTextRuntime& runtime);
    void Cleanup(IActorManagerPlayerStatusTextRuntime& runtime) const;

private:
    bool initialized_ = false;
};

}  // namespace pal4
