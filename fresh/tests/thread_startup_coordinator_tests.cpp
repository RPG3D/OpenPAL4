#include "pal4/thread_startup_coordinator.h"

#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

class Runtime final : public pal4::IThreadStartupRuntime {
public:
    pal4::GameRenderState* game = nullptr;
    std::uint8_t* initialized = nullptr;
    std::vector<std::uint32_t> sleeps;
    std::vector<std::string> events;
    std::uint32_t parameter_address = 0u;
    std::uint32_t parameter_value = 0u;
    std::uint32_t seed = 0u;
    std::uint32_t random_value = 0u;
    std::uint32_t initialize_on_sleep = 0u;
    std::uint32_t game_ready_on_sleep = 0u;
    bool clear_parameter_before_read = false;
#if defined(_WIN32) && UINTPTR_MAX == UINT32_MAX
    bool use_live_parameter = false;
    bool live_stack_observed = false;
#endif

    void Sleep(const std::uint32_t milliseconds) override {
        events.emplace_back("sleep");
        sleeps.push_back(milliseconds);
#if defined(_WIN32) && UINTPTR_MAX == UINT32_MAX
        if (use_live_parameter && parameter_value != 0u) {
            volatile std::uint8_t local = 0u;
            const auto current_stack = reinterpret_cast<std::uintptr_t>(&local);
            const auto published = static_cast<std::uintptr_t>(parameter_value);
            Check(published > current_stack &&
                      published - current_stack < 0x90000u,
                  "published address is in the active thread stack");
            *reinterpret_cast<volatile std::uint8_t*>(published) = 0x5au;
            live_stack_observed =
                *reinterpret_cast<volatile std::uint8_t*>(published) == 0x5au;
        }
#endif
        if (sleeps.size() == initialize_on_sleep) *initialized = 1u;
        if (sleeps.size() == game_ready_on_sleep)
            game->Write(0x28u, 1u);
    }
    std::uint32_t Time() override {
        events.emplace_back("time");
        return 100u;
    }
    void SeedRandom(const std::uint32_t value) override {
        events.emplace_back("seed");
        seed = value;
    }
    std::uint32_t Random() override {
        events.emplace_back("random");
        return random_value;
    }
    void WriteParameter(const std::uint32_t address,
                        const std::uint32_t value) override {
        Check(address == parameter_address, "parameter address");
        events.emplace_back("write");
        parameter_value = value;
#if defined(_WIN32) && UINTPTR_MAX == UINT32_MAX
        if (use_live_parameter)
            *reinterpret_cast<std::uint32_t*>(address) = value;
#endif
    }
    std::uint32_t ReadParameter(const std::uint32_t address) override {
        Check(address == parameter_address, "parameter read address");
        events.emplace_back("read");
#if defined(_WIN32) && UINTPTR_MAX == UINT32_MAX
        if (use_live_parameter)
            return *reinterpret_cast<const std::uint32_t*>(address);
#endif
        return clear_parameter_before_read ? 0u : parameter_value;
    }
};

struct Fixture {
    pal4::GameRenderState game{0x800000u};
    std::uint8_t initialized = 0u;
    std::uint32_t thread_count = 0u;
    pal4::ThreadStartupCoordinator coordinator{game, initialized,
                                                thread_count};
    Runtime runtime;
    Fixture() {
        runtime.game = &game;
        runtime.initialized = &initialized;
        runtime.parameter_address = 0x3000u;
    }
};

void TestCounterThread() {
    Fixture f;
    f.thread_count = 7u;
    f.runtime.initialize_on_sleep = 2u;
    f.runtime.game_ready_on_sleep = 4u;
    Check(f.coordinator.RunCounterThread(3u, f.runtime) == 0u &&
              f.thread_count == 27u,
          "counter thread increments four stack-derived counters");
    Check(f.runtime.sleeps ==
              std::vector<std::uint32_t>{1000u, 1000u, 1000u, 1000u},
          "counter thread waits for both gates");
    f.runtime.sleeps.clear();
    Check(f.coordinator.RunCounterThread(0xffffffffu, f.runtime) == 0u &&
              f.thread_count == 23u && f.runtime.sleeps.empty(),
          "counter thread wraps at 32 bits and skips ready gates");
}

void TestRandomizedThreadDelay() {
    Fixture f;
    f.runtime.random_value = 0x80001u;
    f.runtime.initialize_on_sleep = 1u;
    f.runtime.game_ready_on_sleep = 2u;
    Check(f.coordinator.RunRandomizedThread(0x3000u, 0x10000u, 3u,
                                             f.runtime) == 0u,
          "randomized thread returns zero");
    Check(f.runtime.seed == 0x3064u &&
              f.runtime.parameter_value == 0x10001u &&
              f.thread_count == 1u,
          "randomized thread seeds and publishes stack pointer");
    Check(f.runtime.sleeps ==
              std::vector<std::uint32_t>{1000u, 1000u, 13000u},
          "randomized thread applies observed 8-to-15 second delay");
    Check(f.runtime.events ==
              std::vector<std::string>{"time", "seed", "random", "write",
                                       "sleep", "sleep", "read", "sleep"},
          "randomized thread call order");
}

void TestRandomizedThreadClearedParameter() {
    Fixture f;
    f.initialized = 1u;
    f.game.Write(0x28u, 1u);
    f.runtime.random_value = 0x7ffffu;
    f.runtime.clear_parameter_before_read = true;
    Check(f.coordinator.RunRandomizedThread(0x3000u, 0x90000u, 0u,
                                             f.runtime) == 0u &&
              f.runtime.parameter_value == 0x10ffffu &&
              f.runtime.sleeps.empty() && f.game.Read(0x28u) == 1u,
          "cleared parameter is republished without delay");
    Check(f.runtime.events ==
              std::vector<std::string>{"time", "seed", "random", "write",
                                       "read", "write"},
          "cleared parameter branch ordering");
}

class ConcurrentRuntime final : public pal4::IThreadStartupRuntime {
public:
    explicit ConcurrentRuntime(std::uint32_t& parameter)
        : parameter_(parameter) {}

    void Sleep(std::uint32_t) override {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    std::uint32_t Time() override { return 100u; }
    void SeedRandom(std::uint32_t) override {}
    std::uint32_t Random() override { return 5u; }
    void WriteParameter(std::uint32_t address,
                        std::uint32_t value) override {
        Check(address == 0x3000u, "concurrent parameter address");
        std::atomic_ref(parameter_).store(value, std::memory_order_release);
    }
    std::uint32_t ReadParameter(std::uint32_t address) override {
        Check(address == 0x3000u, "concurrent parameter read address");
        return std::atomic_ref(parameter_).load(std::memory_order_acquire);
    }

private:
    std::uint32_t& parameter_;
};

void TestConcurrentStartupGates() {
    Fixture f;
    std::uint32_t parameter = 0u;
    ConcurrentRuntime runtime(parameter);
    std::thread worker([&] {
        Check(f.coordinator.RunRandomizedThread(0x3000u, 0x10000u, 2u,
                                                runtime) == 0u,
              "concurrent worker returns zero");
    });
    const auto deadline = std::chrono::steady_clock::now() +
                          std::chrono::seconds(5);
    while ((std::atomic_ref(parameter).load(std::memory_order_acquire) == 0u ||
            std::atomic_ref(f.thread_count).load(std::memory_order_relaxed) == 0u) &&
           std::chrono::steady_clock::now() < deadline)
        std::this_thread::yield();
    const bool published =
        std::atomic_ref(parameter).load(std::memory_order_acquire) == 0x10005u &&
        std::atomic_ref(f.thread_count).load(std::memory_order_relaxed) == 1u;
    std::atomic_ref(f.initialized).store(1u, std::memory_order_release);
    f.game.WriteShared(0x28u, 1u);
    worker.join();
    Check(published, "worker publishes parameter before startup gates open");
}

#if defined(_WIN32) && UINTPTR_MAX == UINT32_MAX
void TestLiveStackPublication() {
    Fixture f;
    f.initialized = 1u;
    f.game.Write(0x28u, 1u);
    f.runtime.use_live_parameter = true;
    f.runtime.random_value = 0x12345u;
    std::uint32_t parameter = 0u;
    f.runtime.parameter_address = static_cast<std::uint32_t>(
        reinterpret_cast<std::uintptr_t>(&parameter));
    Check(f.coordinator.RunRandomizedThreadOnLiveStack(
              parameter, 3u, f.runtime) == 0u &&
              parameter == f.runtime.parameter_value &&
              f.runtime.live_stack_observed &&
              f.runtime.sleeps == std::vector<std::uint32_t>{13000u},
          "native entry publishes a live stack address through the parameter");
}
#endif

}  // namespace

int main() {
    TestCounterThread();
    TestRandomizedThreadDelay();
    TestRandomizedThreadClearedParameter();
    TestConcurrentStartupGates();
#if defined(_WIN32) && UINTPTR_MAX == UINT32_MAX
    TestLiveStackPublication();
#endif
    return EXIT_SUCCESS;
}
