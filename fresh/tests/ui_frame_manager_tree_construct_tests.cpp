#include "pal4/ui_frame_manager_singleton.h"

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

struct Runtime final : pal4::IUiFrameManagerTreeConstructRuntime {
    std::map<std::uint32_t, std::uint8_t> bytes{
        {0x7000u, 0x12u}, {0x8000u, 0xe3u}};
    std::vector<std::string> calls;

    std::uint8_t ReadByte(std::uint32_t address) override {
        calls.emplace_back("read " + std::to_string(address));
        return bytes.at(address);
    }
    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override {
        calls.emplace_back("write " + std::to_string(address));
        bytes[address] = value;
    }
    void ConstructRedBlackTree(std::uint32_t tree) override {
        Check(tree == 0x5114u, "red-black tree receiver");
        Check(bytes.at(tree) == 0xe3u && bytes.at(tree + 1u) == 0x12u &&
                  bytes.at(tree + 8u) == 0x7au,
              "tree bytes are initialized before base constructor");
        calls.emplace_back("construct tree");
    }
};

}  // namespace

int main() {
    try {
        Runtime runtime;
        Check(pal4::UiFrameManagerSingleton::ConstructEmbeddedTree(
                  0x5114u, 0x7000u, 0x7au, 0x8000u, runtime) == 0x5114u,
              "constructor returns this tree");
        Check(runtime.calls == (std::vector<std::string>{
                  "read 32768", "write 20756", "read 28672",
                  "write 20764", "write 20757", "construct tree"}),
              "argument reads and byte writes follow assembly order");
        std::cout << "ui_frame_manager_tree_construct_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
