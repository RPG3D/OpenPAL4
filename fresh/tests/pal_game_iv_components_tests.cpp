#include "pal4/pal_game_iv_components.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <unordered_map>
#include <vector>

namespace {

using Kind = pal4::PalGameIVComponentKind;

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IPalGameIVComponentsRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::unordered_map<std::uint32_t, std::uint32_t> ids;
    std::vector<std::uint32_t> allocation_sizes;
    std::vector<Kind> constructors;
    std::vector<std::uint32_t> registered;
    std::uint32_t next = 0x5000u;
    std::uint32_t fail_at = 0u;
    bool duplicate_bink = false;

    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
        const std::unordered_map<std::uint32_t, std::uint32_t> vtable_ids = {
            {0x849498u, 0u}, {0x849480u, 3u},
            {0x849468u, 5u}, {0x849450u, 4u},
            {0x849438u, 2u}, {0x849420u, 8u},
        };
        if (const auto found = vtable_ids.find(value);
            found != vtable_ids.end())
            ids[address] = found->second;
    }
    std::uint32_t Allocate(std::uint32_t bytes) override {
        allocation_sizes.push_back(bytes);
        if (allocation_sizes.size() == fail_at) return 0u;
        const auto object = next;
        next += 0x100u;
        return object;
    }
    std::uint32_t Free(std::uint32_t) override { return 0u; }
    std::uint32_t StateId(std::uint32_t state) override {
        return ids.at(state);
    }
    void DeleteState(std::uint32_t, std::uint32_t) override {}
    std::uint32_t EmptyCleanupResult() override { return 0u; }
    std::uint32_t ConstructComponent(Kind kind,
                                      std::uint32_t object) override {
        constructors.push_back(kind);
        switch (kind) {
            case Kind::World: ids[object] = 1u; break;
            case Kind::SceneNamedNode: break;
            case Kind::CombatXmlDocument: break;
            case Kind::WorldLoading: ids[object] = 6u; break;
            case Kind::GameOver: ids[object] = 7u; break;
            case Kind::Save: ids[object] = 9u; break;
            case Kind::ModalNamedNode: ids[object] = 10u; break;
            case Kind::WorldMap: ids[object] = 11u; break;
            case Kind::Bink: ids[object] = duplicate_bink ? 11u : 12u; break;
        }
        return object;
    }
};

std::vector<std::uint32_t> LinkedIds(Runtime& runtime) {
    std::vector<std::uint32_t> result;
    auto node = runtime.ReadWord(0x1188u);
    while (node != 0u) {
        result.push_back(runtime.StateId(runtime.ReadWord(node)));
        node = runtime.ReadWord(node + 4u);
    }
    return result;
}

void AllComponents() {
    Runtime runtime;
    Check(pal4::PalGameIVComponents::Initialize(0x1000u, runtime),
          "last state registered");
    Check(LinkedIds(runtime) == std::vector<std::uint32_t>({
              1u, 0u, 3u, 5u, 4u, 2u, 6u, 7u, 8u, 9u, 10u, 11u, 12u}),
          "IDA state order and virtual IDs");
    Check(runtime.allocation_sizes == std::vector<std::uint32_t>({
              4u, 8u, 16u, 8u, 72u, 8u, 4u, 8u, 4u, 8u,
              4u, 8u, 56u, 8u, 20u, 8u, 4u, 8u, 4u, 8u,
              8u, 8u, 4u, 8u, 8u, 8u}),
          "component and linked-node allocation sizes");
    Check(runtime.constructors == std::vector<Kind>({
              Kind::World, Kind::SceneNamedNode, Kind::CombatXmlDocument,
              Kind::WorldLoading, Kind::GameOver, Kind::Save,
              Kind::ModalNamedNode, Kind::WorldMap, Kind::Bink}),
          "constructor order");
    Check(runtime.ReadWord(0x5200u) == 0x849498u &&
              runtime.ReadWord(0x5400u) == 0x849480u &&
              runtime.ReadWord(0x5600u) == 0x849468u,
          "composite and direct vtables");
}

void DuplicateAndAllocationFailure() {
    Runtime duplicate;
    duplicate.duplicate_bink = true;
    Check(!pal4::PalGameIVComponents::Initialize(0x1000u, duplicate),
          "duplicate final state result");
    Check(LinkedIds(duplicate).size() == 12u,
          "duplicate final state not linked");
    Runtime failure;
    failure.fail_at = 5u;
    try {
        (void)pal4::PalGameIVComponents::Initialize(0x1000u, failure);
        Check(false, "allocation failure must throw");
    } catch (const std::bad_alloc&) {
        Check(LinkedIds(failure) == std::vector<std::uint32_t>({1u, 0u}) &&
                  failure.allocation_sizes.size() == 5u,
              "failure stops before third registration");
    }
}

}  // namespace

int main() {
    try {
        AllComponents();
        DuplicateAndAllocationFailure();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
