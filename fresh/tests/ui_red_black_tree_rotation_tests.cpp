#include "pal4/ui_red_black_tree_rotation.h"

#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IUiRedBlackTreeLinkRuntime {
    static constexpr std::uint32_t tree = 0x1000u;
    static constexpr std::uint32_t head = 0x2000u;
    static constexpr std::uint32_t nil = 0x3000u;
    std::map<std::uint32_t, std::uint32_t> words;
    Runtime() {
        words[tree + 4u] = head;
        words[head + 4u] = nil;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words.at(address);
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
    }
    std::uint32_t SharedNil() override { return nil; }
};

void CheckRootLeftRotation() {
    Runtime r;
    r.words[r.head + 4u] = 0x5000u;
    r.words[0x5004u] = r.head;
    r.words[0x5008u] = 0x6000u;
    r.words[0x6000u] = 0x7000u;
    r.words[0x6004u] = 0x5000u;
    r.words[0x7004u] = 0x6000u;
    Check(pal4::UiRedBlackTreeRotation::RotateLeft(
              r.tree, 0x5000u, r) == 0x6000u &&
              r.words.at(r.head + 4u) == 0x6000u &&
              r.words.at(0x6004u) == r.head &&
              r.words.at(0x6000u) == 0x5000u &&
              r.words.at(0x5004u) == 0x6000u &&
              r.words.at(0x5008u) == 0x7000u &&
              r.words.at(0x7004u) == 0x5000u,
          "root left rotation moves middle subtree and parents");
}

void CheckRootRightRotation() {
    Runtime r;
    r.words[r.head + 4u] = 0x5000u;
    r.words[0x5004u] = r.head;
    r.words[0x5000u] = 0x6000u;
    r.words[0x6008u] = 0x7000u;
    r.words[0x6004u] = 0x5000u;
    r.words[0x7004u] = 0x6000u;
    Check(pal4::UiRedBlackTreeRotation::RotateRight(
              r.tree, 0x5000u, r) == 0x6000u &&
              r.words.at(r.head + 4u) == 0x6000u &&
              r.words.at(0x6004u) == r.head &&
              r.words.at(0x6008u) == 0x5000u &&
              r.words.at(0x5004u) == 0x6000u &&
              r.words.at(0x5000u) == 0x7000u &&
              r.words.at(0x7004u) == 0x5000u,
          "root right rotation moves middle subtree and parents");
}

void CheckInteriorRotations() {
    Runtime left;
    left.words[left.head + 4u] = 0x8000u;
    left.words[0x8000u] = 0x5000u;
    left.words[0x5004u] = 0x8000u;
    left.words[0x5008u] = 0x6000u;
    left.words[0x6000u] = left.nil;
    left.words[0x6004u] = 0x5000u;
    Check(pal4::UiRedBlackTreeRotation::RotateLeft(
              left.tree, 0x5000u, left) == 0x6000u &&
              left.words.at(0x8000u) == 0x6000u &&
              left.words.at(0x6004u) == 0x8000u &&
              left.words.at(0x5008u) == left.nil,
          "left rotation replaces parent's left link");

    Runtime right;
    right.words[right.head + 4u] = 0x8000u;
    right.words[0x8000u] = right.nil;
    right.words[0x8008u] = 0x5000u;
    right.words[0x5004u] = 0x8000u;
    right.words[0x5000u] = 0x6000u;
    right.words[0x6008u] = right.nil;
    right.words[0x6004u] = 0x5000u;
    Check(pal4::UiRedBlackTreeRotation::RotateRight(
              right.tree, 0x5000u, right) == 0x6000u &&
              right.words.at(0x8008u) == 0x6000u &&
              right.words.at(0x6004u) == 0x8000u &&
              right.words.at(0x5000u) == right.nil,
          "right rotation replaces parent's right link");
}

}  // namespace

int main() {
    try {
        CheckRootLeftRotation();
        CheckRootRightRotation();
        CheckInteriorRotations();
        std::cout << "ui_red_black_tree_rotation_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
