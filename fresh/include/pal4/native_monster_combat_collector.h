#pragma once
#include <cstdint>
#include <memory>

namespace pal4 {
class NativeMonsterManagerOwner;
class NativeBaseObjectMemory;
class IMonsterProximityRuntime;

// Native434720/434810 composition. The owned initiating monster receives the
// original ordered records/names; shared callbacks retain all candidate gates.
class NativeMonsterCombatCollector final {
public:
    NativeMonsterCombatCollector(NativeMonsterManagerOwner& manager,
        NativeBaseObjectMemory& memory, IMonsterProximityRuntime& geometry);
    ~NativeMonsterCombatCollector();
    NativeMonsterCombatCollector(const NativeMonsterCombatCollector&) = delete;
    NativeMonsterCombatCollector& operator=(const NativeMonsterCombatCollector&) = delete;
    std::uint32_t Collect(std::uint32_t monster);
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};
}
