#include "pal4/scene_evf_record.h"

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {

void Check(bool good, const char* message) {
    if (!good) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::ISceneEvfAtomicRuntime {
    std::uint32_t identity = 0x7b4bd0u;
    std::vector<std::string> calls;
    std::uint32_t IdentityRenderCallback() override {
        calls.push_back("identity");
        return identity;
    }
    std::uint32_t DefaultRenderCallback() override {
        calls.push_back("default");
        return 0x1234u;
    }
    void WriteCallback(std::uint32_t atomic, std::uint32_t offset,
                       std::uint32_t callback) override {
        Check(atomic == 0x8000u && offset == 72u,
              "RenderWare atomic callback slot");
        calls.push_back("write:" + std::to_string(callback));
    }
};

}  // namespace

int main() {
    Runtime runtime;
    Check(pal4::SceneEvfRecord::SetAtomicRenderCallback(
              0x8000u, runtime) == 0x8000u &&
              runtime.calls == std::vector<std::string>{
                  "identity", "write:8080336"},
          "native identity callback installed and atomic returned");
    runtime.identity = 0u;
    runtime.calls.clear();
    Check(pal4::SceneEvfRecord::SetAtomicRenderCallback(
              0x8000u, runtime) == 0x8000u &&
              runtime.calls == std::vector<std::string>{
                  "identity", "write:0", "default", "write:4660"},
          "fallback branch overwrites null callback");
}
