#pragma once

#include <cstdint>
#include <string_view>

namespace pal4 {

class IActorManagerMethodErrorRuntime {
public:
    virtual ~IActorManagerMethodErrorRuntime() = default;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual bool AllocateText(std::uint32_t record,
                                            std::uint32_t length,
                                            std::uint8_t mode) = 0;
    virtual void CopyText(std::uint32_t destination,
                          std::string_view text) = 0;
    virtual void RegisterCleanup(std::uint32_t function) = 0;
    [[nodiscard]] virtual std::uint32_t InvokeHandler(
        std::uint32_t handler, std::uint32_t argument) = 0;
    virtual void Free(std::uint32_t address) = 0;
};

class ActorManagerMethodErrorText final {
public:
    static constexpr std::uint32_t kGuard = 0x978ad0u;
    static constexpr std::uint32_t kRecord = 0x978ae0u;
    static constexpr std::uint32_t kCleanup = 0x533210u;
    static constexpr std::string_view kMessage =
        "Error on ptnStdStringMethodProperty";

    [[nodiscard]] static std::uint32_t GetOrHandle(
        std::uint32_t object, std::uint32_t argument,
        IActorManagerMethodErrorRuntime& runtime);
    static void Cleanup(IActorManagerMethodErrorRuntime& runtime);
};

class ActorManagerPlayerMethodErrorText final {
public:
    static constexpr std::uint32_t kGuard = 0x978a98u;
    static constexpr std::uint32_t kRecord = 0x978aa0u;
    static constexpr std::uint32_t kCleanup = 0x53ae30u;

    [[nodiscard]] static std::uint32_t GetOrHandle(
        std::uint32_t object, std::uint32_t argument,
        IActorManagerMethodErrorRuntime& runtime);
    static void Cleanup(IActorManagerMethodErrorRuntime& runtime);
};

}  // namespace pal4
