#include "pal4/crt_noop_atexit_registration.h"

#include <cstdlib>
#include <iostream>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::CrtNoopAtExitRuntimeStub {
    std::vector<std::uint32_t> callbacks;
    int result{17};
    int RegisterAtExit(std::uint32_t callback) override {
        callbacks.push_back(callback);
        return result;
    }
};

void TestRegistration() {
    Runtime runtime;
    std::uint8_t flag = 0x80u;
    Check(pal4::CrtNoopAtExitRegistration::Run(flag, runtime) == 17 &&
              flag == 0x81u,
          "first call sets only low bit and returns CRT result");
    Check(pal4::CrtNoopAtExitRegistration::Run(flag, runtime) == 17 &&
              flag == 0x81u,
          "second call leaves low bit set");
    Check(runtime.callbacks == std::vector<std::uint32_t>{
              0x5e0560u, 0x5e0560u},
          "CRT callback is registered on every call");
}

void TestHostAtexitAdapter() {
    pal4::CrtNoopAtExitRuntime runtime;
    Check(runtime.RegisterAtExit(0x402420u) == -1,
          "host adapter rejects addresses other than the reviewed callback");
    Check(runtime.RegisterAtExit(pal4::CrtNoopAtExitRegistration::kCallback) == 0,
          "reviewed no-op callback registers through host atexit");
}

}  // namespace

int main() {
    TestRegistration();
    TestHostAtexitAdapter();
}
