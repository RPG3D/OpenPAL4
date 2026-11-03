#include "pal4/effect_system_factory_text_tree.h"

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <unordered_map>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IEffectSystemFactoryTextCursorRuntime {
    static constexpr std::uint32_t kHead = 0x3000u;
    static constexpr std::uint32_t kNil = 0x93cdf0u;
    std::unordered_map<std::uint32_t, std::uint32_t> words{
        {kHead, 0x4020u}, {kHead + 4u, 0x4000u},
        {kHead + 8u, 0x4040u},
        {0x4000u, 0x4020u}, {0x4004u, kHead},
        {0x4008u, 0x4040u},
        {0x4020u, kNil}, {0x4024u, 0x4000u},
        {0x4028u, kNil},
        {0x4040u, kNil}, {0x4044u, 0x4000u},
        {0x4048u, kNil}};
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words.at(address);
    }
};

void TestSuccessors() {
    Runtime runtime;
    const auto left = pal4::EffectSystemFactoryTextTree::Next(
        0x4020u, runtime);
    Check(left.next == 0x4000u && left.result == 0x4000u,
          "left node advances through parent with native EAX");
    const auto root = pal4::EffectSystemFactoryTextTree::Next(
        0x4000u, runtime);
    Check(root.next == 0x4040u && root.result == Runtime::kNil,
          "right subtree advances to leftmost child");
    const auto right = pal4::EffectSystemFactoryTextTree::Next(
        0x4040u, runtime);
    Check(right.next == Runtime::kHead && right.result == Runtime::kHead,
          "rightmost node advances to head");
}

}  // namespace

int main() {
    TestSuccessors();
}
