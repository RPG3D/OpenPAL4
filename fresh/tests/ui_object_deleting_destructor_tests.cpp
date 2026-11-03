#include "pal4/ui_object_deleting_destructor.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

struct Runtime final : pal4::IUiObjectDeletingRuntime {
    std::vector<std::string> calls;
    void CallBaseRoutine(std::uint32_t object) override {
        Check(object == 0x5000u, "base routine object");
        calls.emplace_back("base");
    }
    void Free(std::uint32_t object) override {
        Check(object == 0x5000u, "free original object");
        calls.emplace_back("free");
    }
};

}  // namespace

int main() {
    try {
        for (const std::uint8_t flags : {0u, 1u, 2u, 3u, 0x80u, 0x81u}) {
            Runtime runtime;
            Check(pal4::UiObjectDeletingDestructor::Delete(
                      0x5000u, flags, runtime) == 0x5000u,
                  "deleting destructor returns original object");
            const auto expected = (flags & 1u) != 0u
                ? std::vector<std::string>{"base", "free"}
                : std::vector<std::string>{"base"};
            Check(runtime.calls == expected,
                  "base routine precedes conditional free");
        }
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
