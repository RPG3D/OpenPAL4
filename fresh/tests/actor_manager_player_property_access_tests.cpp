#include "pal4/actor_manager_player_property_access.h"

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

constexpr std::uint32_t kActor = 0x1000u;

struct Runtime final : pal4::IActorManagerPlayerPropertyAccessRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> trace;
    std::uint32_t object_result = 0x88u;
    std::uint32_t fallback_result = 0x99u;

    std::uint32_t ReadWord(std::uint32_t address) override {
        trace.emplace_back("read:" + std::to_string(address));
        return words[address];
    }
    void ActorSlot52(std::uint32_t actor, std::uint32_t property) override {
        Check(actor == kActor && property == 4u, "player virtual arguments");
        trace.emplace_back("actor52");
        words[kActor + 800u] = 0x3300u;
    }
    std::uint32_t ObjectSlot168(std::uint32_t object, std::uint32_t actor,
                                std::uint32_t property,
                                std::uint32_t value) override {
        Check(object == 0x3300u && actor == kActor && property == 4u &&
                  value == 0x77u,
              "mode-four object virtual arguments");
        trace.emplace_back("object168");
        return object_result;
    }
    std::uint32_t ForwardPropertySlot168(
        std::uint32_t actor, std::uint32_t property,
        std::uint32_t value) override {
        Check(actor == kActor && property == 3u && value == 0x77u,
              "fallback property arguments");
        trace.emplace_back("fallback168");
        return fallback_result;
    }
};

}  // namespace

int main() {
    try {
        Runtime runtime;
        runtime.words[kActor + 1316u] = 0x4500u;
        Check(pal4::ActorManagerPlayerPropertyAccess::OwnedChild(
                  kActor, runtime) == 0x4500u &&
                  runtime.trace == std::vector<std::string>{"read:5412"},
              "owned-child getter reads player slot");

        runtime.trace.clear();
        runtime.words[kActor + 800u] = 0x2200u;
        Check(pal4::ActorManagerPlayerPropertyAccess::ForwardProperty(
                  kActor, 4u, 0x77u, runtime) == runtime.object_result &&
                  runtime.trace == std::vector<std::string>{
                      "actor52", "read:4896", "object168"},
              "mode four refreshes object before virtual dispatch");

        runtime.trace.clear();
        Check(pal4::ActorManagerPlayerPropertyAccess::ForwardProperty(
                  kActor, 3u, 0x77u, runtime) == runtime.fallback_result &&
                  runtime.trace ==
                      std::vector<std::string>{"fallback168"},
              "other property forwards directly");

        for (std::uint32_t index = 0; index < 10u; ++index) {
            runtime.words[kActor + 204u + index * 4u] = 100u + index;
            runtime.trace.clear();
            Check(pal4::ActorManagerPlayerPropertyAccess::IndexedValue(
                      kActor, index, runtime) == 100u + index &&
                      runtime.trace == std::vector<std::string>{
                          "read:" + std::to_string(kActor + 204u +
                                                    index * 4u)},
                  "indexed getter covers all ten words");
        }
        runtime.trace.clear();
        Check(pal4::ActorManagerPlayerPropertyAccess::IndexedValue(
                  kActor, 10u, runtime) == 10u &&
                  pal4::ActorManagerPlayerPropertyAccess::IndexedValue(
                      kActor, 0xffffffffu, runtime) == 0xffffffffu &&
                  runtime.trace.empty(),
              "out-of-range index returns its unsigned input");
        std::cout << "actor_manager_player_property_access_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
