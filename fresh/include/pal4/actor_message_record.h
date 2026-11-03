#pragma once

#include <array>
#include <cstdint>

namespace pal4 {

class ActorMessageRecord final {
public:
    static constexpr std::uint32_t kBytes = 0x48u;
    static constexpr std::uint32_t kTextBytes = 0x40u;

    [[nodiscard]] std::array<std::uint8_t, kBytes>& Bytes() noexcept {
        return bytes_;
    }
    [[nodiscard]] const std::array<std::uint8_t, kBytes>& Bytes()
        const noexcept { return bytes_; }
    [[nodiscard]] std::uint32_t ReadDword(std::uint32_t offset) const;
    void WriteDword(std::uint32_t offset, std::uint32_t value);
    [[nodiscard]] std::uint32_t CopyFrom(
        const ActorMessageRecord& source);
    void Initialize() noexcept;

private:
    std::array<std::uint8_t, kBytes> bytes_{};
};

class IActorMessageRecordRuntime {
public:
    virtual ~IActorMessageRecordRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Allocate(
        std::uint32_t bytes) = 0;
    [[nodiscard]] virtual ActorMessageRecord& Access(
        std::uint32_t address) = 0;
};

class ActorMessageRecordFactory final {
public:
    [[nodiscard]] static std::uint32_t Create(
        IActorMessageRecordRuntime& runtime);
};

}  // namespace pal4
