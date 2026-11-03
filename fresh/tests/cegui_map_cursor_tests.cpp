#include "pal4/cegui_map_operations.h"

#include <cstdlib>
#include <iostream>
#include <map>

namespace {

constexpr std::uint32_t kHeader = 0x2000u;
constexpr std::uint32_t kRoot = 0x3000u;
constexpr std::uint32_t kLeft = 0x3100u;
constexpr std::uint32_t kRight = 0x3200u;
constexpr std::uint32_t kLeftRight = 0x3300u;
constexpr std::uint32_t kRightLeft = 0x3400u;
constexpr std::uint32_t kNil = 0x9000u;

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

class Runtime final : public pal4::ICeguiMapCursorRuntime {
public:
    std::map<std::uint32_t, std::uint32_t> words;

    Runtime() {
        words[kHeader] = kLeft;
        words[kHeader + 4u] = kRoot;
        words[kHeader + 8u] = kRight;
        words[kHeader + 0x14u] = 0u;
        words[kRoot] = kLeft;
        words[kRoot + 4u] = kHeader;
        words[kRoot + 8u] = kRight;
        words[kRoot + 0x14u] = 1u;
        words[kLeft] = kNil;
        words[kLeft + 4u] = kRoot;
        words[kLeft + 8u] = kLeftRight;
        words[kLeft + 0x14u] = 1u;
        words[kRight] = kRightLeft;
        words[kRight + 4u] = kRoot;
        words[kRight + 8u] = kNil;
        words[kRight + 0x14u] = 1u;
        words[kLeftRight] = kNil;
        words[kLeftRight + 4u] = kLeft;
        words[kLeftRight + 8u] = kNil;
        words[kLeftRight + 0x14u] = 0u;
        words[kRightLeft] = kNil;
        words[kRightLeft + 4u] = kRight;
        words[kRightLeft + 8u] = kNil;
        words[kRightLeft + 0x14u] = 0u;
    }

    std::uint32_t ReadDword(const std::uint32_t address) override {
        return words.at(address);
    }
};

void PreviousFromHeaderUsesRightmost() {
    Runtime runtime;
    const pal4::CeguiTreeGlobals globals{1u, kNil};
    auto cursor = kHeader;
    Check(pal4::CeguiMapOperations::PreviousCursor(
              cursor, globals, runtime) == kRight && cursor == kRight,
          "red header special case moves to rightmost node");
}

void PreviousDescendsRightAndAscends() {
    Runtime runtime;
    const pal4::CeguiTreeGlobals globals{1u, kNil};
    auto cursor = kRoot;
    Check(pal4::CeguiMapOperations::PreviousCursor(
              cursor, globals, runtime) == kNil &&
              cursor == kLeftRight,
          "predecessor descends to rightmost node of left subtree");
    cursor = kRight;
    Check(pal4::CeguiMapOperations::PreviousCursor(
              cursor, globals, runtime) == kNil &&
              cursor == kRightLeft,
          "right child with left subtree descends correctly");
    cursor = kLeft;
    Check(pal4::CeguiMapOperations::PreviousCursor(
              cursor, globals, runtime) == kHeader &&
              cursor == kHeader,
          "predecessor ascends from minimum to header");
    cursor = kRightLeft;
    Check(pal4::CeguiMapOperations::PreviousCursor(
              cursor, globals, runtime) == kRoot && cursor == kRoot,
          "predecessor ascends from a left child to grandparent");
}

void NextDescendsLeftAndAscends() {
    Runtime runtime;
    const pal4::CeguiTreeGlobals globals{1u, kNil};
    auto cursor = kRoot;
    Check(pal4::CeguiMapOperations::NextCursor(
              cursor, globals, runtime) == kNil &&
              cursor == kRightLeft,
          "successor descends to leftmost node of right subtree");
    cursor = kLeft;
    Check(pal4::CeguiMapOperations::NextCursor(
              cursor, globals, runtime) == kNil &&
              cursor == kLeftRight,
          "left child with right subtree descends correctly");
    cursor = kRight;
    Check(pal4::CeguiMapOperations::NextCursor(
              cursor, globals, runtime) == kHeader &&
              cursor == kHeader,
          "successor ascends from maximum to header");
    cursor = kLeftRight;
    Check(pal4::CeguiMapOperations::NextCursor(
              cursor, globals, runtime) == kRoot && cursor == kRoot,
          "successor ascends from a right child to grandparent");
}

}  // namespace

int main() {
    PreviousFromHeaderUsesRightmost();
    PreviousDescendsRightAndAscends();
    NextDescendsLeftAndAscends();
    std::cout << "3 CEGUI map cursor test cases passed\n";
}
