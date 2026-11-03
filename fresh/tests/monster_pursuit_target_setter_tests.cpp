#include "pal4/monster_behavior_manager.h"

#include <cstdlib>
#include <iostream>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

void WriteOnlyDuringActivePursuit() {
    pal4::MonsterBehaviorManagerState state{};
    state.words[0x6e4u / 4u] = 0x1234u;
    state.words[0x6e0u / 4u] = 1u;
    Check(pal4::MonsterBehaviorManager::SetPursuitTargetIfActive(
              state, 0x8000u) == 0u &&
              state.words[0x6e4u / 4u] == 0x1234u,
          "inactive manager returns active field and preserves target");

    state.words[0x1f0u / 4u] = 0x9000u;
    Check(pal4::MonsterBehaviorManager::SetPursuitTargetIfActive(
              state, 0u) == 0u &&
              state.words[0x6e4u / 4u] == 0x1234u,
          "zero target is returned without assignment");
    for (const auto mode : {0u, 2u, 3u}) {
        state.words[0x6e0u / 4u] = mode;
        Check(pal4::MonsterBehaviorManager::SetPursuitTargetIfActive(
                  state, 0x8000u) == 0x8000u &&
                  state.words[0x6e4u / 4u] == 0x1234u,
              "other states return argument but preserve target");
    }
    state.words[0x6e0u / 4u] = 1u;
    Check(pal4::MonsterBehaviorManager::SetPursuitTargetIfActive(
              state, 0x8000u) == 0x8000u &&
              state.words[0x6e4u / 4u] == 0x8000u &&
              state.words[0x1f0u / 4u] == 0x9000u,
          "nonzero target updates only in active pursuit state");
}

}  // namespace

int main() {
    WriteOnlyDuringActivePursuit();
    std::cout << "monster pursuit target setter cases passed\n";
}
