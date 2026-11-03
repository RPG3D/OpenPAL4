#include "pal4/effect_system_task_object.h"

#include <bit>
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

struct Runtime final : pal4::IGomTaskBaseRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;
    std::uint32_t AllocateMemory(std::uint32_t) override { return 0u; }
    std::uint32_t FreeMemory(std::uint32_t address) override {
        calls.push_back("free:" + std::to_string(address));
        return address + 1u;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.push_back("read:" + std::to_string(address));
        return words.at(address);
    }
    void CopyCString(std::uint32_t, const char*,
                     std::uint32_t) override {}
};

void TestConstructAndCleanup() {
    Runtime runtime;
    pal4::GomTaskBase base(runtime);
    pal4::EffectSystemTaskObject object(base, runtime);
    pal4::EffectSystemTaskObjectState state{};
    state.enabled = 9u;
    state.copy_timing = 7u;
    state.reserved64 = 0xaaaaaaaau;
    state.property68 = 0xbbbbbbbbu;
    Check(&object.Construct(state) == &state &&
              state.base.vtable == pal4::EffectSystemTaskObject::kVtable &&
              state.enabled == 0u && state.copy_timing == 0u &&
              state.reserved64 == 0u && state.property68 == 0u &&
              state.base.name == 0u && state.base.previous == 0u,
          "derived construction composes 60-byte base and clears tail");

    state.enabled = 1u;
    state.copy_timing = 1u;
    state.reserved64 = 0x1234u;
    state.property68 = 0x5678u;
    state.base.name = 0x5000u;
    state.base.previous = 0x6000u;
    Check(object.Cleanup(state) == 0x5001u &&
              state.base.vtable == pal4::GomTaskBase::kVtable &&
              state.enabled == 0u && state.copy_timing == 0u &&
              state.reserved64 == 0u && state.property68 == 0u &&
              state.base.name == 0u && state.base.previous == 0u &&
              runtime.calls == std::vector<std::string>{"free:20480"},
          "enabled cleanup clears tail before base name/link destruction");

    state.enabled = 0u;
    state.copy_timing = 1u;
    state.reserved64 = 0x1111u;
    state.property68 = 0x2222u;
    Check(object.Cleanup(state) == 0u &&
              state.copy_timing == 1u &&
              state.reserved64 == 0x1111u &&
              state.property68 == 0x2222u,
          "disabled cleanup preserves inactive tail fields");
}

void TestDeletingDestructor() {
    Runtime runtime;
    pal4::GomTaskBase base(runtime);
    pal4::EffectSystemTaskObject object(base, runtime);
    pal4::EffectSystemTaskObjectState state{};
    state.base.name = 0x5000u;
    Check(object.Delete(state, 0x7000u, 1u) == 0x7000u &&
              runtime.calls == std::vector<std::string>{
                  "free:20480", "free:28672"},
          "deleting destructor frees base name before object storage");
    runtime.calls.clear();
    Check(object.Delete(state, 0x7000u, 2u) == 0x7000u &&
              runtime.calls.empty(),
          "non-deleting flag leaves object storage owned by caller");
}

void TestSetupState() {
    Runtime runtime;
    pal4::GomTaskBase base(runtime);
    pal4::EffectSystemTaskObject object(base, runtime);
    pal4::EffectSystemTaskObjectState state{};
    state.base.previous = 0x6000u;
    Check(object.SetupState(state) == 0u && runtime.calls.empty(),
          "copy-timing flag gates linked task reads");
    state.copy_timing = 1u;
    state.base.previous = 0u;
    Check(object.SetupState(state) == 0u && runtime.calls.empty(),
          "missing linked task performs no copy");

    state.base.previous = 0x6000u;
    runtime.words[0x600cu] = 3u;
    runtime.words[0x6014u] = std::bit_cast<std::uint32_t>(2.5f);
    runtime.words[0x6010u] = 0x3f8000abu;
    Check(object.SetupState(state) == 0xabu &&
              state.base.wait_mode == 3u &&
              state.base.expect_time == 2.5f &&
              std::bit_cast<std::uint32_t>(state.base.wait_time) ==
                  0x3f8000abu &&
              runtime.calls == std::vector<std::string>{
                  "read:24588", "read:24596", "read:24592"},
          "enabled setup copies linked timing in native read order");
}

}  // namespace

int main() {
    TestConstructAndCleanup();
    TestDeletingDestructor();
    TestSetupState();
}
