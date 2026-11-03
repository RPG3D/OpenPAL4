#include "pal4/ui_red_black_tree_iterator.h"

#include <algorithm>
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

struct Runtime final : pal4::IUiRedBlackTreeIteratorRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;
    std::uint32_t nil = 0x9000u;
    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.emplace_back("read " + std::to_string(address));
        return words.at(address);
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        calls.emplace_back("write " + std::to_string(address) + "=" +
                           std::to_string(value));
        words[address] = value;
    }
    std::uint32_t SharedNil() override { return nil; }
};

void CheckRightSubtree() {
    Runtime direct;
    direct.words = {{0x5000u, 0x6000u}, {0x6008u, 0x7000u},
                    {0x7000u, 0x9000u}};
    Check(pal4::UiRedBlackTreeIterator::Next(0x5000u, direct) ==
                  0x9000u && direct.words[0x5000u] == 0x7000u,
          "right child becomes next cursor; EAX retains nil");
    Check(direct.calls == (std::vector<std::string>{
              "read 20480", "read 24584", "read 28672",
              "write 20480=28672"}),
          "right-child call order");

    Runtime left_spine;
    left_spine.words = {{0x5000u, 0x6000u}, {0x6008u, 0x7000u},
                        {0x7000u, 0x7100u}, {0x7100u, 0x7200u},
                        {0x7200u, 0x9000u}};
    Check(pal4::UiRedBlackTreeIterator::Next(0x5000u, left_spine) ==
                  0x9000u && left_spine.words[0x5000u] == 0x7200u,
          "right subtree follows full left spine");
}

void CheckAncestors() {
    Runtime parent;
    parent.words = {{0x5000u, 0x6000u}, {0x6008u, 0x9000u},
                    {0x6004u, 0x7000u}, {0x7008u, 0x9000u}};
    Check(pal4::UiRedBlackTreeIterator::Next(0x5000u, parent) ==
                  0x7000u && parent.words[0x5000u] == 0x7000u,
          "left-child path advances to parent");

    Runtime climb;
    climb.words = {{0x5000u, 0x6000u}, {0x6008u, 0x9000u},
                   {0x6004u, 0x7000u}, {0x7008u, 0x6000u},
                   {0x7004u, 0x8000u}, {0x8008u, 0x9000u}};
    Check(pal4::UiRedBlackTreeIterator::Next(0x5000u, climb) ==
                  0x8000u && climb.words[0x5000u] == 0x8000u,
          "right-child ancestry climbs to first left turn");
    Check(climb.calls.end() !=
              std::find(climb.calls.begin(), climb.calls.end(),
                        "write 20480=28672"),
          "intermediate ancestor is written before final cursor");

    Runtime sentinel;
    sentinel.words = {{0x5000u, 0x6000u}, {0x6008u, 0x9000u},
                      {0x6004u, 0x7000u}, {0x7008u, 0x6000u},
                      {0x7004u, 0x6000u}};
    Check(pal4::UiRedBlackTreeIterator::Next(0x5000u, sentinel) ==
                  0x6000u && sentinel.words[0x5000u] == 0x7000u,
          "maximum node advances to head without overwriting it");
}

void CheckPreviousOutput() {
    Runtime right;
    right.words = {{0x5000u, 0x6000u}, {0x6008u, 0x7000u},
                   {0x7000u, 0x7100u}, {0x7100u, 0x9000u}};
    Check(pal4::UiRedBlackTreeIterator::AdvanceAndReturnPrevious(
              0x5000u, 0x5100u, right) == 0x5100u &&
              right.words.at(0x5000u) == 0x7100u &&
              right.words.at(0x5100u) == 0x6000u,
          "right branch advances cursor and returns former node");

    Runtime ascent;
    ascent.words = {{0x5000u, 0x6000u}, {0x6008u, 0x9000u},
                    {0x6004u, 0x7000u}, {0x7008u, 0x9000u}};
    Check(pal4::UiRedBlackTreeIterator::AdvanceAndReturnPrevious(
              0x5000u, 0x5100u, ascent) == 0x5100u &&
              ascent.words.at(0x5000u) == 0x7000u &&
              ascent.words.at(0x5100u) == 0x6000u,
          "ancestor branch returns former node");

    Runtime maximum;
    maximum.words = {{0x5000u, 0x6000u}, {0x6008u, 0x9000u},
                     {0x6004u, 0x7000u}, {0x7008u, 0x6000u},
                     {0x7004u, 0x6000u}};
    Check(pal4::UiRedBlackTreeIterator::AdvanceAndReturnPrevious(
              0x5000u, 0x5100u, maximum) == 0x5100u &&
              maximum.words.at(0x5000u) == 0x7000u &&
              maximum.words.at(0x5100u) == 0x6000u,
          "maximum-to-head preserves iterator boundary");
}

void CheckPredecessors() {
    Runtime header;
    header.words = {{0x5000u, 0x6000u}, {0x6004u, 0x7000u},
                    {0x6020u, 0u}, {0x7004u, 0x6000u},
                    {0x6008u, 0x8000u}};
    Check(pal4::UiRedBlackTreeIterator::Previous(0x5000u, header) ==
                  0x8000u && header.words.at(0x5000u) == 0x8000u,
          "header predecessor is maximum");

    Runtime subtree;
    subtree.words = {{0x5000u, 0x6000u}, {0x6004u, 0x8000u},
                     {0x6020u, 1u}, {0x8004u, 0x9000u},
                     {0x6000u, 0x7000u}, {0x7008u, 0x7100u},
                     {0x7108u, 0x9000u}};
    Check(pal4::UiRedBlackTreeIterator::Previous(0x5000u, subtree) ==
                  0x9000u && subtree.words.at(0x5000u) == 0x7100u,
          "left subtree uses right spine; EAX retains nil");

    Runtime parent;
    parent.words = {{0x5000u, 0x6000u}, {0x6004u, 0x7000u},
                    {0x6020u, 1u}, {0x7004u, 0x8000u},
                    {0x6000u, 0x9000u}, {0x7000u, 0x9000u}};
    Check(pal4::UiRedBlackTreeIterator::Previous(0x5000u, parent) ==
                  0x7000u && parent.words.at(0x5000u) == 0x7000u,
          "right child ascends to parent");

    Runtime climb;
    climb.words = {{0x5000u, 0x6000u}, {0x6004u, 0x7000u},
                   {0x6020u, 1u}, {0x7004u, 0x8000u},
                   {0x6000u, 0x9000u}, {0x7000u, 0x6000u},
                   {0x8000u, 0x9000u}};
    Check(pal4::UiRedBlackTreeIterator::Previous(0x5000u, climb) ==
                  0x8000u && climb.words.at(0x5000u) == 0x8000u,
          "left-child ancestry climbs to first right turn");
}

void CheckAlternateSharedNil() {
    Runtime r;
    r.nil = 0xa000u;
    r.words = {{0x5000u, 0x6000u}, {0x6008u, 0x7000u},
               {0x7000u, r.nil}};
    Check(pal4::UiRedBlackTreeIterator::Next(0x5000u, r) == r.nil &&
              r.words.at(0x5000u) == 0x7000u,
          "0x4c1650 shares Next logic with a separate nil");
}

void CheckFactoryPredecessors() {
    Runtime header;
    header.nil = 0xa000u;
    header.words = {{0x5000u, 0x6000u}, {0x6004u, 0x7000u},
                    {0x6014u, 0u}, {0x7004u, 0x6000u},
                    {0x6008u, 0x8000u}};
    Check(pal4::UiRedBlackTreeIterator::PreviousFactory(
              0x5000u, header) == 0x8000u &&
              header.words.at(0x5000u) == 0x8000u,
          "factory head returns maximum using +0x14 colour");

    Runtime subtree;
    subtree.nil = 0xa000u;
    subtree.words = {{0x5000u, 0x6000u}, {0x6004u, 0x8000u},
                     {0x6014u, 1u}, {0x6000u, 0x7000u},
                     {0x7008u, 0x7100u}, {0x7108u, 0xa000u}};
    Check(pal4::UiRedBlackTreeIterator::PreviousFactory(
              0x5000u, subtree) == 0xa000u &&
              subtree.words.at(0x5000u) == 0x7100u,
          "factory predecessor follows right spine");

    Runtime climb;
    climb.nil = 0xa000u;
    climb.words = {{0x5000u, 0x6000u}, {0x6004u, 0x7000u},
                   {0x6014u, 1u}, {0x6000u, 0xa000u},
                   {0x7000u, 0x6000u}, {0x7004u, 0x8000u},
                   {0x8000u, 0xa000u}};
    Check(pal4::UiRedBlackTreeIterator::PreviousFactory(
              0x5000u, climb) == 0x8000u &&
              climb.words.at(0x5000u) == 0x8000u,
          "factory predecessor climbs left-child ancestry");
}

void CheckFactorySuccessors() {
    Runtime subtree;
    subtree.nil = 0xdeadu;
    subtree.words = {{0x5000u, 0x6000u}, {0x6008u, 0x7000u},
                     {0x7000u, 0x7100u}, {0x7100u, 0xdeadu}};
    Check(pal4::UiRedBlackTreeIterator::Next(0x5000u, subtree) ==
                  0xdeadu && subtree.words.at(0x5000u) == 0x7100u,
          "factory successor descends to minimum of right subtree");

    Runtime ancestor;
    ancestor.nil = 0xdeadu;
    ancestor.words = {{0x5000u, 0x6000u}, {0x6008u, 0xdeadu},
                      {0x6004u, 0x7000u}, {0x7008u, 0xdeadu}};
    Check(pal4::UiRedBlackTreeIterator::Next(0x5000u, ancestor) ==
                  0x7000u && ancestor.words.at(0x5000u) == 0x7000u,
          "factory successor ascends to parent with factory nil");
}

}  // namespace

int main() {
    try {
        CheckRightSubtree();
        CheckAncestors();
        CheckPreviousOutput();
        CheckPredecessors();
        CheckAlternateSharedNil();
        CheckFactoryPredecessors();
        CheckFactorySuccessors();
        std::cout << "ui_red_black_tree_iterator_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
