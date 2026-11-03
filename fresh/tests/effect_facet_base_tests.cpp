#include "pal4/effect_facet_base.h"

#include <array>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IEffectFacetBaseRuntime {
    std::vector<std::string> events;
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        events.push_back("write:" + std::to_string(address) + ":" +
                         std::to_string(value));
    }
    void Free(std::uint32_t address) override {
        events.push_back("free:" + std::to_string(address));
    }
};

}  // namespace

int main() {
    constexpr std::array<std::uint32_t, 13> expected{
        0x84a1dcu, 0x84a1a0u, 0x84a194u, 0x84a1d0u, 0x84a1c8u,
        0x84a1acu, 0x84a1bcu, 0x84a184u, 0x84a178u, 0x84a2c0u,
        0x84a430u, 0x84a420u, 0x84a408u};
    Runtime runtime;
    for (std::uint32_t index = 0u; index < expected.size(); ++index) {
        const auto kind = static_cast<pal4::EffectFacetBaseKind>(index);
        const auto address = 0x5000u + 4u * index;
        pal4::EffectFacetBase::Initialize(address, kind, runtime);
        Check(runtime.events == std::vector<std::string>{
                  "write:" + std::to_string(address) + ":" +
                      std::to_string(expected[index])},
              "facet initializer writes its exact vtable");
        runtime.events.clear();
        Check(pal4::EffectFacetBase::DeletingDestroy(
                  address, kind, 2u, runtime) == address &&
                  runtime.events == std::vector<std::string>{
                      "write:" + std::to_string(address) + ":" +
                          std::to_string(expected[index])},
              "deleting destructor preserves pointer without flag bit zero");
        runtime.events.clear();
        Check(pal4::EffectFacetBase::DeletingDestroy(
                  address, kind, 3u, runtime) == address &&
                  runtime.events == std::vector<std::string>{
                      "write:" + std::to_string(address) + ":" +
                          std::to_string(expected[index]),
                      "free:" + std::to_string(address)},
              "deleting destructor restores vtable before optional free");
        runtime.events.clear();
    }
}
