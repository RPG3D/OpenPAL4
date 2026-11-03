#include "pal4/collision_tree_storage.h"

#include <cstdlib>
#include <iostream>
#include <limits>
#include <map>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::ICollisionTreeAppendRuntime {
    std::map<std::uint32_t, std::uint32_t> slots;
    std::vector<std::string> events;
    std::vector<std::uint32_t> allocations;
    std::size_t next_allocation = 0;
    std::uint32_t constructed_face = 0;
    pal4::D3dTriangleVector3 constructed_vertices[3]{};

    std::uint32_t ReadWord(const std::uint32_t address) override {
        events.push_back("read:" + std::to_string(address));
        return slots.at(address);
    }
    void DestroyItem(const std::uint32_t address) override {
        events.push_back("destroy:" + std::to_string(address));
    }
    std::uint32_t Free(const std::uint32_t address) override {
        events.push_back("free:" + std::to_string(address));
        return address + 1u;
    }
    std::uint32_t Allocate(const std::uint32_t size) override {
        events.push_back("allocate:" + std::to_string(size));
        return allocations.at(next_allocation++);
    }
    void WriteWord(const std::uint32_t address,
                   const std::uint32_t value) override {
        events.push_back("write:" + std::to_string(address));
        slots[address] = value;
    }
    void ConstructFace(const std::uint32_t address,
                       const pal4::D3dTriangleVector3& first,
                       const pal4::D3dTriangleVector3& second,
                       const pal4::D3dTriangleVector3& third) override {
        events.push_back("construct:" + std::to_string(address));
        constructed_face = address;
        constructed_vertices[0] = first;
        constructed_vertices[1] = second;
        constructed_vertices[2] = third;
    }
};

void TestInitialization() {
    pal4::CollisionTreeStorageState state{};
    state.words.fill(0xa5a5a5a5u);
    Check(&pal4::CollisionTreeStorage::Initialize(state, 0x6bu) == &state,
          "initializer returns storage address");
    Check(state.words[0] == 0xa5a5a56bu &&
              state.words[7] == 0xa5a5a56bu,
          "initializer changes only low flag bytes");
    for (const auto index : {1u, 2u, 3u, 4u, 5u, 8u, 9u, 10u}) {
        Check(state.words[index] == 0u,
              "initializer clears both vector triplets and scalar fields");
    }
    Check(state.words[6] == 0xa5a5a5a5u &&
              state.words[11] == 0xa5a5a5a5u &&
              state.words[14] == 0xa5a5a5a5u &&
              state.words[15] == 100u,
          "initializer preserves untouched words and sets capacity 100");
    Check(pal4::CollisionTreeStorage::SetDebugRenderer(state, 0x12345678u) ==
              0x12345678u && state.words[4] == 0x12345678u &&
              state.words[5] == 0u,
          "debug renderer setter writes only the callback word");
}

void TestClearAndRelease() {
    pal4::CollisionTreeStorageState state{};
    pal4::CollisionTreeStorage::Initialize(state, 1u);
    state.words[1] = 0x1000u;
    state.words[2] = 0x100cu;
    state.words[3] = 0x1010u;
    Runtime runtime;
    runtime.slots = {{0x1000u, 0x2000u},
                     {0x1004u, 0u},
                     {0x1008u, 0x3000u}};
    Check(pal4::CollisionTreeStorage::ClearItems(state, runtime) ==
              0x100cu && state.words[1] == 0x1000u &&
              state.words[2] == 0x1000u && state.words[3] == 0x1010u,
          "clear returns old end and retains allocated buffer");
    Check(runtime.events == std::vector<std::string>{
              "read:4096", "destroy:8192", "free:8192",
              "read:4100", "read:4104", "destroy:12288", "free:12288"},
          "clear destroys and frees only nonnull items in entry order");
    runtime.events.clear();
    Check(pal4::CollisionTreeStorage::ClearItems(state, runtime) ==
              0x1000u && runtime.events.empty(),
          "empty item range makes no runtime calls");

    state.words[2] = 0x1004u;
    state.words[8] = 0x4000u;
    state.words[9] = 0x4008u;
    state.words[10] = 0x4010u;
    state.words[4] = 0xfeedu;
    Check(pal4::CollisionTreeStorage::Release(state, runtime) ==
              0x1001u &&
              runtime.events == std::vector<std::string>{
                  "read:4096", "destroy:8192", "free:8192",
                  "free:16384", "free:4096"},
          "release clears items then second buffer then first buffer");
    Check(state.words[1] == 0u && state.words[2] == 0u &&
              state.words[3] == 0u && state.words[8] == 0u &&
              state.words[9] == 0u && state.words[10] == 0u &&
              state.words[4] == 0xfeedu && state.words[15] == 100u,
          "release zeroes vector triplets and preserves unrelated fields");
    runtime.events.clear();
    Check(pal4::CollisionTreeStorage::Release(state, runtime) == 1u &&
              runtime.events == std::vector<std::string>{"free:0", "free:0"},
          "release forwards null buffer addresses on repeat calls");
}

void TestAppendTriangle() {
    pal4::CollisionTreeStorageState state{};
    pal4::CollisionTreeStorage::Initialize(state, 0u);
    Runtime runtime;
    runtime.allocations = {0x5000u, 0x6000u, 0x5100u, 0x6100u,
                           0x5200u, 0x6200u, 0x5300u};
    const pal4::D3dTriangleVector3 first{1.0f, 2.0f, 3.0f};
    const pal4::D3dTriangleVector3 second{4.0f, 5.0f, 6.0f};
    const pal4::D3dTriangleVector3 third{7.0f, 8.0f, 9.0f};
    Check(pal4::CollisionTreeStorage::AppendTriangle(
              state, first, second, third, runtime) == 0u &&
              state.words[1] == 0x6000u && state.words[2] == 0x6004u &&
              state.words[3] == 0x6004u &&
              runtime.slots.at(0x6000u) == 0x5000u,
          "first append allocates face and one-slot pointer buffer");
    Check(runtime.events == std::vector<std::string>{
              "allocate:240", "construct:20480", "allocate:4",
              "write:24576", "free:0"} &&
              runtime.constructed_vertices[0].x == 1.0f &&
              runtime.constructed_vertices[1].y == 5.0f &&
              runtime.constructed_vertices[2].z == 9.0f,
          "face construction precedes vector growth and keeps vertex order");

    runtime.events.clear();
    Check(pal4::CollisionTreeStorage::AppendTriangle(
              state, first, second, third, runtime) == 1u &&
              state.words[1] == 0x6100u && state.words[2] == 0x6108u &&
              state.words[3] == 0x6108u &&
              runtime.slots.at(0x6100u) == 0x5000u &&
              runtime.slots.at(0x6104u) == 0x5100u,
          "second append grows one-slot buffer to two slots");
    Check(runtime.events == std::vector<std::string>{
              "allocate:240", "construct:20736", "allocate:8",
              "read:24576", "write:24832", "write:24836", "free:24576"},
          "growth copies old slot, appends new face, then frees old buffer");

    Check(pal4::CollisionTreeStorage::AppendTriangle(
              state, first, second, third, runtime) == 2u &&
              state.words[1] == 0x6200u && state.words[2] == 0x620cu &&
              state.words[3] == 0x6210u &&
              runtime.slots.at(0x6200u) == 0x5000u &&
              runtime.slots.at(0x6204u) == 0x5100u &&
              runtime.slots.at(0x6208u) == 0x5200u,
          "growth doubles capacity once size exceeds one");
    runtime.events.clear();
    Check(pal4::CollisionTreeStorage::AppendTriangle(
              state, first, second, third, runtime) == 0x6210u &&
              state.words[2] == 0x6210u && state.words[3] == 0x6210u &&
              runtime.slots.at(0x620cu) == 0x5300u &&
              runtime.events == std::vector<std::string>{
                  "allocate:240", "construct:21248", "write:25100"},
          "available capacity appends in place and returns new end address");
}

struct QueryRuntime final : pal4::ICollisionTreeFaceQueryRuntime {
    struct Face final {
        bool direct = false;
        float height = 0.0f;
        pal4::D3dTriangleVector3 centroid{};
        std::uint32_t trace_status = 0u;
        pal4::D3dTriangleVector2 intersection{};
    };
    std::map<std::uint32_t, std::uint32_t> pointers;
    std::map<std::uint32_t, Face> faces;
    std::vector<std::string> events;

    std::uint32_t ReadFacePointer(std::uint32_t address) override {
        events.push_back("read:" + std::to_string(address));
        return pointers.at(address);
    }
    bool IsOutsideEdges3D(std::uint32_t face,
                           const pal4::D3dTriangleVector3&) override {
        events.push_back("classify:" + std::to_string(face));
        return faces.at(face).direct;
    }
    void ProjectHeight(std::uint32_t face,
                       pal4::D3dTriangleVector3& point) override {
        events.push_back("height:" + std::to_string(face));
        point.y = faces.at(face).height;
    }
    pal4::D3dTriangleVector3 Centroid(std::uint32_t face) override {
        events.push_back("centroid:" + std::to_string(face));
        return faces.at(face).centroid;
    }
    std::uint32_t TraceToPoint(
        std::uint32_t face, const pal4::D3dTriangleVector2& from,
        const pal4::D3dTriangleVector2& to,
        pal4::D3dTriangleVector2& intersection) override {
        events.push_back("trace:" + std::to_string(face));
        const auto& data = faces.at(face);
        Check(from.x == data.centroid.x && from.z == data.centroid.z &&
                  to.x == 0.0f && to.z == 0.0f,
              "fallback traces centroid to query X/Z");
        intersection = data.intersection;
        return data.trace_status;
    }
};

void TestSelectFace() {
    pal4::CollisionTreeStorageState state{};
    QueryRuntime runtime;
    const pal4::D3dTriangleVector3 point{};
    Check(pal4::CollisionTreeStorage::SelectFace(state, point, runtime) == 0u &&
              runtime.events.empty(),
          "empty face range returns null without runtime calls");

    state.words[1] = 0x1000u;
    state.words[2] = 0x1010u;
    runtime.pointers = {{0x1000u, 0xa000u}, {0x1004u, 0xb000u},
                        {0x1008u, 0xc000u}, {0x100cu, 0xd000u}};
    runtime.faces[0xa000u] = {false, 4.0f, {1.0f, 0.0f, 1.0f},
                              2u, {3.0f, 0.0f}};
    runtime.faces[0xb000u] = {false, 2.0f, {2.0f, 0.0f, 2.0f},
                              2u, {0.0f, 0.0f}};
    runtime.faces[0xc000u] = {true, 10.0f};
    runtime.faces[0xd000u] = {true, 1.0f};
    Check(pal4::CollisionTreeStorage::SelectFace(state, point, runtime) ==
              0xd000u,
          "direct faces supersede fallback distance and choose closest height");
    Check(runtime.events == std::vector<std::string>{
              "read:4096", "classify:40960", "centroid:40960",
              "trace:40960", "height:40960",
              "read:4100", "classify:45056", "centroid:45056",
              "trace:45056", "height:45056",
              "read:4104", "classify:49152", "height:49152",
              "read:4108", "classify:53248", "height:53248"},
          "fallback queries stop once a direct face is selected");

    runtime.events.clear();
    runtime.faces[0xc000u].height = 1.0f;
    Check(pal4::CollisionTreeStorage::SelectFace(state, point, runtime) ==
              0xc000u,
          "equal direct height difference retains the first direct face");
    runtime.faces[0xc000u].height =
        std::numeric_limits<float>::quiet_NaN();
    Check(pal4::CollisionTreeStorage::SelectFace(state, point, runtime) ==
              0xc000u,
          "first unordered direct difference remains selected");

    runtime.faces[0xc000u].direct = false;
    runtime.faces[0xc000u].trace_status = 1u;
    runtime.faces[0xd000u].direct = false;
    runtime.faces[0xd000u].trace_status = 0u;
    Check(pal4::CollisionTreeStorage::SelectFace(state, point, runtime) ==
              0xb000u,
          "fallback chooses shortest 3D distance and ignores non-hit traces");
    runtime.faces[0xb000u].height = 4.0f;
    runtime.faces[0xb000u].intersection = {3.0f, 0.0f};
    Check(pal4::CollisionTreeStorage::SelectFace(state, point, runtime) ==
              0xa000u,
          "equal fallback distance retains the first face");
    runtime.faces[0xa000u].trace_status = 0u;
    runtime.faces[0xb000u].trace_status = 0u;
    Check(pal4::CollisionTreeStorage::SelectFace(state, point, runtime) == 0u,
          "no direct or traced face returns null");
}

}  // namespace

int main() {
    TestInitialization();
    TestClearAndRelease();
    TestAppendTriangle();
    TestSelectFace();
}
