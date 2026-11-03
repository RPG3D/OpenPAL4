#pragma once

#include <cstdint>

namespace pal4 {

class IGameArenaCollisionDebugRuntime {
public:
    virtual ~IGameArenaCollisionDebugRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t size,
                                                 std::uint32_t flags) = 0;
    [[nodiscard]] virtual std::uint32_t GetVertexAddress(
        std::uint32_t triangle, std::uint32_t index) = 0;
    virtual void SetRenderState(std::uint32_t state, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t Transform(
        std::uint32_t vertices, std::uint32_t count,
        std::uint32_t matrix, std::uint32_t flags) = 0;
    virtual void RenderPrimitive(std::uint32_t type) = 0;
    virtual void EndTransform() = 0;
    virtual void Free(std::uint32_t address) = 0;
};

class GameArenaCollisionDebugRuntimeStub final
    : public IGameArenaCollisionDebugRuntime {
public:
    [[nodiscard]] std::uint32_t ReadWord(std::uint32_t) override { return 0u; }
    void WriteWord(std::uint32_t, std::uint32_t) override {}
    [[nodiscard]] std::uint32_t Allocate(std::uint32_t,
                                         std::uint32_t) override { return 0u; }
    [[nodiscard]] std::uint32_t GetVertexAddress(
        std::uint32_t, std::uint32_t) override { return 0u; }
    void SetRenderState(std::uint32_t, std::uint32_t) override {}
    [[nodiscard]] std::uint32_t Transform(
        std::uint32_t, std::uint32_t,
        std::uint32_t, std::uint32_t) override { return 0u; }
    void RenderPrimitive(std::uint32_t) override {}
    void EndTransform() override {}
    void Free(std::uint32_t) override {}
};

class GameArenaCollisionDebugRenderer final {
public:
    // IDA 0x51c2a0.
    static void Render(std::uint32_t tree,
                       IGameArenaCollisionDebugRuntime& runtime);
};

}  // namespace pal4
