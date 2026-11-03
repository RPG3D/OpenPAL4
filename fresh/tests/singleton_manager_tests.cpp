#include "pal4/singleton_manager.h"

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

class Runtime : public pal4::ISingletonManagerRuntime {
public:
    void DestroyAll() override { events.emplace_back("destroy-all"); }
    std::uint32_t ReadDword(std::uint32_t address) override {
        events.emplace_back("read:" + std::to_string(address));
        return words.at(address);
    }
    void WriteDword(std::uint32_t address, std::uint32_t value) override {
        events.emplace_back("write:" + std::to_string(address));
        words[address] = value;
    }
    std::uint32_t ResolveReferenceByte(std::uint32_t element,
                                       std::uint32_t value) override {
        Check(value == words.at(element + 4u),
              "reference helper receives element and its value");
        events.emplace_back("resolve:" + std::to_string(element));
        ++resolve_counts[element];
        return element + 20u;
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        events.emplace_back("read-byte");
        return bytes.at(address);
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        events.emplace_back("write-byte");
        bytes[address] = value;
    }
    void DisposeElement(std::uint32_t element,
                        std::uint32_t value_minus_one,
                        std::uint32_t field_plus_two) override {
        events.emplace_back("dispose");
        disposals.push_back({element, value_minus_one, field_plus_two});
    }
    std::uint32_t Free(std::uint32_t address) override {
        events.emplace_back("free:" + std::to_string(address));
        return 13u;
    }

    struct Disposal {
        std::uint32_t element;
        std::uint32_t value_minus_one;
        std::uint32_t field_plus_two;
        bool operator==(const Disposal&) const = default;
    };
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::unordered_map<std::uint32_t, std::uint8_t> bytes;
    std::unordered_map<std::uint32_t, unsigned> resolve_counts;
    std::vector<Disposal> disposals;
    std::vector<std::string> events;
};

class DestroyRuntime final : public pal4::ISingletonDestroyRuntime {
public:
    void SortByDescendingId(std::uint32_t begin,
                            std::uint32_t end) override {
        Check(begin == expected_begin && end == expected_end,
              "sort receives current element range");
        events.emplace_back("sort");
    }
    std::uint32_t ReadDword(std::uint32_t address) override {
        events.emplace_back("read:" + std::to_string(address));
        return words.at(address);
    }
    void LogDestroy(std::uint32_t singleton_id,
                    std::uint32_t name_address,
                    const char* source_file, const char* format,
                    std::uint32_t line) override {
        Check(std::string(source_file) ==
                  "D:\\VcProject\\Pal4\\project\\Sourcecode\\Scl\\scl_Singleton.cpp" &&
                  std::string(format) ==
                  "FILE:  [%s]\nLINE:  [%d]\nINFO:  singleton-destroy: [ %d ], [ %s ]\n\n\n" &&
                  line == 64u,
              "destroy log receives exact source path, format and line");
        events.emplace_back("log");
        logged_ids.push_back(singleton_id);
        logged_names.push_back(name_address);
    }
    void InvokeDestroy(std::uint32_t callback,
                       std::uint32_t argument) override {
        events.emplace_back("callback");
        callbacks.push_back({callback, argument});
    }
    std::uint32_t ReleaseTextRecord(std::uint32_t element) override {
        events.emplace_back("release-text:" + std::to_string(element));
        return element + 1u;
    }

    struct Callback {
        std::uint32_t function;
        std::uint32_t argument;
        bool operator==(const Callback&) const = default;
    };
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> events;
    std::vector<std::uint32_t> logged_ids;
    std::vector<std::uint32_t> logged_names;
    std::vector<Callback> callbacks;
    std::uint32_t expected_begin = 0u;
    std::uint32_t expected_end = 0u;
};

class IntegratedCleanupRuntime final : public Runtime {
public:
    IntegratedCleanupRuntime(pal4::SingletonManager& manager,
                             DestroyRuntime& destroy)
        : manager_(manager), destroy_(destroy) {}

    void DestroyAll() override {
        events.emplace_back("destroy-all");
        (void)manager_.DestroyAll(0x9000u, destroy_);
    }

private:
    pal4::SingletonManager& manager_;
    DestroyRuntime& destroy_;
};

class AccessRuntime final : public pal4::ISingletonManagerAccessRuntime {
public:
    explicit AccessRuntime(const pal4::SingletonManagerGlobalState& state)
        : state_(state) {}

    std::uint32_t Allocate(std::uint32_t bytes) override {
        Check(bytes == 16u && (state_.initialization_bits & 1u) != 0u,
              "guard bit is set before 16-byte allocation");
        events.emplace_back("allocate");
        return allocated;
    }
    std::uint32_t Initialize(std::uint32_t address) override {
        Check(address == allocated, "initializer receives allocated address");
        events.emplace_back("initialize");
        return initialized;
    }
    void RegisterExit(std::uint32_t callback_address) override {
        Check(callback_address ==
                  pal4::SingletonManagerAccess::kExitCallbackAddress &&
                  state_.instance == initialized,
              "exit callback is registered after instance is stored");
        events.emplace_back("register-exit");
    }
    void Cleanup(std::uint32_t address) override {
        Check(address == state_.instance, "cleanup receives cached instance");
        events.emplace_back("cleanup");
    }
    void Free(std::uint32_t address) override {
        Check(address == state_.instance, "free receives cached instance");
        events.emplace_back("free");
    }

    std::vector<std::string> events;
    std::uint32_t allocated = 0x5000u;
    std::uint32_t initialized = 0x5000u;

private:
    const pal4::SingletonManagerGlobalState& state_;
};

void CheckInitialize() {
    pal4::SingletonManagerState state{0xaabbccddu, 1u, 2u, 3u};
    pal4::SingletonManager manager(state);
    Check(manager.Initialize(0x12345678u) == 0x12345678u &&
              state.first_word == 0xaabbcc12u && state.begin == 0u &&
              state.end == 0u && state.capacity == 0u,
          "initialization stores pointer high byte in first byte and clears three words");
}

void CheckEmptyCleanup() {
    pal4::SingletonManagerState state{0x1234u, 0u, 0u, 0u};
    pal4::SingletonManager manager(state);
    Runtime runtime;
    Check(manager.Cleanup(runtime) == 13u &&
              runtime.events == std::vector<std::string>{
                  "destroy-all", "free:0"} && state.first_word == 0x1234u,
          "empty cleanup destroys all then frees null array and preserves first word");
}

void CheckElementBranches() {
    constexpr std::uint32_t begin = 0x1000u;
    pal4::SingletonManagerState state{0x55u, begin, begin + 4u * 28u,
                                      begin + 8u * 28u};
    pal4::SingletonManager manager(state);
    Runtime runtime;
    for (std::uint32_t index = 0u; index < 4u; ++index) {
        const std::uint32_t element = begin + index * 28u;
        runtime.words[element + 4u] = index == 0u ? 0u : index * 2u + 3u;
        runtime.words[element + 8u] = 0x99u;
        runtime.words[element + 12u] = 9u;
    }
    runtime.bytes[begin + 28u + 20u] = 0u;
    runtime.bytes[begin + 56u + 20u] = 5u;
    runtime.bytes[begin + 84u + 20u] = 0xffu;

    Check(manager.Cleanup(runtime) == 13u &&
              runtime.disposals == std::vector<Runtime::Disposal>{
                  {begin + 28u, 4u, 11u},
                  {begin + 84u, 8u, 11u}} &&
              runtime.bytes[begin + 56u + 20u] == 4u &&
              runtime.resolve_counts[begin + 28u] == 1u &&
              runtime.resolve_counts[begin + 56u] == 3u &&
              runtime.resolve_counts[begin + 84u] == 2u,
          "zero, zero-byte, ordinary-byte and ff-byte elements take distinct paths");
    for (std::uint32_t index = 0u; index < 4u; ++index) {
        const std::uint32_t element = begin + index * 28u;
        Check(runtime.words[element + 4u] == 0u &&
                  runtime.words[element + 8u] == 0u &&
                  runtime.words[element + 12u] == 0u,
              "each processed element clears three observed words");
    }
    Check(state.first_word == 0x55u && state.begin == 0u &&
              state.end == 0u && state.capacity == 0u &&
              runtime.events.front() == "destroy-all" &&
              runtime.events.back() == "free:4096",
          "cleanup frees original array and clears manager pointers");
}

void CheckSignedComparator() {
    Runtime runtime;
    runtime.words[0x1010u] = 5u;
    runtime.words[0x2010u] = 4u;
    Check(pal4::SingletonManager::CompareBySingletonId(
              0x1000u, 0x2000u, runtime) == 1u,
          "left singleton ID greater than right returns one");
    runtime.words[0x2010u] = 5u;
    Check(pal4::SingletonManager::CompareBySingletonId(
              0x1000u, 0x2000u, runtime) == 0u,
          "equal IDs return zero");
    runtime.words[0x1010u] = 0xffffffffu;
    runtime.words[0x2010u] = 1u;
    Check(pal4::SingletonManager::CompareBySingletonId(
              0x1000u, 0x2000u, runtime) == 0u,
          "IDs compare as signed 32-bit values");
}

void CheckSingletonAccess() {
    pal4::SingletonManagerGlobalState state{0xa0u, 0u, 0u};
    pal4::SingletonManagerAccess access(state);
    AccessRuntime runtime(state);
    Check(access.GetInstance(runtime) == 0x5000u &&
              state.initialization_bits == 0xa1u &&
              state.owns_instance == 1u && state.instance == 0x5000u &&
              runtime.events == std::vector<std::string>{
                  "allocate", "initialize", "register-exit"},
          "first access allocates, initializes, caches and registers exit");
    runtime.events.clear();
    Check(access.GetInstance(runtime) == 0x5000u && runtime.events.empty(),
          "warm access returns cached instance");
    access.ExitCleanup(runtime);
    Check(runtime.events == std::vector<std::string>{"cleanup", "free"} &&
              state.instance == 0x5000u && state.owns_instance == 1u,
          "exit callback cleans then frees without clearing global state");
}

void CheckFailedSingletonAccess() {
    pal4::SingletonManagerGlobalState state;
    pal4::SingletonManagerAccess access(state);
    AccessRuntime runtime(state);
    runtime.allocated = 0u;
    runtime.initialized = 0u;
    Check(access.GetInstance(runtime) == 0u &&
              state.initialization_bits == 1u &&
              state.owns_instance == 0u && state.instance == 0u &&
              runtime.events == std::vector<std::string>{
                  "allocate", "register-exit"},
          "allocation failure still caches zero and registers exit");
    runtime.events.clear();
    access.ExitCleanup(runtime);
    Check(runtime.events.empty(), "unowned null instance skips exit cleanup");
    state.owns_instance = 1u;
    access.ExitCleanup(runtime);
    Check(runtime.events.empty(), "owned flag alone does not free null instance");
}

void CheckDestroyAll() {
    pal4::SingletonManagerState empty;
    pal4::SingletonManager empty_manager(empty);
    DestroyRuntime runtime;
    Check(empty_manager.DestroyAll(0x9000u, runtime) == 0x9000u &&
              runtime.events.empty(),
          "null begin returns the singleton instance without work");

    empty.begin = 0x1000u;
    empty.end = 0x1000u;
    Check(empty_manager.DestroyAll(0x9000u, runtime) == 0u &&
              runtime.events.empty(),
          "non-null empty range stops before sorting");

    pal4::SingletonManagerState state{0u, 0x1000u, 0x1038u, 0x1100u};
    pal4::SingletonManager manager(state);
    runtime.expected_begin = state.begin;
    runtime.expected_end = state.end;
    runtime.words[0x1004u] = 0u;
    runtime.words[0x1010u] = 9u;
    runtime.words[0x1014u] = 0xaaaa;
    runtime.words[0x1018u] = 0xbbbb;
    runtime.words[0x1020u] = 0x4444u;
    runtime.words[0x102cu] = 3u;
    runtime.words[0x1030u] = 0xcccc;
    runtime.words[0x1034u] = 0xdddd;
    Check(manager.DestroyAll(0x9000u, runtime) == 0x101du &&
              runtime.logged_ids == std::vector<std::uint32_t>{9u, 3u} &&
              runtime.logged_names == std::vector<std::uint32_t>{
                  pal4::SingletonManager::kEmptyLogMessageAddress, 0x4444u} &&
              runtime.callbacks == std::vector<DestroyRuntime::Callback>{
                  {0xbbbbu, 0xaaaau}, {0xddddu, 0xccccu}} &&
              runtime.events.front() == "sort" &&
              runtime.events[runtime.events.size() - 2u] == "release-text:4096" &&
              runtime.events.back() == "release-text:4124" &&
              state.begin == 0x1000u && state.end == 0x1000u &&
              state.capacity == 0x1100u,
          "destroy all logs and invokes each element then removes stubs and resets end");
}

void CheckDestroyCleanupComposition() {
    pal4::SingletonManagerState state{0u, 0x1000u, 0x101cu, 0x1100u};
    pal4::SingletonManager manager(state);
    DestroyRuntime destroy;
    destroy.expected_begin = state.begin;
    destroy.expected_end = state.end;
    destroy.words[0x1004u] = 0x2000u;
    destroy.words[0x1010u] = 1u;
    destroy.words[0x1014u] = 0x3000u;
    destroy.words[0x1018u] = 0x4000u;
    IntegratedCleanupRuntime cleanup(manager, destroy);
    Check(manager.Cleanup(cleanup) == 13u &&
              destroy.events.back() == "release-text:4096" &&
              cleanup.events == std::vector<std::string>{
                  "destroy-all", "free:4096"} &&
              state.begin == 0u && state.end == 0u,
          "cleanup observes DestroyAll's end reset and then frees the original array");
}

}  // namespace

int main() {
    CheckInitialize();
    CheckEmptyCleanup();
    CheckElementBranches();
    CheckSignedComparator();
    CheckSingletonAccess();
    CheckFailedSingletonAccess();
    CheckDestroyAll();
    CheckDestroyCleanupComposition();
    std::cout << "8 singleton manager test cases passed\n";
}
