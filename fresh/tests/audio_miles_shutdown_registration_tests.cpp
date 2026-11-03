#include "pal4/audio_manager.h"

#include <cstdlib>
#include <iostream>

namespace {

void MilesShutdownCallback() {}

struct AtexitRecorder final : pal4::ICrtAtexitApi {
    pal4::CrtExitCallback callback = nullptr;
    int status = 0;
    int calls = 0;

    int Register(pal4::CrtExitCallback value) override {
        callback = value;
        ++calls;
        return status;
    }
};

void Check(bool value, const char* message) {
    if (!value) {
        std::cerr << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

}  // namespace

int main() {
    AtexitRecorder runtime;
    Check(pal4::AudioSystem::RegisterMilesShutdown(
              MilesShutdownCallback, runtime) == 0 &&
          runtime.callback == MilesShutdownCallback && runtime.calls == 1,
          "registers Miles callback and returns zero");
    runtime.status = -1;
    Check(pal4::AudioSystem::RegisterMilesShutdown(
              MilesShutdownCallback, runtime) == 0 && runtime.calls == 2,
          "ignores CRT registration status on every call");
}
