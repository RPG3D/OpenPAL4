#include "pal4/process_module_enumerator.h"

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

class Runtime final : public pal4::IProcessModuleRuntime {
public:
    std::uint32_t InitializeToolHelp32() override {
        events.emplace_back("initialize");
        return initialize_result;
    }
    std::uint32_t CreateSnapshot(std::uint32_t flags,
                                 std::uint32_t process_id) override {
        Check(flags == 8u && process_id == 0x55u,
              "snapshot uses module flag and supplied PID");
        events.emplace_back("snapshot");
        return snapshot;
    }
    std::uint32_t Module32First(std::uint32_t handle,
                                pal4::ModuleEntry32& entry) override {
        Check(handle == snapshot && entry.size == 548u &&
                  entry.base_address == 0u && entry.module_name[0] == 0,
              "first call receives zeroed 548-byte entry");
        events.emplace_back("first");
        if (first_result == 1u && !modules.empty()) {
            entry.base_address = modules[0];
            index = 1u;
        }
        return first_result;
    }
    std::uint32_t Module32Next(std::uint32_t handle,
                               pal4::ModuleEntry32& entry) override {
        Check(handle == snapshot, "next uses snapshot handle");
        events.emplace_back("next");
        if (index >= modules.size()) return next_end_result;
        entry.base_address = modules[index++];
        return 1u;
    }
    void WriteModule(std::uint32_t address,
                     std::uint32_t base_address) override {
        events.emplace_back("write");
        addresses.push_back(address);
        written.push_back(base_address);
    }
    void CloseHandle(std::uint32_t handle) override {
        Check(handle == snapshot, "close uses snapshot handle");
        events.emplace_back("close");
    }
    std::uint32_t GetLastError() override {
        events.emplace_back("get-error");
        return 0x123u;
    }
    void SetLastErrorEx(std::uint32_t error, std::uint32_t type) override {
        Check(type == 1u, "extended error type is one");
        events.emplace_back("error:" + std::to_string(error));
    }
    void SetLastError(std::uint32_t error) override {
        Check(error == 0u, "success clears last error");
        events.emplace_back("clear-error");
    }

    std::vector<std::string> events;
    std::vector<std::uint32_t> modules{0x1000u, 0x2000u, 0x3000u};
    std::vector<std::uint32_t> addresses;
    std::vector<std::uint32_t> written;
    std::uint32_t initialize_result = 1u;
    std::uint32_t snapshot = 0x77u;
    std::uint32_t first_result = 1u;
    std::uint32_t next_end_result = 0u;
    std::size_t index = 0u;
};

std::uint32_t Run(Runtime& runtime, std::uint32_t capacity,
                  std::uint32_t& count) {
    return pal4::ProcessModuleEnumerator::Enumerate(
        0x55u, capacity, 0x8000u, count, runtime);
}

void CheckEarlyFailures() {
    std::uint32_t count = 99u;
    Runtime initialize;
    initialize.initialize_result = 0u;
    Check(Run(initialize, 2u, count) == 0u && count == 0u &&
              initialize.events == std::vector<std::string>{
                  "initialize", "error:1114"},
          "initialization failure clears count and sets 0x45a");

    Runtime snapshot;
    snapshot.snapshot = 0xffffffffu;
    count = 99u;
    Check(Run(snapshot, 2u, count) == 0u && count == 0u &&
              snapshot.events == std::vector<std::string>{
                  "initialize", "snapshot"},
          "invalid snapshot returns without closing or changing error");

    Runtime first;
    first.first_result = 2u;
    count = 99u;
    Check(Run(first, 2u, count) == 0u && count == 0u &&
              first.events == std::vector<std::string>{
                  "initialize", "snapshot", "first", "get-error", "error:291"},
          "first-result other than one forwards last error without closing snapshot");
}

void CheckCapacityAndCounting() {
    std::uint32_t count = 0u;
    Runtime all;
    Check(Run(all, 0u, count) == 1u && count == 3u &&
              all.written.empty() && all.events.back() == "clear-error",
          "zero capacity counts modules without writing array values");

    Runtime short_buffer;
    count = 0u;
    Check(Run(short_buffer, 2u, count) == 0u && count == 2u &&
              short_buffer.written == std::vector<std::uint32_t>{
                  0x1000u, 0x2000u} &&
              short_buffer.addresses == std::vector<std::uint32_t>{
                  0x8000u, 0x8004u} &&
              short_buffer.events[short_buffer.events.size() - 2u] == "close" &&
              short_buffer.events.back() == "error:122",
          "next module beyond capacity closes snapshot and sets 0x7a");

    Runtime exact;
    exact.modules.resize(2u);
    count = 0u;
    Check(Run(exact, 2u, count) == 1u && count == 2u &&
              exact.written.size() == 2u &&
              exact.events.back() == "clear-error",
          "exact module count fits capacity and succeeds");
}

void CheckNextResultMustEqualOne() {
    Runtime runtime;
    runtime.modules.resize(1u);
    runtime.next_end_result = 2u;
    std::uint32_t count = 0u;
    Check(Run(runtime, 4u, count) == 1u && count == 1u &&
              runtime.events.back() == "clear-error",
          "next-result other than one ends enumeration successfully");
}

}  // namespace

int main() {
    CheckEarlyFailures();
    CheckCapacityAndCounting();
    CheckNextResultMustEqualOne();
    std::cout << "3 process module enumerator test cases passed\n";
}
