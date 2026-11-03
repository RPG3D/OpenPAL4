#include "pal4/win32_startup_threads.h"

#include <windows.h>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <thread>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

std::uint32_t WaitForPublication(std::uint32_t& parameter) {
    const auto deadline = std::chrono::steady_clock::now() +
                          std::chrono::seconds(5);
    std::uint32_t value = 0u;
    while (std::chrono::steady_clock::now() < deadline) {
        value = std::atomic_ref(parameter).load(std::memory_order_acquire);
        if (value != 0u) return value;
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    return 0u;
}

void TestLiveThreadAndNormalJoin() {
    pal4::GameRenderState game{0x800000u};
    std::uint8_t initialized = 0u;
    std::uint32_t thread_count = 0u;
    std::uint32_t parameter = 0u;
    pal4::Win32StartupThreads threads{game, initialized, thread_count};
    Check(threads.CreateRandomizedThread(parameter, 0u),
          "CreateThread starts the randomized worker");

    const auto published = WaitForPublication(parameter);
    MEMORY_BASIC_INFORMATION memory{};
    const auto queried = VirtualQuery(reinterpret_cast<const void*>(published),
                                      &memory, sizeof(memory));
    const bool live = published != 0u && queried == sizeof(memory) &&
                      memory.State == MEM_COMMIT &&
                      (memory.Protect & PAGE_GUARD) == 0u &&
                      (memory.Protect & PAGE_NOACCESS) == 0u;
    std::atomic_ref(parameter).store(0u, std::memory_order_release);
    std::atomic_ref(initialized).store(1u, std::memory_order_release);
    game.WriteShared(0x28u, 1u);
    const bool joined = threads.JoinAll();
    Check(live && joined &&
              std::atomic_ref(parameter).load(std::memory_order_acquire) != 0u &&
              std::atomic_ref(thread_count).load(std::memory_order_relaxed) == 1u,
          "worker publishes committed stack memory and exits after both gates");
}

void TestCounterThread() {
    pal4::GameRenderState game{0x800000u};
    std::uint8_t initialized = 1u;
    std::uint32_t thread_count = 0u;
    game.WriteShared(0x28u, 1u);
    pal4::Win32StartupThreads threads{game, initialized, thread_count};
    Check(threads.CreateCounterThread(3u) && threads.JoinAll() &&
              std::atomic_ref(thread_count).load(std::memory_order_relaxed) == 12u,
          "counter callback runs on a Win32 thread");
}

void TestStopBeforeStartupCompletes() {
    pal4::GameRenderState game{0x800000u};
    std::uint8_t initialized = 0u;
    std::uint32_t thread_count = 0u;
    std::uint32_t parameter = 0u;
    pal4::Win32StartupThreads threads{game, initialized, thread_count};
    Check(threads.CreateRandomizedThread(parameter, 0u),
          "blocked randomized worker starts");
    const auto published = WaitForPublication(parameter);
    const auto start = std::chrono::steady_clock::now();
    const bool stopped = threads.StopAndJoin();
    const auto elapsed = std::chrono::steady_clock::now() - start;
    Check(published != 0u && stopped &&
              elapsed < std::chrono::seconds(2) &&
              !threads.CreateCounterThread(0u),
          "stop event releases blocked worker before parameter lifetime ends");
}

}  // namespace

int main() {
    TestLiveThreadAndNormalJoin();
    TestCounterThread();
    TestStopBeforeStartupCompletes();
    return EXIT_SUCCESS;
}
