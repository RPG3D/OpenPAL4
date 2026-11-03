#include "pal4/remote_function_executor.h"

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

class Runtime final : public pal4::IRemoteFunctionRuntime {
public:
    std::uint32_t LoadDebugFunctions() override {
        events.emplace_back("load");
        return load_result;
    }
    std::uint32_t OpenProcess(std::uint32_t access, std::uint32_t inherit,
                              std::uint32_t process_id) override {
        Check(access == 0x410u && inherit == 0u && process_id == 0x99u,
              "OpenProcess receives exact access and PID");
        events.emplace_back("open");
        return process;
    }
    std::uint32_t IsBadCodePointer() override {
        events.emplace_back("validate-code");
        return bad_code;
    }
    std::uint32_t CallDebugFunction(std::uint32_t handle,
                                    std::uint32_t modules,
                                    std::uint32_t bytes,
                                    std::uint32_t& bytes_written) override {
        Check(handle == process && modules == 0x2000u &&
                  bytes == expected_bytes && bytes_written == 0u,
              "dynamic debug function receives handle, array, byte count and zero output");
        events.emplace_back("call");
        bytes_written = supplied_bytes;
        return call_result;
    }
    void CloseHandle(std::uint32_t handle) override {
        Check(handle == process, "close uses opened process handle");
        events.emplace_back("close");
    }
    void WriteOutput(std::uint32_t address, std::uint32_t count) override {
        Check(address == 0x3000u, "write uses caller output pointer");
        events.emplace_back("write");
        written_count = count;
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
    std::uint32_t load_result = 1u;
    std::uint32_t process = 0x4444u;
    std::uint32_t bad_code = 0u;
    std::uint32_t call_result = 1u;
    std::uint32_t supplied_bytes = 8u;
    std::uint32_t expected_bytes = 12u;
    std::uint32_t written_count = 0xffffffffu;
};

std::uint32_t Run(Runtime& runtime, std::uint32_t count = 3u) {
    runtime.expected_bytes = count * 4u;
    return pal4::RemoteFunctionExecutor::Execute(
        0x99u, count, 0x2000u, 0x3000u, runtime);
}

void CheckEarlyFailures() {
    Runtime load;
    load.load_result = 0u;
    Check(Run(load) == 0u &&
              load.events == std::vector<std::string>{"load", "error:1114"},
          "missing debug functions set error 0x45a");
    Runtime open;
    open.process = 0u;
    Check(Run(open) == 0u &&
              open.events == std::vector<std::string>{"load", "open"},
          "failed process open returns without changing the error");
    Runtime code;
    code.bad_code = 1u;
    Check(Run(code) == 0u &&
              code.events == std::vector<std::string>{
                  "load", "open", "validate-code", "close", "error:1154"},
          "exactly invalid callback closes process and sets error 0x482");
}

void CheckCallResults() {
    Runtime success;
    Check(Run(success) == 1u && success.written_count == 2u &&
              success.events == std::vector<std::string>{
                  "load", "open", "validate-code", "call", "close",
                  "write", "clear-error"},
          "successful call closes, writes module count, and clears error");

    Runtime failed;
    failed.call_result = 0u;
    Check(Run(failed) == 0u && failed.written_count == 2u &&
              failed.events.back() == "write",
          "failed callback with nonzero requested count still writes output");
    Runtime empty;
    empty.supplied_bytes = 0u;
    Check(Run(empty) == 0u && empty.written_count == 0u &&
              empty.events.back() == "write",
          "zero byte count fails even when callback succeeded");

    Runtime over;
    over.supplied_bytes = 16u;
    Check(Run(over) == 0u && over.written_count == 4u &&
              over.events[5] == "write" &&
              over.events[6] == "error:122",
          "overflow is reported after module count is written");
}

void CheckZeroAndWrappedCount() {
    Runtime zero;
    zero.call_result = 0u;
    zero.supplied_bytes = 4u;
    Check(Run(zero, 0u) == 1u && zero.written_count == 1u &&
              zero.events.back() == "clear-error",
          "zero requested count permits a false callback with nonzero output");

    Runtime wrapped;
    wrapped.bad_code = 2u;
    wrapped.supplied_bytes = 4u;
    Check(Run(wrapped, 0x40000001u) == 1u &&
              wrapped.expected_bytes == 4u,
          "non-one code validation is accepted and byte count wraps at 32 bits");
}

}  // namespace

int main() {
    CheckEarlyFailures();
    CheckCallResults();
    CheckZeroAndWrappedCount();
    std::cout << "3 remote function executor test cases passed\n";
}
