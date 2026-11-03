#include "pal4/effect_system_global_callback.h"

#include <cstdlib>
#include <iostream>

namespace {

struct Runtime final : pal4::IEffectSystemGlobalCallbackRuntime {
    std::uint32_t called = 0u;
    std::uint32_t result = 0x12345678u;
    std::uint32_t Invoke(const std::uint32_t address) override {
        called = address;
        return result;
    }
};

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

}  // namespace

int main() {
    Runtime runtime;
    std::uint32_t slot = 0u;
    Check(pal4::EffectSystemGlobalCallback::Call(slot, runtime) == 0u &&
              runtime.called == 0u,
          "null callback returns zero without invoking runtime");
    Check(pal4::EffectSystemGlobalCallback::Set(slot, 0x93ceu) == 0x93ceu &&
              slot == 0x93ceu,
          "setter stores and returns callback address");
    Check(pal4::EffectSystemGlobalCallback::Call(slot, runtime) ==
              0x12345678u && runtime.called == 0x93ceu,
          "callback is invoked once and result is forwarded");
    Check(pal4::EffectSystemGlobalCallback::Set(slot, 0u) == 0u &&
              pal4::EffectSystemGlobalCallback::Call(slot, runtime) == 0u &&
              runtime.called == 0x93ceu,
          "clearing callback disables future calls");
    std::cout << "EffectSystem global callback tests passed\n";
}
