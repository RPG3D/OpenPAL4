#include "pal4/script_random_player_position.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

struct Runtime final : pal4::IScriptRandomPlayerPositionRuntime {
    std::uint32_t manager = 0x1000u;
    std::uint32_t player = 0x4000u;
    std::uint32_t current_index = 2u;
    std::uint32_t object_type = 1u;
    std::vector<std::uint32_t> collision_results{0u};
    std::vector<bool> movement_results{true};
    std::uint32_t collision_checks = 0u;
    std::uint32_t movement_checks = 0u;
    std::uint32_t random_components = 0u;
    std::uint32_t random_radii = 0u;
    std::vector<std::string> calls;

    std::uint32_t PlayerManager() override {
        calls.emplace_back("manager"); return manager;
    }
    std::uint32_t PlayerByIndex(std::uint32_t resolved_manager,
                                std::int32_t index) override {
        Check(resolved_manager == manager && index == -2,
              "signed indexed player lookup");
        calls.emplace_back("player"); return player;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        if (address == manager + 0x2c50u) {
            calls.emplace_back("current index"); return current_index;
        }
        if (address == 0x8004u) {
            calls.emplace_back("object type"); return object_type;
        }
        Check(address == 0x8044u, "collidable clump pointer offset");
        calls.emplace_back("clump"); return 0x9000u;
    }
    float ReadFloat(std::uint32_t address) override {
        Check(address >= player + 0xacu && address <= player + 0xb4u &&
                  (address - player - 0xacu) % 4u == 0u,
              "player XYZ offsets");
        calls.emplace_back("position");
        return std::array<float, 3>{100.0f, 50.0f, 200.0f}[
            (address - player - 0xacu) / 4u];
    }
    double RandomComponent(float zero) override {
        Check(zero == 0.0f, "random component seed zero");
        calls.emplace_back("random component");
        return random_components++ % 3u == 0u ? 1.0 : 0.0;
    }
    double RandomRadius(float minimum, float maximum,
                        float zero) override {
        Check(minimum == 5.0f && maximum == 20.0f && zero == 0.0f,
              "random radius bounds and third zero");
        calls.emplace_back("radius");
        ++random_radii;
        return 10.0;
    }
    std::uint32_t CollisionWorld() override {
        calls.emplace_back("collision world"); return 0x7000u;
    }
    void ForEachCollisionObject(std::uint32_t world,
                                pal4::ScriptCollisionProbe& probe) override {
        Check(world == 0x7000u, "collision world forwarding");
        calls.emplace_back("foreach");
        (void)pal4::ScriptRandomPlayerPosition::CheckCollision(
            0x8000u, probe, *this);
    }
    std::uint32_t ProcessClumpCollision(
        const std::array<float, 6>& segment, std::uint32_t clump,
        std::array<std::uint8_t, 24>& scratch) override {
        Check(segment == std::array<float, 6>{
                  110.0f, 150.0f, 200.0f, 110.0f, -50.0f, 200.0f} &&
                  clump == 0x9000u && scratch.size() == 24u,
              "vertical collision segment around candidate");
        calls.emplace_back("collision");
        const auto index = collision_checks++;
        return collision_results[std::min<std::size_t>(
            index, collision_results.size() - 1u)];
    }
    double ObjectProperty(std::uint32_t resolved_player) override {
        Check(resolved_player == player, "player object property source");
        calls.emplace_back("property"); return 10.0;
    }
    std::uint32_t MovementAdapter() override {
        calls.emplace_back("adapter"); return 0xa000u;
    }
    bool CheckMovement(std::uint32_t adapter,
                       std::array<float, 3>& candidate,
                       float width, float height) override {
        Check(adapter == 0xa000u && candidate ==
                  std::array<float, 3>{110.0f, 50.0f, 200.0f} &&
                  width == 3.5f && std::fabs(height - 25.3f) < 0.0001f,
              "movement candidate and scaled player dimensions");
        calls.emplace_back("check movement");
        const auto index = movement_checks++;
        return movement_results[std::min<std::size_t>(
            index, movement_results.size() - 1u)];
    }
    void SetTransform(std::uint32_t resolved_player,
                      const std::array<float, 3>& position,
                      std::uint32_t flag) override {
        Check(resolved_player == player && position ==
                  std::array<float, 3>{110.0f, 50.0f, 200.0f} &&
                  flag == 0u, "successful player transform");
        calls.emplace_back("transform");
    }
    std::uint32_t MinimapManager() override {
        calls.emplace_back("minimap"); return 0xb000u;
    }
    std::uint32_t UpdateGrid(std::uint32_t minimap,
                             std::uint32_t position,
                             std::uint32_t orientation) override {
        Check(minimap == 0xb000u && position == player + 0xacu &&
                  orientation == player + 0xa0u,
              "minimap update offsets");
        calls.emplace_back("grid"); return 0x1234u;
    }
};

void CollisionCallback() {
    Runtime sentinel;
    pal4::ScriptCollisionProbe probe;
    probe.segment = {110.0f, 150.0f, 200.0f,
                     110.0f, -50.0f, 200.0f};
    Check(pal4::ScriptRandomPlayerPosition::CheckCollision(
                  0xfffffffcu, probe, sentinel) == 0xfffffffcu &&
              sentinel.calls.empty(),
          "minus-four object guard avoids dereference");
    Runtime other_type;
    other_type.object_type = 2u;
    Check(pal4::ScriptRandomPlayerPosition::CheckCollision(
                  0x8000u, probe, other_type) == 0x8000u &&
              other_type.calls == std::vector<std::string>{"object type"},
          "non-type-one object skips clump test");
    Runtime hit;
    hit.collision_results = {1u};
    Check(pal4::ScriptRandomPlayerPosition::CheckCollision(
                  0x8000u, probe, hit) == 0u && probe.blocked == 1u,
          "collision sets probe flag and stops callback");
    probe.blocked = 0u;
    Runtime miss;
    Check(pal4::ScriptRandomPlayerPosition::CheckCollision(
                  0x8000u, probe, miss) == 0x8000u &&
              probe.blocked == 0u,
          "miss returns original object without flag write");
}

void SelectionAndImmediateSuccess() {
    Runtime missing;
    missing.player = 0u;
    Check(pal4::ScriptRandomPlayerPosition::MoveIndexed(
                  -2, 5.0f, 20.0f, missing) == 0u &&
              missing.calls == std::vector<std::string>{"manager", "player"},
          "missing indexed player exits before RNG");
    Runtime indexed;
    Check(pal4::ScriptRandomPlayerPosition::MoveIndexed(
                  -2, 5.0f, 20.0f, indexed) == 0x1234u &&
              indexed.random_radii == 1u && indexed.random_components == 3u &&
              indexed.collision_checks == 1u && indexed.movement_checks == 1u &&
              indexed.calls[indexed.calls.size() - 3u] == "transform" &&
              indexed.calls[indexed.calls.size() - 2u] == "minimap" &&
              indexed.calls.back() == "grid",
          "indexed player first candidate moves then updates minimap");
    Runtime current;
    current.player = current.manager + current.current_index * 0xb14u;
    Check(pal4::ScriptRandomPlayerPosition::MoveCurrent(
                  5.0f, 20.0f, current) == 0x1234u &&
              current.calls[0] == "manager" &&
              current.calls[1] == "current index",
          "current player selected by inline manager stride");
    Runtime zero_current;
    zero_current.manager = 0u;
    zero_current.current_index = 0u;
    Check(pal4::ScriptRandomPlayerPosition::MoveCurrent(
                  5.0f, 20.0f, zero_current) == 0u &&
              zero_current.calls == std::vector<std::string>{
                  "manager", "current index"},
          "zero computed current player exits before RNG");
}

void RetriesAndLimit() {
    Runtime blocked;
    blocked.collision_results = {1u, 0u};
    Check(pal4::ScriptRandomPlayerPosition::MoveIndexed(
                  -2, 5.0f, 20.0f, blocked) == 0x1234u &&
              blocked.random_radii == 2u && blocked.collision_checks == 2u &&
              blocked.movement_checks == 2u &&
              std::count(blocked.calls.begin(), blocked.calls.end(),
                         "property") == 2,
          "blocked first candidate rerolls without resampling dimensions");
    Runtime invalid_path;
    invalid_path.movement_results = {false, true};
    Check(pal4::ScriptRandomPlayerPosition::MoveIndexed(
                  -2, 5.0f, 20.0f, invalid_path) == 0x1234u &&
              invalid_path.random_radii == 2u &&
              invalid_path.movement_checks == 2u,
          "movement adapter failure rerolls candidate");
    Runtime exhausted;
    exhausted.movement_results = {false};
    Check(pal4::ScriptRandomPlayerPosition::MoveIndexed(
                  -2, 5.0f, 20.0f, exhausted) == 0u &&
              exhausted.random_radii == 100u &&
              exhausted.random_components == 300u &&
              exhausted.collision_checks == 100u &&
              exhausted.movement_checks == 100u &&
              std::count(exhausted.calls.begin(), exhausted.calls.end(),
                         "transform") == 0,
          "one hundred failed attempts return zero without moving player");
}

}  // namespace

int main() {
    try {
        CollisionCallback();
        SelectionAndImmediateSuccess();
        RetriesAndLimit();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
