#include "pal4/platform_function_dispatcher.h"

#include <cstddef>
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

class Runtime final : public pal4::IPlatformFunctionRuntime {
public:
    std::uint32_t IsBadWritePointer(std::uint32_t address,
                                    std::uint32_t bytes) override {
        events.emplace_back("validate:" + std::to_string(address) + ":" +
                            std::to_string(bytes));
        const std::uint32_t result = validation_count == 0u
            ? output_validation : module_validation;
        ++validation_count;
        return result;
    }

    void SetLastErrorEx(std::uint32_t error, std::uint32_t type) override {
        Check(error == 0x57u && type == 1u, "invalid pointer sets exact error");
        events.emplace_back("error");
    }

    std::uint32_t GetVersion(pal4::OperatingSystemVersion& version) override {
        events.emplace_back("version");
        Check(version.size == 148u && version.major == 0u &&
                  version.minor == 0u && version.build == 0u &&
                  version.platform == 0u && version.service_pack[0] == 0 &&
                  version.service_pack[127] == 0,
              "version structure is zeroed and reports its 148-byte size");
        version.platform = platform;
        version.major = major;
        return version_result;
    }

    std::uint32_t ExecuteRemoteFunction(std::uint32_t process_id,
                                        std::uint32_t count,
                                        std::uint32_t modules,
                                        std::uint32_t output) override {
        Check(process_id == 7u && count == expected_count &&
                  modules == 0x2000u && output == 0x3000u,
              "remote path forwards all arguments");
        events.emplace_back("remote");
        return 0x1234u;
    }

    std::uint32_t EnumerateProcessModules(std::uint32_t process_id,
                                          std::uint32_t count,
                                          std::uint32_t modules,
                                          std::uint32_t output) override {
        Check(process_id == 7u && count == expected_count &&
                  modules == 0x2000u && output == 0x3000u,
              "enumeration path forwards all arguments");
        events.emplace_back("enumerate");
        return 0xabcd;
    }

    std::vector<std::string> events;
    std::uint32_t output_validation = 0u;
    std::uint32_t module_validation = 0u;
    std::uint32_t version_result = 1u;
    std::uint32_t platform = 2u;
    std::uint32_t major = 4u;
    std::uint32_t expected_count = 3u;
    std::size_t validation_count = 0u;
};

void CheckValidation() {
    Runtime runtime;
    runtime.output_validation = 1u;
    Check(pal4::PlatformFunctionDispatcher::Dispatch(
              7u, 3u, 0x2000u, 0x3000u, runtime) == 0u &&
              runtime.events == std::vector<std::string>{
                  "validate:12288:4", "error"},
          "invalid output pointer stops before module validation");

    Runtime second;
    second.module_validation = 1u;
    Check(pal4::PlatformFunctionDispatcher::Dispatch(
              7u, 3u, 0x2000u, 0x3000u, second) == 0u &&
              second.events == std::vector<std::string>{
                  "validate:12288:4", "validate:8192:12", "error"},
          "invalid module array sets error after both validations");

    Runtime non_one;
    non_one.output_validation = 2u;
    non_one.module_validation = 2u;
    Check(pal4::PlatformFunctionDispatcher::Dispatch(
              7u, 3u, 0x2000u, 0x3000u, non_one) == 0x1234u,
          "only an exact IsBadWritePtr result of one is rejected");
}

void CheckVersionAndDispatch() {
    Runtime runtime;
    runtime.version_result = 0u;
    Check(pal4::PlatformFunctionDispatcher::Dispatch(
              7u, 3u, 0x2000u, 0x3000u, runtime) == 0u &&
              runtime.events.back() == "version",
          "version-query failure returns zero without setting pointer error");

    Runtime legacy;
    Check(pal4::PlatformFunctionDispatcher::Dispatch(
              7u, 3u, 0x2000u, 0x3000u, legacy) == 0x1234u &&
              legacy.events.back() == "remote",
          "NT 4.0 selects remote function path");

    Runtime modern;
    modern.major = 5u;
    Check(pal4::PlatformFunctionDispatcher::Dispatch(
              7u, 3u, 0x2000u, 0x3000u, modern) == 0xabcdu &&
              modern.events.back() == "enumerate",
          "other major version selects module enumeration");
    Runtime other_platform;
    other_platform.platform = 1u;
    Check(pal4::PlatformFunctionDispatcher::Dispatch(
              7u, 3u, 0x2000u, 0x3000u, other_platform) == 0xabcdu,
          "other platform selects module enumeration");
}

void CheckZeroCountAndWrappedSize() {
    Runtime zero;
    zero.expected_count = 0u;
    Check(pal4::PlatformFunctionDispatcher::Dispatch(
              7u, 0u, 0x2000u, 0x3000u, zero) == 0x1234u &&
              zero.validation_count == 1u,
          "zero count skips module-array validation");
    Runtime wrapped;
    wrapped.expected_count = 0x40000001u;
    Check(pal4::PlatformFunctionDispatcher::Dispatch(
              7u, 0x40000001u, 0x2000u, 0x3000u, wrapped) == 0x1234u &&
              wrapped.events[1] == "validate:8192:4",
          "module-array size uses 32-bit multiplication");
}

void CheckPlatformCache() {
    std::uint32_t guard = 0u;
    std::uint32_t is_nt = 9u;
    pal4::OperatingSystemPlatformCache cache(guard, is_nt);
    Runtime failed;
    failed.version_result = 0u;
    Check(cache.IsNt(failed) == 0u && guard == 0u && is_nt == 9u &&
              failed.events == std::vector<std::string>{"version"},
          "version failure leaves cache untouched for retry");

    Runtime nt;
    Check(cache.IsNt(nt) == 1u && guard == 1u && is_nt == 1u &&
              nt.events == std::vector<std::string>{"version"},
          "NT platform result and guard are cached");
    nt.events.clear();
    Check(cache.IsNt(nt) == 1u && nt.events.empty(),
          "warm cache avoids version query");
    is_nt = 7u;
    Check(cache.IsNt(nt) == 0u && nt.events.empty(),
          "warm cache compares stored result to exactly one");

    guard = 2u;
    Runtime other;
    other.platform = 1u;
    Check(cache.IsNt(other) == 0u && guard == 1u && is_nt == 0u &&
              other.events == std::vector<std::string>{"version"},
          "non-one guard re-queries and caches non-NT platform");
}

}  // namespace

int main() {
    CheckValidation();
    CheckVersionAndDispatch();
    CheckZeroCountAndWrappedSize();
    CheckPlatformCache();
    std::cout << "4 platform function dispatcher test cases passed\n";
}
