#pragma once

#include "pal4/game_render_system.h"

#include <cstdint>
#include <memory>

namespace pal4 {

class Win32StartupThreads final {
public:
    Win32StartupThreads(GameRenderState& game, std::uint8_t& initialized,
                        std::uint32_t& thread_count);
    ~Win32StartupThreads();

    Win32StartupThreads(const Win32StartupThreads&) = delete;
    Win32StartupThreads& operator=(const Win32StartupThreads&) = delete;

    [[nodiscard]] bool CreateCounterThread(std::uint32_t stack_word);
    [[nodiscard]] bool CreateRandomizedThread(std::uint32_t& parameter,
                                               std::uint8_t stack_byte);
    [[nodiscard]] bool JoinAll();
    [[nodiscard]] bool StopAndJoin();

private:
    struct State;
    std::unique_ptr<State> state_;
};

}  // namespace pal4
