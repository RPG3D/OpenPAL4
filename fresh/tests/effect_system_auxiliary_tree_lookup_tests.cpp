#include "pal4/effect_system_auxiliary_tree_lookup.h"

#include <array>
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

struct Runtime final : pal4::IEffectSystemAuxiliaryLookupRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words;

    std::uint32_t ReadWord(std::uint32_t address) const override {
        return words.at(address);
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
    }
    void MakeTree(std::uint32_t tree, std::uint32_t head,
                  std::uint32_t nil, std::uint32_t root) {
        const auto left = root + 0x20u;
        const auto right = root + 0x40u;
        words[tree + 4u] = head;
        words[head] = left;
        words[head + 4u] = root;
        words[head + 8u] = right;
        words[root] = left;
        words[root + 4u] = head;
        words[root + 8u] = right;
        words[root + 12u] = 8u;
        words[left] = nil;
        words[left + 4u] = root;
        words[left + 8u] = nil;
        words[left + 12u] = 4u;
        words[right] = nil;
        words[right + 4u] = root;
        words[right + 8u] = nil;
        words[right + 12u] = 12u;
        words[nil] = nil;
        words[nil + 8u] = nil;
    }
};

void TestBounds() {
    Runtime runtime;
    runtime.MakeTree(0x2000u, 0x3000u, 0x93ce10u, 0x4000u);
    runtime.MakeTree(0x2100u, 0x3100u, 0x93cdfcu, 0x5000u);
    constexpr std::array<std::uint32_t, 8> keys{
        0u, 4u, 5u, 8u, 9u, 12u, 13u, 0xffffffffu};
    constexpr std::array<std::uint32_t, 8> first{
        0x4020u, 0x4020u, 0x4000u, 0x4000u,
        0x4040u, 0x4040u, 0x3000u, 0x3000u};
    constexpr std::array<std::uint32_t, 8> second{
        0x5020u, 0x5020u, 0x5000u, 0x5000u,
        0x5040u, 0x5040u, 0x3100u, 0x3100u};
    for (std::size_t i = 0; i < keys.size(); ++i) {
        runtime.words[0x2600u] = keys[i];
        Check(pal4::EffectSystemAuxiliaryTreeLookup::LowerBoundClump(
                  0x2000u, 0x2600u, runtime) == first[i] &&
                  pal4::EffectSystemAuxiliaryTreeLookup::LowerBoundAtomic(
                      0x2100u, 0x2600u, runtime) == second[i] &&
                  pal4::EffectSystemAuxiliaryTreeLookup::
                      LowerBoundAtomicInto(0x2100u, 0x2700u, 0x2600u,
                                           runtime) == 0x2700u &&
                  runtime.ReadWord(0x2700u) == second[i],
              "unsigned lower bounds match both nils and result-slot ABI");
    }
}

void TestSuccessors() {
    Runtime runtime;
    runtime.MakeTree(0x2000u, 0x3000u, 0x93ce10u, 0x4000u);
    runtime.MakeTree(0x2100u, 0x3100u, 0x93cdfcu, 0x5000u);
    auto first = 0x4020u;
    auto second = 0x5020u;
    Check(pal4::EffectSystemAuxiliaryTreeLookup::NextClump(
              first, runtime) == 0x4000u && first == 0x4000u &&
              pal4::EffectSystemAuxiliaryTreeLookup::NextAtomic(
                  second, runtime) == 0x5000u && second == 0x5000u,
          "left child advances to parent in both trees");
    Check(pal4::EffectSystemAuxiliaryTreeLookup::NextClump(
              first, runtime) == 0x93ce10u && first == 0x4040u &&
              pal4::EffectSystemAuxiliaryTreeLookup::NextAtomic(
                  second, runtime) == 0x93cdfcu && second == 0x5040u,
          "right-subtree branch returns nil while updating cursor");
    Check(pal4::EffectSystemAuxiliaryTreeLookup::NextClump(
              first, runtime) == 0x3000u && first == 0x3000u &&
              pal4::EffectSystemAuxiliaryTreeLookup::NextAtomic(
                  second, runtime) == 0x3100u && second == 0x3100u,
          "rightmost elements advance to their headers");
}

}  // namespace

int main() {
    TestBounds();
    TestSuccessors();
}
