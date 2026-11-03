#include "pal4/ui_frame_manager_node_allocator.h"

#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

struct Runtime final : pal4::IUiFrameManagerNodeAllocatorRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;
    std::uint32_t Allocate(std::uint32_t size) override {
        Check(size == 0x28u, "node allocation size");
        calls.emplace_back("allocate");
        return 0x7000u;
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        calls.emplace_back("write " + std::to_string(address));
        words[address] = value;
    }
};

}  // namespace

int main() {
    try {
        for (const auto first : {0u, 0x6000u}) {
            for (const auto second : {0u, 0x8000u}) {
                Runtime runtime;
                Check(pal4::UiFrameManagerNodeAllocator::AllocateAndInitialize(
                          first, second, runtime) == 0x7000u,
                      "returns allocated node");
                Check(runtime.words[0x7000u] ==
                          (first != 0u ? first : 0x7000u) &&
                          runtime.words[0x7004u] ==
                              (second != 0u ? second : 0x7000u),
                      "each link defaults independently to self");
                Check(runtime.calls == (std::vector<std::string>{
                          "allocate", "write 28672", "write 28676"}),
                      "allocation and link writes follow assembly order");
            }
        }
        std::cout << "ui_frame_manager_node_allocator_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
