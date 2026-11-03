#include "pal4/pal_game_object_manager_visit.h"

#include <cmath>
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

struct Runtime final : pal4::IPalGameObjectManagerVisitRuntime {
    static constexpr std::uint32_t manager = 0x1000u;
    static constexpr std::uint32_t end = 0x2000u;
    std::vector<std::uint32_t> nodes{0x2010u, 0x2020u, 0x2030u};
    std::map<std::uint32_t, std::uint32_t> objects{
        {0x2010u, 0x3000u}, {0x2020u, 0x4000u},
        {0x2030u, 0x5000u}};
    std::map<std::uint32_t, std::string> names{
        {0x3000u, "Bell"}, {0x4000u, "Gate"},
        {0x5000u, ""}};
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, bool> selectable;
    std::map<std::uint32_t, float> point_distance;
    std::map<std::uint32_t, float> ray_distance;
    std::map<std::uint32_t, bool> point_valid;
    std::map<std::uint32_t, bool> ray_valid;
    std::map<std::uint32_t, std::uint32_t> material_count;
    std::map<std::uint32_t, std::int32_t> type;
    std::vector<std::string> calls;
    std::uint32_t point_calls = 0u;
    std::uint32_t ray_calls = 0u;

    std::uint32_t FirstNode(std::uint32_t address) override {
        Check(address == manager, "first-node manager");
        return nodes.empty() ? end : nodes.front();
    }
    std::uint32_t EndNode(std::uint32_t address) override {
        Check(address == manager, "end-node manager");
        return end;
    }
    std::uint32_t NextNode(std::uint32_t node) override {
        for (std::size_t index = 0; index < nodes.size(); ++index)
            if (nodes[index] == node)
                return index + 1u < nodes.size() ? nodes[index + 1u] : end;
        throw std::runtime_error("unknown tree node");
    }
    std::uint32_t NodeObject(std::uint32_t node) override {
        return objects.at(node);
    }
    std::string_view ObjectName(std::uint32_t object) override {
        return names.at(object);
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
        calls.emplace_back("write " + std::to_string(address) +
                           "=" + std::to_string(value));
    }
    std::uint32_t ClearObjectTree(std::uint32_t address) override {
        Check(address == manager, "clear-tree manager");
        calls.emplace_back("clear");
        nodes.clear();
        return end;
    }
    void ReleaseObjectResources(std::uint32_t object) override {
        calls.emplace_back("release " + std::to_string(object));
    }
    void DestroyObject(std::uint32_t object,
                       std::uint32_t mode) override {
        Check(mode == 1u, "deleting destructor mode");
        calls.emplace_back("destroy " + std::to_string(object));
    }
    void DeactivateObject(std::uint32_t object) override {
        calls.emplace_back("deactivate " + std::to_string(object));
    }
    void ActivateObject(std::uint32_t object) override {
        calls.emplace_back("activate " + std::to_string(object));
    }
    void UpdateObject(std::uint32_t object,
                      std::uint32_t argument) override {
        Check(argument == 0xaau, "first update argument");
        calls.emplace_back("update1 " + std::to_string(object));
    }
    void UpdateObjectSecond(std::uint32_t object,
                            std::uint32_t argument) override {
        Check(argument == 0xbbu, "second update argument");
        calls.emplace_back("update2 " + std::to_string(object));
    }
    bool IsSelectable(std::uint32_t object) override {
        calls.emplace_back("selectable " + std::to_string(object));
        return selectable[object];
    }
    bool TryPointDistance(std::uint32_t object,
                          const pal4::GameObjectPoint& point,
                          float& distance) override {
        Check(point.x == 1.0f && point.y == 2.0f && point.z == 3.0f,
              "point forwarded");
        ++point_calls;
        distance = point_distance[object];
        return point_valid[object];
    }
    std::uint32_t MaterialCount(std::uint32_t clump) override {
        calls.emplace_back("materials " + std::to_string(clump));
        return material_count[clump];
    }
    std::int32_t ObjectType(std::uint32_t object) override {
        calls.emplace_back("type " + std::to_string(object));
        return type[object];
    }
    bool TryRayDistance(std::uint32_t object,
                        std::uint32_t first, std::uint32_t second,
                        float& distance) override {
        Check(first == 0xaaaau && second == 0xbbbbu,
              "ray arguments forwarded");
        ++ray_calls;
        distance = ray_distance[object];
        return ray_valid[object];
    }
};

void CheckDeactivateAndVisit() {
    Runtime runtime;
    Check(pal4::PalGameObjectManagerVisit::DeactivateAndClear(
              Runtime::manager, runtime) == Runtime::end &&
          runtime.nodes.empty() &&
          runtime.calls == (std::vector<std::string>{
              "release 12288", "destroy 12288",
              "release 16384", "destroy 16384",
              "release 20480", "destroy 20480", "clear"}),
          "resource release, deleting destructor and tree clear order");
    runtime.calls.clear();
    Check(pal4::PalGameObjectManagerVisit::DeactivateAndClear(
              Runtime::manager, runtime) == Runtime::end &&
          runtime.calls == (std::vector<std::string>{"clear"}),
          "empty tree clear");

    Runtime visitor_runtime;
    std::vector<std::uint32_t> visited;
    const auto stop = [&visited](std::uint32_t object,
                                 std::uint32_t context) {
        Check(context == 0x99u, "visitor context");
        visited.push_back(object);
        return object == 0x4000u ? 0u : 1u;
    };
    Check(pal4::PalGameObjectManagerVisit::VisitUntilFalse(
              Runtime::manager, stop, 0x99u, visitor_runtime) == 0u &&
          visited == (std::vector<std::uint32_t>{0x3000u, 0x4000u}),
          "visitor stops on false callback");
    visited.clear();
    Check(pal4::PalGameObjectManagerVisit::VisitUntilFalse(
              Runtime::manager,
              [&visited](std::uint32_t object, std::uint32_t) {
                  visited.push_back(object);
                  return 1u;
              }, 0u, visitor_runtime) == Runtime::end &&
          visited.size() == 3u, "visitor reaches tree end");
}

void CheckUpdates() {
    Runtime runtime;
    runtime.words[0x3000u + 0x184u] = 1u;
    runtime.words[0x4000u + 0x184u] = 0u;
    runtime.words[0x5000u + 0x184u] = 2u;
    Check(pal4::PalGameObjectManagerVisit::UpdateEnabled(
              Runtime::manager, 0xaau, runtime) == Runtime::end &&
          runtime.calls == (std::vector<std::string>{
              "update1 12288", "update1 20480"}),
          "first virtual update gate");
    runtime.calls.clear();
    Check(pal4::PalGameObjectManagerVisit::UpdateEnabledSecond(
              Runtime::manager, 0xbbu, runtime) == Runtime::end &&
          runtime.calls == (std::vector<std::string>{
              "update2 12288", "update2 20480"}),
          "second virtual update gate");
    runtime.nodes.clear();
    runtime.calls.clear();
    Check(pal4::PalGameObjectManagerVisit::UpdateEnabled(
              Runtime::manager, 0xaau, runtime) == Runtime::end &&
          runtime.calls.empty(), "empty update traversal");
}

void CheckClosestPoint() {
    Runtime runtime;
    for (const auto object : {0x3000u, 0x4000u, 0x5000u}) {
        runtime.words[object + 0x184u] = 1u;
        runtime.words[object + 0x1c8u] = 7u;
        runtime.selectable[object] = true;
        runtime.point_valid[object] = true;
    }
    runtime.point_distance = {{0x3000u, 4.0f}, {0x4000u, 2.0f},
                              {0x5000u, 2.0f}};
    const pal4::GameObjectPoint point{1.0f, 2.0f, 3.0f};
    Check(pal4::PalGameObjectManagerVisit::FindClosestPoint(
              Runtime::manager, point, false, true, runtime) == 0x4000u &&
          runtime.point_calls == 3u &&
          runtime.words[0x3000u + 0x1c8u] == 0u &&
          runtime.words[0x4000u + 0x1c8u] == 1u &&
          runtime.words[0x5000u + 0x1c8u] == 0u &&
          runtime.calls.back() == "write 16840=1",
          "closest point chooses first tie and marks only winner");

    runtime.calls.clear();
    runtime.point_calls = 0u;
    runtime.words[0x4000u + 0x184u] = 0u;
    runtime.selectable[0x5000u] = false;
    runtime.point_valid[0x3000u] = false;
    Check(pal4::PalGameObjectManagerVisit::FindClosestPoint(
              Runtime::manager, point, false, false, runtime) == 0u &&
          runtime.point_calls == 1u &&
          runtime.words[0x4000u + 0x1c8u] == 0u,
          "point gates and failed distance");
    runtime.point_valid[0x3000u] = true;
    runtime.point_distance[0x3000u] = std::nanf("");
    runtime.point_distance[0x5000u] = 1.0f;
    runtime.point_calls = 0u;
    Check(pal4::PalGameObjectManagerVisit::FindClosestPoint(
              Runtime::manager, point, true, false, runtime) == 0x5000u &&
          runtime.point_calls == 3u &&
          runtime.words[0x5000u + 0x1c8u] == 0u,
          "bypass gate, NaN and unmarked winner");
}

void CheckClosestRay() {
    Runtime runtime;
    for (const auto object : {0x3000u, 0x4000u, 0x5000u}) {
        runtime.words[object + 0x14u] = object + 0x100u;
        runtime.words[object + 0x184u] = 1u;
        runtime.words[object + 0x1c8u] = 7u;
        runtime.material_count[object + 0x100u] = 1u;
        runtime.type[object] = 2;
        runtime.selectable[object] = true;
        runtime.ray_valid[object] = true;
    }
    runtime.material_count[0x3100u] = 0u;
    runtime.type[0x4000u] = 3;
    runtime.ray_distance = {{0x3000u, 0.5f}, {0x4000u, 1.0f},
                            {0x5000u, 2.0f}};
    Check(pal4::PalGameObjectManagerVisit::FindClosestRay(
              Runtime::manager, 0xaaaau, 0xbbbbu, true, 2,
              runtime) == 0x5000u && runtime.ray_calls == 1u &&
          runtime.words[0x5000u + 0x1c8u] == 1u &&
          runtime.words[0x3000u + 0x1c8u] == 0u,
          "ray material and type gates");
    runtime.ray_calls = 0u;
    runtime.words[0x5000u + 0x184u] = 0u;
    Check(pal4::PalGameObjectManagerVisit::FindClosestRay(
              Runtime::manager, 0xaaaau, 0xbbbbu, false, -1,
              runtime) == 0x4000u && runtime.ray_calls == 1u &&
          runtime.words[0x4000u + 0x1c8u] == 0u,
          "negative type bypass and no selection mark");
    runtime.ray_valid[0x4000u] = false;
    Check(pal4::PalGameObjectManagerVisit::FindClosestRay(
              Runtime::manager, 0xaaaau, 0xbbbbu, true, -1,
              runtime) == 0u, "ray distance failure has no winner");
}

void CheckFindByName() {
    Runtime runtime;
    Check(pal4::PalGameObjectManagerVisit::FindByName(
              Runtime::manager, "Gate", runtime) == 0x4000u,
          "exact object name lookup");
    Check(pal4::PalGameObjectManagerVisit::FindByName(
              Runtime::manager, "gate", runtime) == 0u,
          "name comparison is case sensitive");
    Check(pal4::PalGameObjectManagerVisit::FindByName(
              Runtime::manager, "", runtime) == 0x5000u,
          "empty object name lookup");
    Check(pal4::PalGameObjectManagerVisit::FindByName(
              Runtime::manager, "Missing", runtime) == 0u,
          "missing name lookup");
}

void CheckBroadcast() {
    Runtime runtime;
    Check(pal4::PalGameObjectManagerVisit::DeactivateAll(
              Runtime::manager, runtime) == Runtime::end &&
          runtime.calls == (std::vector<std::string>{
              "deactivate 12288", "deactivate 16384",
              "deactivate 20480"}),
          "deactivation virtual dispatch in tree order");
    runtime.calls.clear();
    Check(pal4::PalGameObjectManagerVisit::ActivateAll(
              Runtime::manager, runtime) == Runtime::end &&
          runtime.calls == (std::vector<std::string>{
              "activate 12288", "activate 16384", "activate 20480"}),
          "activation virtual dispatch in tree order");
    runtime.nodes.clear();
    runtime.calls.clear();
    Check(pal4::PalGameObjectManagerVisit::ActivateAll(
              Runtime::manager, runtime) == Runtime::end &&
          runtime.calls.empty(), "empty broadcast tree");
}

}  // namespace

int main() {
    try {
        CheckDeactivateAndVisit();
        CheckUpdates();
        CheckClosestPoint();
        CheckClosestRay();
        CheckFindByName();
        CheckBroadcast();
        std::cout << "pal_game_object_manager_visit_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
