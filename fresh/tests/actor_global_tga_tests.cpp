#include "pal4/actor_global_tga.h"

#include <cstdint>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IActorGlobalTgaRuntime {
    std::vector<std::uint32_t> calls;
    std::function<void()> callback;
    int register_result = -17;

    std::uint32_t InitializeHeader(std::uint32_t address) override {
        calls.push_back(address);
        return 0x1234u;
    }
    int RegisterAtExit(const std::function<void()>& cleanup) override {
        Check(calls.size() == 1, "header initialized before registration");
        callback = cleanup;
        calls.push_back(0u);
        return register_result;
    }
};

}  // namespace

int main() {
    try {
        Runtime runtime;
        Check(pal4::ActorGlobalTga::InitializeHeader(runtime) == 0x1234u &&
              runtime.calls == std::vector<std::uint32_t>{0x8f60b0u},
              "TGA header thunk forwards address and result");
        runtime.calls.clear();
        Check(pal4::ActorGlobalTga::Initialize(runtime) == -17 &&
              runtime.calls ==
                  (std::vector<std::uint32_t>{0x8f60b0u, 0u}) &&
              static_cast<bool>(runtime.callback),
              "global initialization registers cleanup and forwards result");
        runtime.callback();
        pal4::ActorGlobalTga::CleanupHeader();
        std::cout << "actor_global_tga_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
