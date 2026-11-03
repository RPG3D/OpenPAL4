#include "pal4/log_content_accessor.h"

#include <cstdlib>
#include <iostream>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::ILogContentRuntime {
    std::uint32_t source = 0u;
    std::uint32_t destination = 0u;
    unsigned calls = 0u;

    void CopyString(const std::uint32_t input,
                    const std::uint32_t output) override {
        source = input;
        destination = output;
        ++calls;
    }
};

}  // namespace

int main() {
    for (const auto log : {0x1000u, 0xfffffffcu}) {
        Runtime runtime;
        Check(pal4::LogContentAccessor::Get(log, 0x2000u, runtime) ==
                  0x2000u &&
                  runtime.source == log + 12u &&
                  runtime.destination == 0x2000u && runtime.calls == 1u,
              "log content copies from +12 and returns destination");
    }
}
