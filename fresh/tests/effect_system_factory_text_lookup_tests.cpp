#include "pal4/effect_system_factory_text_tree.h"

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>
#include <unordered_map>
#include <utility>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IEffectSystemFactoryTextLookupRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words{
        {0x2004u, 0x3000u}, {0x3004u, 0x4000u},
        {0x4000u, 0x4020u}, {0x4008u, 0x4040u},
        {0x4020u, 0x93cdf0u}, {0x4028u, 0x93cdf0u},
        {0x4040u, 0x93cdf0u}, {0x4048u, 0x93cdf0u}};
    std::unordered_map<std::uint32_t, std::string> names{
        {0x400cu, "M"}, {0x402cu, "F"}, {0x404cu, "T"},
        {0x5000u, ""}};
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words.at(address);
    }
    std::int32_t CompareText(std::uint32_t first,
                             std::uint32_t second) override {
        return names.at(first).compare(names.at(second));
    }
};

void TestExactLookup() {
    Runtime runtime;
    for (const auto& [name, expected] :
         {std::pair{"F", 0x4020u}, {"M", 0x4000u},
          {"T", 0x4040u}, {"A", 0x3000u},
          {"H", 0x3000u}, {"Z", 0x3000u}}) {
        runtime.names[0x5000u] = name;
        Check(pal4::EffectSystemFactoryTextTree::FindExact(
                  0x2000u, 0x5000u, runtime) == expected,
              "exact lookup returns matching node or head");
    }
    runtime.words[0x3004u] = 0x93cdf0u;
    Check(pal4::EffectSystemFactoryTextTree::FindExact(
              0x2000u, 0x5000u, runtime) == 0x3000u,
          "empty tree returns head");
}

void TestLowerBound() {
    Runtime runtime;
    for (const auto& [name, expected] :
         {std::pair{"A", 0x4020u}, {"F", 0x4020u},
          {"H", 0x4000u}, {"M", 0x4000u},
          {"S", 0x4040u}, {"T", 0x4040u},
          {"Z", 0x3000u}}) {
        runtime.names[0x5000u] = name;
        Check(pal4::EffectSystemFactoryTextTree::LowerBound(
                  0x2000u, 0x5000u, runtime) == expected,
              "lower bound returns first name not less than input");
    }
    runtime.words[0x3004u] = 0x93cdf0u;
    Check(pal4::EffectSystemFactoryTextTree::LowerBound(
              0x2000u, 0x5000u, runtime) == 0x3000u,
          "empty lower bound returns head");
}

}  // namespace

int main() {
    TestExactLookup();
    TestLowerBound();
}
