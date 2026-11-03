#include "pal4/input_method_static_registration.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IInputMethodStaticRuntime {
    std::vector<std::string> calls;
    std::int32_t result = -17;
    void BeginInputMethod(std::uint32_t object) override {
        Check(object == 0x9078f0u, "giIMMBegin object");
        calls.emplace_back("begin");
    }
    void ReleaseVirtualMemory(std::uint32_t object) override {
        Check(object == 0x9078f0u, "virtual-memory cleanup object");
        calls.emplace_back("release");
    }
    std::int32_t RegisterAtExit(std::uint32_t callback) override {
        Check(callback == 0x5c4460u, "native cleanup callback address");
        calls.emplace_back("atexit");
        return result;
    }
};

}  // namespace

int main() {
    try {
        Runtime runtime;
        Check(pal4::InputMethodStaticRegistration::Register(runtime) == -17 &&
                  runtime.calls == std::vector<std::string>{"begin", "atexit"},
              "registration initializes first and returns signed CRT result");
        runtime.calls.clear();
        pal4::InputMethodStaticRegistration::Initialize(runtime);
        pal4::InputMethodStaticRegistration::Cleanup(runtime);
        Check(runtime.calls == std::vector<std::string>{"begin", "release"},
              "direct initializer and cleanup forward same native object");
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
