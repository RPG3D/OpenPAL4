#include "pal4/actor_manager_property_facet.h"

#include <array>
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
constexpr std::uint32_t object = 0x1000u;

struct Runtime final : pal4::IActorManagerPropertyFacetRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> trace;

    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
        trace.emplace_back("write:" + std::to_string(address) + ":" +
                           std::to_string(value));
    }
    void Free(std::uint32_t address) override {
        Check(address == object, "facet free address");
        trace.emplace_back("free");
    }
};

void ConstructAndMark() {
    using pal4::ActorManagerPropertyFacet;
    Runtime runtime;
    runtime.words[object + 4u] = 0x999u;
    Check(ActorManagerPropertyFacet::Construct(
              object, 0x222u, 0x333u, runtime) == object &&
              runtime.words[object] == ActorManagerPropertyFacet::kBaseVtable &&
              runtime.words[object + 4u] == 0x999u &&
              runtime.words[object + 8u] == 0x222u &&
              runtime.words[object + 12u] == 0x333u &&
              runtime.trace.size() == 3u,
          "constructor writes vtable and two payload words only");
    runtime.trace.clear();
    ActorManagerPropertyFacet::MarkFirst(object, runtime);
    ActorManagerPropertyFacet::MarkSecond(object, runtime);
    Check(runtime.trace == std::vector<std::string>{
              "write:4096:8671112", "write:4096:8671120"} &&
              runtime.words[object] == ActorManagerPropertyFacet::kSecondVtable,
          "vtable markers write first then second variant");
}

void DeleteVariants() {
    using pal4::ActorManagerPropertyFacet;
    using Delete = std::uint32_t (*)(
        std::uint32_t, std::uint8_t,
        pal4::IActorManagerPropertyFacetRuntime&);
    const std::array<Delete, 4> methods{
        &ActorManagerPropertyFacet::DeleteFirst,
        &ActorManagerPropertyFacet::DeleteSecond,
        &ActorManagerPropertyFacet::DeleteFirstInline,
        &ActorManagerPropertyFacet::DeleteSecondInline};
    for (std::size_t i = 0; i < methods.size(); ++i) {
        for (const auto flags : std::array<std::uint8_t, 4>{0u, 1u, 2u, 3u}) {
            Runtime runtime;
            const auto vtable = i % 2u == 0u
                ? ActorManagerPropertyFacet::kFirstVtable
                : ActorManagerPropertyFacet::kSecondVtable;
            Check(methods[i](object, flags, runtime) == object &&
                      runtime.words[object] == vtable &&
                      runtime.trace.size() == (flags & 1u ? 2u : 1u) &&
                      runtime.trace.front() ==
                          "write:4096:" + std::to_string(vtable) &&
                      (!(flags & 1u) || runtime.trace.back() == "free"),
                  "deleting variants honor low flag bit and write-before-free");
        }
    }
}

}  // namespace

int main() {
    try {
        ConstructAndMark();
        DeleteVariants();
        std::cout << "actor_manager_property_facet_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
