#include "pal4/thread_startup_coordinator.h"

#include <atomic>

namespace pal4 {

bool ThreadStartupCoordinator::WaitForInitialization(
    IThreadStartupRuntime& runtime) {
    while (std::atomic_ref(initialized_).load(std::memory_order_acquire) == 0u) {
        if (runtime.ShouldStop()) return false;
        runtime.Sleep(1000u);
    }
    return !runtime.ShouldStop();
}

bool ThreadStartupCoordinator::WaitForGameState(
    IThreadStartupRuntime& runtime) {
    while (game_.ReadShared(0x28u) == 0u) {
        if (runtime.ShouldStop()) return false;
        runtime.Sleep(1000u);
    }
    return !runtime.ShouldStop();
}

std::uint32_t ThreadStartupCoordinator::RunCounterThread(
    const std::uint32_t stack_word, IThreadStartupRuntime& runtime) {
    std::uint32_t counter = stack_word;
    while (std::atomic_ref(initialized_).load(std::memory_order_acquire) == 0u) {
        if (runtime.ShouldStop()) return 0u;
        ++counter;
        runtime.Sleep(1000u);
    }
    if (!WaitForGameState(runtime)) return 0u;
    std::atomic_ref(thread_count_).fetch_add(counter * 4u,
                                              std::memory_order_relaxed);
    return 0u;
}

std::uint32_t ThreadStartupCoordinator::RunRandomizedThread(
    const std::uint32_t parameter_address, const std::uint32_t stack_base,
    const std::uint8_t stack_byte, IThreadStartupRuntime& runtime) {
    runtime.SeedRandom(runtime.Time() + parameter_address);
    const auto stack_pointer = stack_base + runtime.Random() % 0x80000u;
    runtime.WriteParameter(parameter_address, stack_pointer);
    std::atomic_ref(thread_count_).store(1u, std::memory_order_relaxed);
    if (!WaitForInitialization(runtime) || !WaitForGameState(runtime))
        return 0u;
    if (runtime.ReadParameter(parameter_address) != 0u) {
        const auto delay = 8u +
            ((std::atomic_ref(thread_count_).load(std::memory_order_relaxed) +
              stack_byte + game_.ReadShared(0x28u)) & 7u);
        runtime.Sleep(delay * 1000u);
    } else {
        runtime.WriteParameter(parameter_address, stack_pointer);
        game_.WriteShared(0x28u, 1u);
    }
    return 0u;
}

#if defined(_WIN32) && UINTPTR_MAX == UINT32_MAX
std::uint32_t ThreadStartupCoordinator::RunRandomizedThreadOnLiveStack(
    std::uint32_t& parameter, const std::uint8_t stack_byte,
    IThreadStartupRuntime& runtime) {
    volatile std::uint8_t stack[0x80644u];
    stack[0] = 0u;
    stack[0x80643u] = 0u;
    const auto base = static_cast<std::uint32_t>(
        reinterpret_cast<std::uintptr_t>(&stack[0]));
    const auto parameter_address = static_cast<std::uint32_t>(
        reinterpret_cast<std::uintptr_t>(&parameter));
    return RunRandomizedThread(parameter_address, base, stack_byte, runtime);
}
#endif

}  // namespace pal4
