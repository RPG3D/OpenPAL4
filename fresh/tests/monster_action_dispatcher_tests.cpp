#include "pal4/monster_action_dispatcher.h"
#include "pal4/monster_behavior_manager.h"

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

struct Runtime final : pal4::IMonsterActionDispatchRuntime {
    std::vector<std::string> calls;
    std::uint32_t instance{0x8000u};
    std::uint32_t result{0x1234u};

    std::uint32_t ManagerInstance() override {
        calls.emplace_back("instance");
        return instance;
    }
    std::uint32_t IterateAndCallback(std::uint32_t manager,
        std::uint32_t callback, std::uint32_t context) override {
        Check(manager == instance &&
                  callback == pal4::MonsterActionDispatcher::
                      kObjectInteractionCallback &&
                  context == 0x5000u,
              "callback identity and context");
        calls.emplace_back("iterate");
        return result;
    }
};

}  // namespace

int main() {
    Runtime runtime;
    Check(pal4::MonsterActionDispatcher::Process(
              0x5000u, runtime) == 0x1234u &&
              runtime.calls == std::vector<std::string>{
                  "instance", "iterate"},
          "dispatcher forwards singleton, callback and return value");
    runtime.calls.clear();
    runtime.instance = 0u;
    runtime.result = 0u;
    Check(pal4::MonsterActionDispatcher::Process(
              0x5000u, runtime) == 0u &&
              runtime.calls.size() == 2u,
          "dispatcher preserves zero singleton and return values");
    pal4::MonsterActionDispatchRuntimeStub stub;
    Check(pal4::MonsterActionDispatcher::Process(
              0x5000u, stub) == 0u,
          "singleton and iteration have a linkable stub");
    std::cout << "monster action dispatcher cases passed\n";
}
