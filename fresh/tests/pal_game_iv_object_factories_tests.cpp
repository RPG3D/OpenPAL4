#include "pal4/pal_game_iv_object_factories.h"

#include <array>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IPalGameIVObjectFactoryRuntime {
    std::vector<std::string> order;
    std::vector<pal4::PalGameIVFactoryRegistration> event_entries;
    std::vector<pal4::PalGameIVFactoryRegistration> game_entries;
    std::vector<pal4::PalGameIVFactoryRegistration> storage_entries;
    std::uint32_t rejected_key = 0xffffffffu;

    std::uint32_t EventVolumeRegistry() override {
        order.push_back("event-get");
        return 0x1000u;
    }
    void RegisterEventVolume(
        std::uint32_t registry,
        pal4::PalGameIVFactoryRegistration entry) override {
        Check(registry == 0x1000u, "event registry handle");
        order.push_back("event-register");
        event_entries.push_back(entry);
    }
    std::uint32_t GameObjectRegistry() override {
        order.push_back("game-get");
        return 0x2000u;
    }
    void RegisterGameObject(
        std::uint32_t registry,
        pal4::PalGameIVFactoryRegistration entry) override {
        Check(registry == 0x2000u, "game registry handle");
        order.push_back("game-register");
        game_entries.push_back(entry);
    }
    std::uint32_t StorageObjectRegistry() override {
        order.push_back("storage-get");
        return 0x3000u;
    }
    bool RegisterStorageObject(
        std::uint32_t registry,
        pal4::PalGameIVFactoryRegistration entry) override {
        Check(registry == 0x3000u, "storage registry handle");
        order.push_back("storage-register");
        storage_entries.push_back(entry);
        return entry.key != rejected_key;
    }
};

template <std::size_t N>
void CheckEntries(const std::vector<pal4::PalGameIVFactoryRegistration>& actual,
                  const std::array<pal4::PalGameIVFactoryRegistration, N>& expected,
                  const char* message) {
    Check(actual.size() == expected.size(), message);
    for (std::size_t i = 0; i < expected.size(); ++i)
        Check(actual[i].key == expected[i].key &&
                  actual[i].factory == expected[i].factory, message);
}

void RegistrationTables() {
    Runtime runtime;
    Check(pal4::PalGameIVObjectFactories::RegisterAll(runtime),
          "final registration result");
    constexpr std::array<pal4::PalGameIVFactoryRegistration, 6> events = {{
        {0u, 0x5a9380u}, {1u, 0x5a9e80u}, {2u, 0x5a94c0u},
        {3u, 0x5a9420u}, {4u, 0x5a92f0u}, {5u, 0x5aa0f0u},
    }};
    constexpr std::array<pal4::PalGameIVFactoryRegistration, 11> game = {{
        {0u, 0x5a42e0u}, {1u, 0x59f380u}, {2u, 0x59d100u},
        {3u, 0x5a5f40u}, {4u, 0x59b780u}, {5u, 0x59fc00u},
        {6u, 0x59aa00u}, {7u, 0x59e6f0u}, {8u, 0x59c460u},
        {9u, 0x5a3cd0u}, {10u, 0x5a4c50u},
    }};
    constexpr std::array<pal4::PalGameIVFactoryRegistration, 13> storage = {{
        {6u, 0x59b1a0u}, {4u, 0x59bfc0u}, {8u, 0x59cc70u},
        {7u, 0x59edb0u}, {5u, 0x5a3160u}, {0u, 0x5a47d0u},
        {1u, 0x59f770u}, {2u, 0x59df90u}, {9u, 0x5a3fa0u},
        {10u, 0x5a5bf0u}, {3u, 0x5a6990u}, {11u, 0x432180u},
        {12u, 0x40bfb0u},
    }};
    CheckEntries(runtime.event_entries, events, "event factory table");
    CheckEntries(runtime.game_entries, game, "game object factory table");
    CheckEntries(runtime.storage_entries, storage, "storage factory table");
    Check(runtime.order.size() == 60u, "thirty getter/register pairs");
    for (std::size_t i = 0; i < 30u; ++i) {
        const auto group = i < 6u ? "event" : i < 17u ? "game" : "storage";
        Check(runtime.order[i * 2u] == std::string(group) + "-get" &&
                  runtime.order[i * 2u + 1u] ==
                      std::string(group) + "-register",
              "factory registration order");
    }
}

void LastResultForwarding() {
    Runtime runtime;
    runtime.rejected_key = 12u;
    Check(!pal4::PalGameIVObjectFactories::RegisterAll(runtime) &&
              runtime.storage_entries.size() == 13u,
          "last storage result returned");
    Runtime earlier_rejection;
    earlier_rejection.rejected_key = 6u;
    Check(pal4::PalGameIVObjectFactories::RegisterAll(earlier_rejection) &&
              earlier_rejection.storage_entries.size() == 13u,
          "earlier rejection does not stop later registrations");
}

}  // namespace

int main() {
    try {
        RegistrationTables();
        LastResultForwarding();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
