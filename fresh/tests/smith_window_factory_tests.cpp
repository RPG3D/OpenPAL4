#include "pal4/smith_window_factory.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::ISmithWindowFactoryRuntime {
    std::uint32_t allocated = 0x5000u;
    std::uint32_t constructed = 0x5100u;
    bool throw_in_constructor = false;
    std::vector<std::string> calls;

    std::uint32_t Allocate(std::uint32_t bytes) override {
        Check(bytes == 0xfcu, "SmithWindow allocation size");
        calls.emplace_back("allocate");
        return allocated;
    }
    std::uint32_t Construct(std::uint32_t object,
                            std::uint32_t layout_name) override {
        Check(object == allocated && layout_name == 0x7000u,
              "constructor object and layout argument");
        calls.emplace_back("construct");
        if (throw_in_constructor)
            throw std::runtime_error("constructor failure");
        return constructed;
    }
    void Free(std::uint32_t object) override {
        Check(object == allocated, "failed constructor frees object");
        calls.emplace_back("free");
    }
};

void CheckPaths() {
    Runtime success;
    Check(pal4::SmithWindowFactory::Create(0x7000u, success) == 0x5100u &&
              success.calls ==
                  std::vector<std::string>{"allocate", "construct"},
          "factory forwards constructed pointer and argument");

    Runtime empty;
    empty.allocated = 0u;
    Check(pal4::SmithWindowFactory::Create(0x7000u, empty) == 0u &&
              empty.calls == std::vector<std::string>{"allocate"},
          "allocation failure skips constructor");

    Runtime failure;
    failure.throw_in_constructor = true;
    bool threw = false;
    try {
        (void)pal4::SmithWindowFactory::Create(0x7000u, failure);
    } catch (const std::runtime_error&) {
        threw = true;
    }
    Check(threw && failure.calls ==
              (std::vector<std::string>{
                  "allocate", "construct", "free"}),
          "exception path frees the allocated object");
}

}  // namespace

int main() {
    try {
        CheckPaths();
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
