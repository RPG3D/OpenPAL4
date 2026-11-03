#include "pal4/game_data_property_lifecycle.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IGameDataPropertyLifecycleRuntime {
    std::vector<std::string> calls;
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        Check(address == 0x1234 &&
                  (value == 0x8444c4 || value == 0x843380),
              "constructor writes exact vtable at object start");
        calls.emplace_back(value == 0x8444c4 ? "vtable" : "numeric base");
    }
    void DestroyBase(std::uint32_t object) override {
        Check(object == 0x1234, "base destructor receives object");
        calls.emplace_back("base");
    }
    void FreeMemory(std::uint32_t object) override {
        Check(object == 0x1234, "allocator receives object");
        calls.emplace_back("free");
    }
};

}  // namespace

int main() {
    try {
        Runtime runtime;
        Check(pal4::GameDataPropertyLifecycle::Construct(0x1234, runtime) ==
                  0x1234 && runtime.calls == std::vector<std::string>{"vtable"},
              "constructor returns original object");
        runtime.calls.clear();
        Check(pal4::GameDataPropertyLifecycle::ConstructNumericBase(
                  0x1234, runtime) == 0x1234 &&
                  runtime.calls == std::vector<std::string>{"numeric base"},
              "numeric base constructor writes its own vtable");
        runtime.calls.clear();
        Check(pal4::GameDataPropertyLifecycle::Destroy(0x1234, 0, runtime) ==
                  0x1234 && runtime.calls == std::vector<std::string>{"base"},
              "zero flags only invoke base destructor");
        runtime.calls.clear();
        Check(pal4::GameDataPropertyLifecycle::Destroy(0x1234, 2, runtime) ==
                  0x1234 && runtime.calls == std::vector<std::string>{"base"},
              "other flag bits do not delete");
        runtime.calls.clear();
        Check(pal4::GameDataPropertyLifecycle::Destroy(0x1234, 3, runtime) ==
                  0x1234 && runtime.calls ==
                  std::vector<std::string>{"base", "free"},
              "bit zero frees only after base destructor");
        std::cout << "game_data_property_lifecycle_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
