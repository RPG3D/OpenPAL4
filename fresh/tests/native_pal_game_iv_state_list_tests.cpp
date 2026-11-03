#include "pal4/native_pal_game_iv_state_list.h"
#include "pal4/native_pal_game_iv_texture_storage.h"
#include "pal4/native_world_loading_state_owner.h"
#include "pal4/native_world_state_owner.h"

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

std::uint32_t ReadWord(std::uint32_t address) {
    return *reinterpret_cast<const std::uint32_t*>(
        static_cast<std::uintptr_t>(address));
}

}  // namespace

int main() {
    pal4::NativePalGameIVTextureStorage game([](std::uint32_t) {});
    const auto game_address = game.Construct();
    pal4::NativeWorldLoadingStateOwner loading;
    const auto loading_address = loading.Construct();
    pal4::NativeWorldStateOwner scene;
    const auto scene_address = scene.Construct();
    Check(ReadWord(scene_address) == pal4::WorldState::kVtable,
          "native Scene owner reuses the WorldState constructor");
    std::vector<std::uint32_t> destroyed;
    {
        pal4::NativePalGameIVStateList list(game_address);
        Check(list.Register(loading_address, 6u, [&] {
                  destroyed.push_back(6u);
                  loading.Destroy();
              }) &&
                  list.Find(6u) == loading_address &&
                  ReadWord(game_address + 0x188u) != 0u &&
                  list.OutstandingAllocations() == 1u,
              "PALGameIV owns a native linked entry for Loading state 6");
        Check(!list.Register(scene_address, 6u, [&] {
                  destroyed.push_back(99u);
              }) &&
                  list.Count() == 1u &&
                  list.OutstandingAllocations() == 1u,
              "duplicate virtual state ID does not add or destroy an entry");
        Check(list.Register(scene_address, 1u, [&] {
                  destroyed.push_back(1u);
                  scene.Destroy();
              }) &&
                  list.Find(1u) == scene_address &&
                  list.Find(0u) == 0u &&
                  list.Count() == 2u &&
                  list.OutstandingAllocations() == 2u,
              "state lookup uses the registered virtual IDs");
        list.Clear();
        Check(destroyed == std::vector<std::uint32_t>{6u, 1u} &&
                  loading.Address() == 0u &&
                  scene.Address() == 0u &&
                  loading.OutstandingAllocations() == 0u &&
                  list.Count() == 0u &&
                  list.OutstandingAllocations() == 0u &&
                  ReadWord(game_address + 0x188u) == 0u,
              "PALGameIV clears state nodes and invokes owned destructors");
    }
    game.Destroy();
    return EXIT_SUCCESS;
}
