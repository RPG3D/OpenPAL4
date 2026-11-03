#include "pal4/ui_tree_initializer.h"

#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IUiTreeInitializerRuntime {
    std::map<std::uint32_t, std::uint8_t> bytes{{0x2000u, 0x31u},
                                                 {0x3000u, 0x72u}};
    std::vector<std::string> calls;

    std::uint8_t ReadByte(std::uint32_t address) override {
        calls.push_back("read " + std::to_string(address));
        return bytes.at(address);
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        calls.push_back("write " + std::to_string(address));
        bytes[address] = value;
    }
    void InitializeTree(std::uint32_t tree) override {
        Check(tree == 0x1000u, "tree base");
        calls.emplace_back("initialize");
    }
};

}  // namespace

int main() {
    try {
        Runtime runtime;
        Check(pal4::UiTreeInitializer::Initialize(
                  0x1000u, 0x2000u, 0xa5u, 0x3000u, runtime) == 0x1000u &&
                  runtime.bytes.at(0x1000u) == 0x72u &&
                  runtime.bytes.at(0x1001u) == 0x31u &&
                  runtime.bytes.at(0x1008u) == 0xa5u &&
                  runtime.calls ==
                      (std::vector<std::string>{"read 12288", "write 4096",
                                                "read 8192", "write 4104",
                                                "write 4097", "initialize"}),
              "tree bytes and initialization order");
        std::cout << "ui_tree_initializer_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
