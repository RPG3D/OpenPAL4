#include "pal4/collision_tree_storage.h"

#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::ICollisionTreePointProjectionRuntime {
    std::vector<std::string> events;
    std::uint32_t selected_face = 0x2000u;
    bool accepted = true;
    pal4::D3dTriangleVector3 last_select{};
    pal4::D3dTriangleVector3 last_check{};

    std::uint32_t SelectFace(
        const std::uint32_t tree,
        const pal4::D3dTriangleVector3& point) override {
        events.push_back("select:" + std::to_string(tree));
        last_select = point;
        return selected_face;
    }
    pal4::D3dTriangleVector3 Project(
        const std::uint32_t tree, const std::uint32_t face,
        const pal4::D3dTriangleVector3& point) override {
        events.push_back("project:" + std::to_string(tree) + ":" +
                         std::to_string(face));
        return {point.x + 1.0f, point.y + 2.0f, point.z + 3.0f};
    }
    bool IsOutsideEdges(
        const std::uint32_t face,
        const pal4::D3dTriangleVector3& point) override {
        events.push_back("edges:" + std::to_string(face));
        last_check = point;
        return accepted;
    }
    void ClampPoint(const std::uint32_t face,
                    pal4::D3dTriangleVector3& point) override {
        events.push_back("clamp:" + std::to_string(face));
        point.x = 9.0f;
        point.z = 11.0f;
    }
    void ProjectHeight(const std::uint32_t face,
                       pal4::D3dTriangleVector3& point) override {
        events.push_back("height:" + std::to_string(face));
        point.y = 7.0f;
    }
};

void TestAcceptedFace() {
    Runtime runtime;
    pal4::D3dTriangleVector3 point{1.0f, 2.0f, 3.0f};
    Check(pal4::CollisionTreeStorage::SnapPointToFace(
              0x1000u, point, runtime) == 0x2000u &&
              point.x == 2.0f && point.y == 7.0f && point.z == 6.0f &&
              runtime.last_select.x == 1.0f &&
              runtime.last_check.x == 2.0f &&
              runtime.last_check.y == 4.0f &&
              runtime.events == std::vector<std::string>{
                  "select:4096", "project:4096:8192", "edges:8192",
                  "height:8192"},
          "accepted face projects point and height without clamping");
}

void TestClampAndNullFace() {
    Runtime runtime;
    runtime.accepted = false;
    pal4::D3dTriangleVector3 point{1.0f, 2.0f, 3.0f};
    Check(pal4::CollisionTreeStorage::SnapPointToFace(
              0x1000u, point, runtime) == 0x2000u &&
              point.x == 9.0f && point.y == 7.0f && point.z == 11.0f &&
              runtime.events == std::vector<std::string>{
                  "select:4096", "project:4096:8192", "edges:8192",
                  "clamp:8192", "height:8192"},
          "failed edge test clamps before final height projection");
    runtime.events.clear();
    runtime.selected_face = 0u;
    runtime.accepted = true;
    Check(pal4::CollisionTreeStorage::SnapPointToFace(
              0x1000u, point, runtime) == 0u &&
              runtime.events == std::vector<std::string>{
                  "select:4096", "project:4096:0", "edges:0", "height:0"},
          "native call sequence continues even when face selection is null");
}

}  // namespace

int main() {
    TestAcceptedFace();
    TestClampAndNullFace();
}
