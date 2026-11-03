#include "pal4/collision_tree_storage.h"

#include <bit>
#include <cstdlib>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

using pal4::D3dTriangleVector3;

void SetVertices(pal4::D3dTriangleFaceState& face,
                 const D3dTriangleVector3& a,
                 const D3dTriangleVector3& b,
                 const D3dTriangleVector3& c) {
    const D3dTriangleVector3 vertices[] = {a, b, c};
    for (std::uint32_t vertex = 0u; vertex < 3u; ++vertex) {
        const float values[] = {vertices[vertex].x, vertices[vertex].y,
                                vertices[vertex].z};
        for (std::uint32_t component = 0u; component < 3u; ++component) {
            face.words[7u + vertex * 3u + component] =
                std::bit_cast<std::uint32_t>(values[component]);
        }
    }
}

struct Runtime final : pal4::ICollisionTreeNeighborRuntime {
    std::map<std::uint32_t, std::uint32_t> slots;
    std::map<std::uint32_t, pal4::D3dTriangleFaceState> faces;
    std::vector<std::uint32_t> reads;

    std::uint32_t ReadFacePointer(const std::uint32_t slot) override {
        reads.push_back(slot);
        return slots.at(slot);
    }
    pal4::D3dTriangleFaceState& Face(
        const std::uint32_t address) override {
        return faces.at(address);
    }
};

pal4::CollisionTreeStorageState Storage(const std::uint32_t count) {
    pal4::CollisionTreeStorageState state{};
    state.words[1] = 0x1000u;
    state.words[2] = 0x1000u + count * 4u;
    return state;
}

void TestEmptyAndSingle() {
    Runtime runtime;
    auto storage = Storage(0u);
    Check(pal4::CollisionTreeStorage::LinkFaceNeighbors(storage, runtime) ==
              0x1000u && runtime.reads.empty(),
          "empty range returns end without reading faces");
    storage = Storage(1u);
    runtime.slots[0x1000u] = 0x2000u;
    runtime.faces[0x2000u] = {};
    Check(pal4::CollisionTreeStorage::LinkFaceNeighbors(storage, runtime) ==
              0x1004u && runtime.reads == std::vector<std::uint32_t>{0x1000u},
          "single face is read once and skips its own slot");
}

void TestSharedEdgeAndOrder() {
    Runtime runtime;
    auto storage = Storage(3u);
    runtime.slots = {{0x1000u, 0x2000u}, {0x1004u, 0x3000u},
                     {0x1008u, 0x4000u}};
    const D3dTriangleVector3 a{0.0f, 0.0f, 0.0f};
    const D3dTriangleVector3 b{1.0f, 0.0f, 0.0f};
    const D3dTriangleVector3 c{0.0f, 0.0f, 1.0f};
    const D3dTriangleVector3 d{1.0f, 0.0f, 1.0f};
    const D3dTriangleVector3 e{5.0f, 0.0f, 5.0f};
    SetVertices(runtime.faces[0x2000u], a, b, c);
    SetVertices(runtime.faces[0x3000u], b, a, d);
    SetVertices(runtime.faces[0x4000u], e, d, b);
    Check(pal4::CollisionTreeStorage::LinkFaceNeighbors(storage, runtime) ==
              0x100cu,
          "link returns pointer-array end");
    Check(pal4::D3dTriangleFace::Neighbor(runtime.faces.at(0x2000u), 0u) ==
              0x3000u &&
              pal4::D3dTriangleFace::Neighbor(runtime.faces.at(0x3000u), 0u) ==
                  0x2000u,
          "shared AB edge links both directions");
    Check(pal4::D3dTriangleFace::Neighbor(runtime.faces.at(0x2000u), 1u) ==
              0u &&
              pal4::D3dTriangleFace::Neighbor(runtime.faces.at(0x2000u), 2u) ==
                  0u,
          "nonmatching edges stay clear");
    Check(runtime.reads == std::vector<std::uint32_t>{
              0x1000u, 0x1004u, 0x1008u,
              0x1004u, 0x1000u, 0x1008u,
              0x1008u, 0x1000u, 0x1004u},
          "ordered pair traversal reads each different slot once");
}

void TestLaterEdgesAndOccupiedEdge() {
    Runtime runtime;
    auto storage = Storage(2u);
    runtime.slots = {{0x1000u, 0x2000u}, {0x1004u, 0x3000u}};
    const D3dTriangleVector3 a{0.0f, 0.0f, 0.0f};
    const D3dTriangleVector3 b{1.0f, 0.0f, 0.0f};
    const D3dTriangleVector3 c{0.0f, 0.0f, 1.0f};
    const D3dTriangleVector3 d{1.0f, 0.0f, 1.0f};
    SetVertices(runtime.faces[0x2000u], a, b, c);
    SetVertices(runtime.faces[0x3000u], c, b, d);
    static_cast<void>(pal4::CollisionTreeStorage::LinkFaceNeighbors(
        storage, runtime));
    Check(pal4::D3dTriangleFace::Neighbor(runtime.faces.at(0x2000u), 1u) ==
              0x3000u &&
              pal4::D3dTriangleFace::Neighbor(runtime.faces.at(0x3000u), 0u) ==
                  0x2000u,
          "second edge matches in reverse direction");

    runtime.faces[0x2000u] = {};
    runtime.faces[0x3000u] = {};
    SetVertices(runtime.faces[0x2000u], a, b, c);
    SetVertices(runtime.faces[0x3000u], a, c, d);
    static_cast<void>(pal4::D3dTriangleFace::SetNeighbor(
        runtime.faces[0x2000u], 0u, 0x9999u));
    static_cast<void>(pal4::CollisionTreeStorage::LinkFaceNeighbors(
        storage, runtime));
    Check(pal4::D3dTriangleFace::Neighbor(runtime.faces.at(0x2000u), 0u) ==
              0x9999u &&
              pal4::D3dTriangleFace::Neighbor(runtime.faces.at(0x2000u), 2u) ==
                  0x3000u &&
              pal4::D3dTriangleFace::Neighbor(runtime.faces.at(0x3000u), 0u) ==
                  0x2000u,
          "occupied first edge is preserved while third edge links");
}

void TestFaceCountAndIndexedAccess() {
    Runtime runtime;
    auto storage = Storage(0u);
    storage.words[1] = 0u;
    storage.words[2] = 0x1000u;
    Check(pal4::CollisionTreeStorage::FaceCount(storage) == 0,
          "null begin reports zero even with nonzero end");
    bool threw = false;
    try {
        static_cast<void>(pal4::CollisionTreeStorage::FaceAt(
            storage, 0u, runtime));
    } catch (const std::out_of_range& error) {
        threw = std::string(error.what()) ==
            "invalid vector<T> subscript";
    }
    Check(threw && runtime.reads.empty(),
          "empty face array throws native vector-subscript error");

    storage = Storage(3u);
    runtime.slots = {{0x1000u, 0x2000u}, {0x1004u, 0x3000u},
                     {0x1008u, 0x4000u}};
    Check(pal4::CollisionTreeStorage::FaceCount(storage) == 3 &&
              pal4::CollisionTreeStorage::FaceAt(storage, 2u, runtime) ==
                  0x4000u &&
              runtime.reads == std::vector<std::uint32_t>{0x1008u},
          "count and valid indexed access use four-byte pointer slots");
    for (const auto index : {3u, 0xffffffffu}) {
        threw = false;
        try {
            static_cast<void>(pal4::CollisionTreeStorage::FaceAt(
                storage, index, runtime));
        } catch (const std::out_of_range& error) {
            threw = std::string(error.what()) ==
                "invalid vector<T> subscript";
        }
        Check(threw, "end and large indexes throw out_of_range");
    }
    Check(runtime.reads == std::vector<std::uint32_t>{0x1008u},
          "invalid indexes do not read pointer slots");

    storage.words[2] = storage.words[1] - 4u;
    Check(pal4::CollisionTreeStorage::FaceCount(storage) == -1,
          "native count uses signed arithmetic shift");
}

}  // namespace

int main() {
    TestEmptyAndSingle();
    TestSharedEdgeAndOrder();
    TestLaterEdgesAndOccupiedEdge();
    TestFaceCountAndIndexedAccess();
}
