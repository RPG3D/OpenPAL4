#include "pal4/behavior_path_endpoint_helper.h"

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct GlobalsRuntime final : pal4::IBehaviorPathGlobalsRuntime {
    pal4::BehaviorPathGlobalsState* state = nullptr;
    std::vector<std::string> calls;
    int registration_result = -17;
    bool throw_free = false;

    int RegisterCleanup() override {
        calls.emplace_back("register");
        Check(state->begin == 0u && state->end == 0u &&
                  state->capacity == 0u &&
                  state->high_ecx_byte == 0x12u,
              "global initialization precedes CRT registration");
        return registration_result;
    }
    void Free(std::uint32_t address) override {
        calls.emplace_back("free");
        Check(state->cleanup_guard == 0x81u && address == state->begin,
              "guard bit is set before pointer release");
        if (throw_free) throw std::runtime_error("free");
    }
};

struct EndpointRuntime final : pal4::IBehaviorPathEndpointRuntime {
    pal4::BehaviorPathEndpointState* state = nullptr;
    std::vector<std::string> calls;
    std::vector<std::uint32_t> reads;
    std::unordered_map<std::uint32_t, std::uint32_t> memory;
    std::uint32_t activation_result = 0xfeedu;

    void ConstructBase(pal4::BehaviorPathEndpointState& target) override {
        calls.emplace_back("base_construct");
        Check(target.words[0] == 0xaaaaaaaau &&
                  target.words[47] == 0xaaaaaaaau,
              "base constructor precedes endpoint writes");
        target.words[1] = 0x12345678u;
    }
    void DestructBase(pal4::BehaviorPathEndpointState& target) override {
        calls.emplace_back("base_destruct");
        Check(target.words[0] ==
                  pal4::BehaviorPathEndpointHelper::kVtable &&
                  target.words[47] == 0u,
              "base cleanup sees class vtable and cleared path pointer");
    }
    void Free(std::uint32_t address) override {
        calls.emplace_back("free");
        Check(address == 0x2000u, "deleting destructor frees this address");
    }
    std::uint32_t Read32(std::uint32_t address) override {
        reads.push_back(address);
        const auto it = memory.find(address);
        Check(it != memory.end(), "path read targets mapped input word");
        return it->second;
    }
    void ApplyPathParameter(pal4::BehaviorPathEndpointState& target,
                            std::uint32_t parameter) override {
        calls.emplace_back("apply");
        Check(&target == state && parameter == 0x77u,
              "virtual slot +0xb0 receives input parameter");
    }
    std::uint32_t Activate(
        pal4::BehaviorPathEndpointState& target) override {
        calls.emplace_back("activate");
        Check(&target == state,
              "virtual slot +0xa0 receives endpoint helper");
        return activation_result;
    }
};

void TestGlobals() {
    pal4::BehaviorPathGlobalsState state;
    GlobalsRuntime runtime;
    runtime.state = &state;
    state.cleanup_guard = 0x80u;
    state.begin = 0x3000u;
    state.end = 0x3004u;
    state.capacity = 0x3010u;
    Check(pal4::BehaviorPathEndpointGlobals::RegisterExit(
              state, 0x12345678u, runtime) == -17 &&
              state.cleanup_guard == 0x80u &&
              runtime.calls == std::vector<std::string>{"register"},
          "register exit initializes three pointers and forwards CRT result");
    Check(pal4::BehaviorPathEndpointGlobals::kGuardAddress == 0x8e41e4u &&
              pal4::BehaviorPathEndpointGlobals::kHighByteAddress ==
                  0x8e41e8u &&
              pal4::BehaviorPathEndpointGlobals::kBeginAddress == 0x8e41ecu,
          "global storage addresses match IDA");

    state.begin = 0x3000u;
    state.end = 0x3004u;
    state.capacity = 0x3010u;
    runtime.calls.clear();
    pal4::BehaviorPathEndpointGlobals::Cleanup(state, runtime);
    Check(state.cleanup_guard == 0x81u && state.begin == 0u &&
              state.end == 0u && state.capacity == 0u &&
              state.high_ecx_byte == 0x12u &&
              runtime.calls == std::vector<std::string>{"free"},
          "cleanup marks guard, frees begin and clears pointer triple");
    pal4::BehaviorPathEndpointGlobals::Cleanup(state, runtime);
    Check(runtime.calls == std::vector<std::string>{"free"},
          "guard suppresses a second cleanup");

    state.cleanup_guard = 0x80u;
    state.begin = 0x3000u;
    state.end = 0x3004u;
    state.capacity = 0x3010u;
    runtime.throw_free = true;
    bool threw = false;
    try { pal4::BehaviorPathEndpointGlobals::Cleanup(state, runtime); }
    catch (const std::runtime_error&) { threw = true; }
    Check(threw && state.cleanup_guard == 0x81u &&
              state.begin == 0x3000u && state.end == 0x3004u,
          "release exception leaves fields untouched after guard write");
    pal4::BehaviorPathGlobalsRuntimeStub stub;
    Check(pal4::BehaviorPathEndpointGlobals::RegisterExit(
              state, 0u, stub) == 0,
          "global registration links without original CRT");
}

void TestConstructionAndDeletion() {
    pal4::BehaviorPathEndpointState state;
    EndpointRuntime runtime;
    runtime.state = &state;
    state.words.fill(0xaaaaaaaau);
    Check(&pal4::BehaviorPathEndpointHelper::Construct(state, runtime) ==
                  &state &&
              runtime.calls == std::vector<std::string>{"base_construct"} &&
              state.words[0] ==
                  pal4::BehaviorPathEndpointHelper::kVtable &&
              state.words[1] == 0x12345678u &&
              state.words[47] == 0u &&
              state.words[49] == 0xaaaa0000u &&
              state.words[48] == 0xaaaaaaaau &&
              state.words[50] == 0u && state.words[51] == 0u &&
              state.words[52] == 0u && state.words[53] == 0u &&
              state.words[54] == 0u && state.words[55] == 0u,
          "constructor clears only confirmed fields and two flag bytes");
    runtime.calls.clear();
    state.words[47] = 0x4000u;
    Check(pal4::BehaviorPathEndpointHelper::Delete(
              state, 0x2000u, 0u, runtime) == 0x2000u &&
              runtime.calls == std::vector<std::string>{"base_destruct"} &&
              state.words[47] == 0u,
          "nondeleting destruction clears path before base cleanup");
    runtime.calls.clear();
    state.words[47] = 0x4000u;
    Check(pal4::BehaviorPathEndpointHelper::Delete(
              state, 0x2000u, 3u, runtime) == 0x2000u &&
              runtime.calls == std::vector<std::string>{
                  "base_destruct", "free"},
          "deleting destructor frees after base cleanup");
    pal4::BehaviorPathEndpointRuntimeStub stub;
    pal4::BehaviorPathEndpointState empty;
    static_cast<void>(pal4::BehaviorPathEndpointHelper::Construct(empty, stub));
    pal4::BehaviorPathEndpointHelper::Destruct(empty, stub);
}

void TestPathCaching() {
    pal4::BehaviorPathEndpointState state;
    EndpointRuntime runtime;
    runtime.state = &state;
    state.words[50] = 0xdeadu;
    state.words[53] = 0xbeefu;
    Check(pal4::BehaviorPathEndpointHelper::SetPathAndCacheEndpoints(
              state, 0x77u, 0u, runtime) == 0xfeedu &&
              state.words[47] == 0u && state.words[50] == 0xdeadu &&
              state.words[53] == 0xbeefu && runtime.reads.empty() &&
              runtime.calls == std::vector<std::string>{
                  "apply", "activate"},
          "null path preserves cached endpoints but still invokes virtuals");

    runtime.calls.clear();
    runtime.memory = {
        {0x400cu, 0x3000u}, {0x4008u, 3u},
        {0x3000u, 10u}, {0x3004u, 11u}, {0x3008u, 12u},
        {0x3018u, 30u}, {0x301cu, 31u}, {0x3020u, 32u}};
    Check(pal4::BehaviorPathEndpointHelper::SetPathAndCacheEndpoints(
              state, 0x77u, 0x4000u, runtime) == 0xfeedu &&
              state.words[47] == 0x4000u &&
              state.words[50] == 10u && state.words[51] == 11u &&
              state.words[52] == 12u && state.words[53] == 30u &&
              state.words[54] == 31u && state.words[55] == 32u &&
              runtime.reads == std::vector<std::uint32_t>{
                  0x400cu, 0x3000u, 0x3004u, 0x3008u,
                  0x4008u, 0x400cu, 0x3018u, 0x301cu, 0x3020u} &&
              runtime.calls == std::vector<std::string>{
                  "apply", "activate"},
          "non-null path caches first and last vec3 in read order");

    runtime.reads.clear();
    runtime.memory[0x4008u] = 0u;
    runtime.memory[0x2ff4u] = 90u;
    runtime.memory[0x2ff8u] = 91u;
    runtime.memory[0x2ffcu] = 92u;
    Check(pal4::BehaviorPathEndpointHelper::SetPathAndCacheEndpoints(
              state, 0x77u, 0x4000u, runtime) == 0xfeedu &&
              state.words[53] == 90u && state.words[54] == 91u &&
              state.words[55] == 92u &&
              runtime.reads[6] == 0x2ff4u,
          "zero point count retains the binary's preceding-element read");
}

}  // namespace

int main() {
    TestGlobals();
    TestConstructionAndDeletion();
    TestPathCaching();
}
