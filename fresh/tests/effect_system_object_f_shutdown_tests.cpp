#include "pal4/effect_system_object_f_shutdown.h"

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

struct Runtime final : pal4::IEffectSystemObjectFShutdownRuntime {
    std::vector<std::string> events;
    std::uint32_t freed = 0u;

    void ClearComponent(pal4::EffectSystemObjectFState&,
                        std::uint32_t offset) override {
        events.push_back("component:" + std::to_string(offset));
    }
    void CleanupXml(pal4::EffectSystemObjectFState& state,
                    std::uint32_t offset) override {
        events.push_back("xml:" + std::to_string(offset));
        Check(state.words[1u] == 0x0000cc00u,
              "active and adjacent state bytes clear before XML");
    }
    void DeinitializeObject(std::uint32_t object) override {
        events.push_back("deinitialize:" + std::to_string(object));
    }
    void FreeObject(std::uint32_t object) override {
        events.push_back("free:" + std::to_string(object));
        freed = object;
    }
    void ProcessEvent(std::uint32_t system,
                      pal4::EffectSystemObjectFState& state) override {
        events.push_back("event:" + std::to_string(system));
        Check(state.words[29u] == 0u,
              "embedded pointer clears before event dispatch");
    }
    std::int32_t FinalizeObject(pal4::EffectSystemObjectFState&,
                                std::uint32_t argument) override {
        events.push_back("finalize:" + std::to_string(argument));
        return -17;
    }
};

void TestActiveShutdown() {
    Runtime runtime;
    pal4::EffectSystemObjectFState state{};
    state.words[1u] = 0xaabbccddu;
    state.words[4u] = 0x4000u;
    state.words[29u] = 0x5000u;
    Check(pal4::EffectSystemObjectFShutdown::ShutdownAndFinalize(
              state, runtime) == -17,
          "returns final virtual result");
    Check(runtime.events == std::vector<std::string>{
              "component:48", "component:20", "xml:56",
              "deinitialize:20480", "free:20480", "event:16384",
              "finalize:1"} && runtime.freed == 0x5000u &&
              state.words[1u] == 0x0000cc00u && state.words[29u] == 0u,
          "shutdown follows native component, XML, object, event order");
}

void TestNullObjectAndInactive() {
    Runtime runtime;
    pal4::EffectSystemObjectFState state{};
    state.words[1u] = 0x1122cc01u;
    state.words[4u] = 9u;
    Check(pal4::EffectSystemObjectFShutdown::ShutdownAndFinalize(
              state, runtime) == -17 &&
              runtime.events == std::vector<std::string>{
                  "component:48", "component:20", "xml:56",
                  "event:9", "finalize:1"},
          "null embedded object skips deinitialize and free");
    runtime.events.clear();
    Check(pal4::EffectSystemObjectFShutdown::ShutdownAndFinalize(
              state, runtime) == -17 &&
              runtime.events == std::vector<std::string>{"finalize:1"},
          "inactive object still calls final virtual slot");
}

}  // namespace

int main() {
    TestActiveShutdown();
    TestNullObjectAndInactive();
}
