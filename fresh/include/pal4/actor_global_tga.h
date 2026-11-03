#pragma once

#include <cstdint>
#include <functional>

namespace pal4 {

class IActorGlobalTgaRuntime {
public:
    virtual ~IActorGlobalTgaRuntime() = default;
    [[nodiscard]] virtual std::uint32_t InitializeHeader(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual int RegisterAtExit(
        const std::function<void()>& callback) = 0;
};

class ActorGlobalTga final {
public:
    static constexpr std::uint32_t kHeaderAddress = 0x8f60b0u;

    [[nodiscard]] static int Initialize(IActorGlobalTgaRuntime& runtime);
    [[nodiscard]] static std::uint32_t InitializeHeader(
        IActorGlobalTgaRuntime& runtime);
    static void CleanupHeader();
};

}  // namespace pal4
