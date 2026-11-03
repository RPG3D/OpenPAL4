#include "pal4/event_property_wrapper.h"

#include <cstdlib>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IEventPropertyWrapperRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;
    bool throw_on_destroy{};

    std::uint32_t ReadWord(const std::uint32_t address) override {
        calls.push_back("read:" + std::to_string(address));
        return words.at(address);
    }
    std::uint32_t Invoke0(const std::uint32_t object,
                          const std::uint32_t slot) override {
        calls.push_back("call0:" + std::to_string(object) + ":" +
                        std::to_string(slot));
        return 0x1234u + slot;
    }
    std::uint32_t Invoke1(const std::uint32_t object,
                          const std::uint32_t slot,
                          const std::uint32_t first) override {
        calls.push_back("call1:" + std::to_string(object) + ":" +
                        std::to_string(slot) + ":" +
                        std::to_string(first));
        return 0x55u;
    }
    std::uint32_t Invoke2(const std::uint32_t object,
                          const std::uint32_t slot,
                          const std::uint32_t first,
                          const std::uint32_t second) override {
        calls.push_back("call2:" + std::to_string(object) + ":" +
                        std::to_string(slot) + ":" +
                        std::to_string(first) + ":" +
                        std::to_string(second));
        return 0xdeadu;
    }
    void WriteWord(const std::uint32_t address,
                   const std::uint32_t value) override {
        calls.push_back("write:" + std::to_string(address) + ":" +
                        std::to_string(value));
        words[address] = value;
    }
    void DestroyChild(const std::uint32_t child,
                      const std::uint32_t flag) override {
        calls.push_back("destroy:" + std::to_string(child) + ":" +
                        std::to_string(flag));
        if (throw_on_destroy) throw std::runtime_error("child");
    }
    void DecrementTempOff() override { calls.push_back("dec_tmpoff"); }
    void Free(const std::uint32_t address) override {
        calls.push_back("free:" + std::to_string(address));
    }
};

void TestForwarding() {
    Runtime runtime;
    runtime.words[0x5004u] = 0x6000u;
    runtime.words[0x5008u] = 0x7000u;
    Check(pal4::EventPropertyWrapper::ForwardSet(
              0x5000u, 0xabu, 0xcdu, runtime) == 0xdeadu &&
              runtime.calls == std::vector<std::string>{
                  "read:20484", "call1:24576:0:171", "read:20488",
                  "call2:28672:4:85:205"},
          "setter translates key through parent before child slot four");
    runtime.calls.clear();
    Check(pal4::EventPropertyWrapper::Call8(0x5000u, runtime) ==
              0x123cu &&
              runtime.calls == std::vector<std::string>{
                  "read:20488", "call0:28672:8"},
          "first query delegates to child slot eight");
    runtime.calls.clear();
    Check(pal4::EventPropertyWrapper::Call12(0x5000u, runtime) ==
              0x1240u &&
              runtime.calls == std::vector<std::string>{
                  "read:20488", "call0:28672:12"},
          "second query delegates to child slot twelve");
}

void TestCleanup() {
    Runtime runtime;
    runtime.words[0x5008u] = 0x7000u;
    Check(pal4::EventPropertyWrapper::Destroy(
              0x5000u, 2u, runtime) == 0x5000u &&
              runtime.words.at(0x5000u) == 0x849898u &&
              runtime.calls == std::vector<std::string>{
                  "write:20480:8689800", "read:20488",
                  "destroy:28672:1", "write:20480:8689816",
                  "dec_tmpoff"},
          "nondeleting destructor releases child then base state");
    runtime.calls.clear();
    Check(pal4::EventPropertyWrapper::Destroy(
              0x5000u, 1u, runtime) == 0x5000u &&
              runtime.calls.back() == "free:20480",
          "low deletion flag frees wrapper after child cleanup");
    runtime.words[0x5008u] = 0u;
    runtime.calls.clear();
    pal4::EventPropertyWrapper::Cleanup(0x5000u, runtime);
    Check(runtime.calls == std::vector<std::string>{
              "write:20480:8689800", "read:20488",
              "write:20480:8689816", "dec_tmpoff"},
          "null child skips virtual destructor but decrements temp count");

    runtime.words[0x5008u] = 0x7000u;
    runtime.throw_on_destroy = true;
    runtime.calls.clear();
    bool caught = false;
    try {
        pal4::EventPropertyWrapper::Cleanup(0x5000u, runtime);
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught && runtime.words.at(0x5000u) == 0x849898u &&
              runtime.calls.back() == "dec_tmpoff" &&
              runtime.calls[runtime.calls.size() - 2u] ==
                  "write:20480:8689816",
          "exceptional child deletion still runs base cleanup");

    pal4::EventPropertyWrapperRuntimeStub stub;
    Check(stub.ReadWord(0u) == 0u &&
              stub.Invoke0(0u, 0u) == 0u &&
              stub.Invoke1(0u, 0u, 0u) == 0u &&
              stub.Invoke2(0u, 0u, 0u, 0u) == 0u,
          "native wrapper boundary has a linkable stub");
}

}  // namespace

int main() {
    TestForwarding();
    TestCleanup();
}
