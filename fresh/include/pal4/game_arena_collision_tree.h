#pragma once

#include "pal4/game_arena_object_list.h"

#include <array>
#include <cstdint>

namespace pal4 {

using CollisionRawVector = std::array<std::uint32_t, 3>;
using CollisionVector = std::array<float, 3>;

class IGameArenaCollisionRuntime : public IGameArenaObjectListRuntime {
public:
    [[nodiscard]] virtual std::uint32_t AllocateTree(std::uint32_t size) = 0;
    [[nodiscard]] virtual std::uint32_t InitializeTree(std::uint32_t memory) = 0;
    virtual void ForAllAtomics(
        std::uint32_t clump, std::uint32_t arena,
        std::uint32_t (*callback)(std::uint32_t, std::uint32_t,
                                  IGameArenaCollisionRuntime&)) = 0;
    virtual void SetupRenderStates(std::uint32_t tree) = 0;
    virtual void SetDebugRenderer(std::uint32_t tree) = 0;
    [[nodiscard]] virtual std::uint16_t ReadHalfWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t GetObjectTransform(
        std::uint32_t frame) = 0;
    [[nodiscard]] virtual CollisionRawVector TransformVertex(
        std::uint32_t vertex, std::uint32_t transform) = 0;
    [[nodiscard]] virtual CollisionVector ConvertCoordinates(
        const CollisionRawVector& raw) = 0;
    [[nodiscard]] virtual bool VectorsDiffer(
        const CollisionVector& left, const CollisionVector& right) = 0;
    virtual void AddTriangle(std::uint32_t tree,
                             const CollisionVector& first,
                             const CollisionVector& second,
                             const CollisionVector& third) = 0;
};

class GameArenaCollisionRuntimeStub final : public IGameArenaCollisionRuntime {
public:
    [[nodiscard]] std::uint32_t ReadWord(std::uint32_t) override { return 0u; }
    void WriteWord(std::uint32_t, std::uint32_t) override {}
    [[nodiscard]] std::string ReadCString(std::uint32_t) override { return {}; }
    [[nodiscard]] bool TextEqualsIgnoreCase(
        std::uint32_t, std::string_view) override { return false; }
    void InitializeText(std::uint32_t, std::string_view) override {}
    void ReleaseObjectChildren(std::uint32_t) override {}
    void DestroyObject(std::uint32_t) override {}
    [[nodiscard]] std::uint32_t FreeObject(std::uint32_t) override { return 0u; }
    [[nodiscard]] std::uint32_t AllocateTree(std::uint32_t) override { return 0u; }
    [[nodiscard]] std::uint32_t InitializeTree(std::uint32_t) override { return 0u; }
    void ForAllAtomics(std::uint32_t, std::uint32_t,
        std::uint32_t (*)(std::uint32_t, std::uint32_t,
                          IGameArenaCollisionRuntime&)) override {}
    void SetupRenderStates(std::uint32_t) override {}
    void SetDebugRenderer(std::uint32_t) override {}
    [[nodiscard]] std::uint16_t ReadHalfWord(std::uint32_t) override { return 0u; }
    [[nodiscard]] std::uint32_t GetObjectTransform(std::uint32_t) override {
        return 0u;
    }
    [[nodiscard]] CollisionRawVector TransformVertex(
        std::uint32_t, std::uint32_t) override { return {}; }
    [[nodiscard]] CollisionVector ConvertCoordinates(
        const CollisionRawVector&) override { return {}; }
    [[nodiscard]] bool VectorsDiffer(
        const CollisionVector&, const CollisionVector&) override { return false; }
    void AddTriangle(std::uint32_t, const CollisionVector&,
                     const CollisionVector&, const CollisionVector&) override {}
};

class GameArenaCollisionTree final {
public:
    // IDA 0x51c0a0 and 0x51c140.
    static void BuildFromClump(std::uint32_t arena, std::uint32_t clump,
                               IGameArenaCollisionRuntime& runtime);
    [[nodiscard]] static std::uint32_t AddTriangles(
        std::uint32_t atomic, std::uint32_t arena,
        IGameArenaCollisionRuntime& runtime);
};

}  // namespace pal4
