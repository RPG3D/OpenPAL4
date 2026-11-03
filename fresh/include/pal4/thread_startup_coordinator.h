#pragma once

#include "pal4/game_render_system.h"

#include <cstdint>

namespace pal4 {

class IThreadStartupRuntime {
public:
    virtual ~IThreadStartupRuntime() = default;
    virtual void Sleep(std::uint32_t milliseconds) = 0;
    [[nodiscard]] virtual bool ShouldStop() const { return false; }
    [[nodiscard]] virtual std::uint32_t Time() = 0;
    virtual void SeedRandom(std::uint32_t seed) = 0;
    [[nodiscard]] virtual std::uint32_t Random() = 0;
    // Implementations must access the ApplicationEntry parameter atomically.
    virtual void WriteParameter(std::uint32_t address,
                                std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t ReadParameter(
        std::uint32_t address) = 0;
};

class ThreadStartupCoordinator final {
public:
    ThreadStartupCoordinator(GameRenderState& game, std::uint8_t& initialized,
                             std::uint32_t& thread_count)
        : game_(game), initialized_(initialized), thread_count_(thread_count) {}

    [[nodiscard]] std::uint32_t RunCounterThread(
        std::uint32_t stack_word, IThreadStartupRuntime& runtime);
    [[nodiscard]] std::uint32_t RunRandomizedThread(
        std::uint32_t parameter_address, std::uint32_t stack_base,
        std::uint8_t stack_byte, IThreadStartupRuntime& runtime);
#if defined(_WIN32) && UINTPTR_MAX == UINT32_MAX
    [[nodiscard]] std::uint32_t RunRandomizedThreadOnLiveStack(
        std::uint32_t& parameter, std::uint8_t stack_byte,
        IThreadStartupRuntime& runtime);
#endif

private:
    [[nodiscard]] bool WaitForInitialization(IThreadStartupRuntime& runtime);
    [[nodiscard]] bool WaitForGameState(IThreadStartupRuntime& runtime);

    GameRenderState& game_;
    std::uint8_t& initialized_;
    std::uint32_t& thread_count_;
};

}  // namespace pal4
