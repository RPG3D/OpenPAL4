#include "pal4/game_object_animation_event_array.h"

#include <cstdlib>
#include <iostream>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) { std::cerr << message << '\n'; std::exit(1); }
}

class Runtime final : public pal4::IGameObjectAnimationEventArrayRuntime {
public:
    std::vector<std::uint32_t> destroyed;
    void DestroyEvent(std::uint32_t address) override {
        destroyed.push_back(address);
    }
};

}  // namespace

int main() {
    Runtime runtime;
    pal4::GameObjectAnimationEventArray::CleanupRange(
        0x2000u, 0x2000u, runtime);
    Check(runtime.destroyed.empty(), "empty event range has no calls");
    pal4::GameObjectAnimationEventArray::CleanupRange(
        0x2000u, 0x2000u + 3u * 0x14cu, runtime);
    Check(runtime.destroyed == std::vector<std::uint32_t>{
              0x2000u, 0x214cu, 0x2298u},
          "cleanup visits each 332-byte event in address order");
    runtime.destroyed.clear();
    pal4::GameObjectAnimationEventArray::CleanupRange(
        0x214cu, 0x2000u, runtime);
    pal4::GameObjectAnimationEventArray::CleanupRange(
        0x2000u, 0x2001u, runtime);
    Check(runtime.destroyed.empty(), "invalid ranges terminate safely");
    pal4::GameObjectAnimationEventArrayRuntimeStub stub;
    pal4::GameObjectAnimationEventArray::CleanupRange(
        0x2000u, 0x214cu, stub);
    std::cout << "game_object_animation_event_array_tests passed\n";
}
