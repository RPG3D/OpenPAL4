#include "pal4/standard_stream_lifecycle.h"

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

class Runtime final : public pal4::IStandardStreamRuntime {
public:
    std::vector<std::string> calls;
    std::uint32_t narrow_result = 0x21u;
    std::uint32_t wide_result = 0x22u;
    int registration_result = 37;

    std::uint32_t ConstructNarrow(std::uint32_t address) override {
        Check(address == 0x8e3f31u, "narrow object address");
        calls.emplace_back("construct-narrow");
        return narrow_result;
    }
    std::uint32_t ConstructWide(std::uint32_t address) override {
        Check(address == 0x8e3f30u, "wide object address");
        calls.emplace_back("construct-wide");
        return wide_result;
    }
    int RegisterExit(std::uint32_t address) override {
        calls.emplace_back("register-" + std::to_string(address));
        return registration_result;
    }
    void FlushNarrow(std::uint32_t address) override {
        calls.emplace_back("flush-" + std::to_string(address));
    }
    void CleanupWide(std::uint32_t address) override {
        calls.emplace_back("cleanup-" + std::to_string(address));
    }
};

void TestNarrowLifecycle() {
    std::uint32_t narrow_count = 2u;
    std::uint32_t wide_count = 1u;
    pal4::StandardStreamLifecycle streams(narrow_count, wide_count);
    Runtime runtime;
    Check(streams.ConstructNarrow(runtime) == 0x21u,
          "narrow wrapper forwards constructor result");
    Check(streams.RegisterNarrowExit(runtime) == 37,
          "narrow exit wrapper forwards registration result");
    runtime.calls.clear();
    runtime.registration_result = -2;
    Check(streams.InitializeNarrow(runtime) == -2 &&
              runtime.calls == std::vector<std::string>{
                                   "construct-narrow", "register-4242416"},
          "narrow aggregate constructs before registering destructor");
    runtime.calls.clear();
    streams.CleanupNarrow(runtime);
    Check(narrow_count == 1u && runtime.calls.empty(),
          "narrow cleanup is gated by reference count");
    streams.CleanupNarrow(runtime);
    Check(narrow_count == 0u &&
              runtime.calls == std::vector<std::string>{
                                   "flush-9766520", "flush-9766816",
                                   "flush-9766376"},
          "narrow cleanup flushes three streams in observed order");
}

void TestWideLifecycle() {
    std::uint32_t narrow_count = 0u;
    std::uint32_t wide_count = 2u;
    pal4::StandardStreamLifecycle streams(narrow_count, wide_count);
    Runtime runtime;
    Check(streams.ConstructWide(runtime) == 0x22u &&
              streams.RegisterWideExit(runtime) == 37,
          "wide wrappers forward return values");
    runtime.calls.clear();
    Check(streams.InitializeWide(runtime) == 37 &&
              runtime.calls == std::vector<std::string>{
                                   "construct-wide", "register-4242480"},
          "wide aggregate constructs before registering destructor");
    runtime.calls.clear();
    streams.CleanupWide(runtime);
    Check(wide_count == 1u && runtime.calls.empty(),
          "wide cleanup is gated by reference count");
    streams.CleanupWide(runtime);
    Check(wide_count == 0u &&
              runtime.calls == std::vector<std::string>{
                                   "cleanup-9767232", "cleanup-9766872",
                                   "cleanup-9766936"},
          "wide cleanup visits three stream objects in observed order");
    streams.CleanupWide(runtime);
    Check(wide_count == 0xffffffffu && runtime.calls.size() == 3u,
          "zero count underflows without repeating cleanup");
}

}  // namespace

int main() {
    Check(pal4::StandardStreamLifecycle::ConstructIostreamInit(0x1234u) ==
              0x1234u &&
              pal4::StandardStreamLifecycle::ConstructIostreamInit(0u) == 0u,
          "0x419e40 returns this without mutating it");
    TestNarrowLifecycle();
    TestWideLifecycle();
    return EXIT_SUCCESS;
}
