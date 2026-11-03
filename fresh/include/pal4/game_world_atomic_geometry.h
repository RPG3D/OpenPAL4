#pragma once

#include <array>
#include <cstdint>
#include <string>

namespace pal4 {

struct GameWorldAtomicGeometryState final {
    std::uint32_t first_atomic = 0u;
    std::uint32_t transform_mode = 0u;
};

using GameWorldBoundsBits = std::array<std::uint32_t, 4>;
using GameWorldMatrixBits = std::array<std::uint32_t, 16>;

struct GameWorldAtomicUserData final {
    std::uint32_t type = 0u;
    std::string text;
};

class IGameWorldAtomicGeometryRuntime {
public:
    virtual ~IGameWorldAtomicGeometryRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ForAllAtomics(
        std::uint32_t clump, GameWorldBoundsBits& bounds,
        GameWorldAtomicGeometryState& state) = 0;
    [[nodiscard]] virtual std::uint32_t UserDataCount(
        std::uint32_t atomic) = 0;
    [[nodiscard]] virtual GameWorldAtomicUserData ReadUserData(
        std::uint32_t atomic, std::uint32_t index) = 0;
    [[nodiscard]] virtual std::uint32_t AtomicFlags(
        std::uint32_t atomic) = 0;
    virtual void ResyncSphere(std::uint32_t atomic) = 0;
    [[nodiscard]] virtual GameWorldBoundsBits ReadSphere(
        std::uint32_t atomic) = 0;
    [[nodiscard]] virtual std::uint32_t AtomicFrame(
        std::uint32_t atomic) = 0;
    [[nodiscard]] virtual std::uint32_t ParentFrame(
        std::uint32_t atomic) = 0;
    [[nodiscard]] virtual GameWorldMatrixBits ObjectTransform(
        std::uint32_t frame) = 0;
    [[nodiscard]] virtual GameWorldMatrixBits SetupTransformMatrix(
        const GameWorldMatrixBits& matrix) = 0;
    [[nodiscard]] virtual GameWorldMatrixBits MultiplyMatrix(
        const GameWorldMatrixBits& first,
        const GameWorldMatrixBits& second) = 0;
    [[nodiscard]] virtual GameWorldBoundsBits TransformSphere(
        const GameWorldBoundsBits& sphere,
        const GameWorldMatrixBits& matrix) = 0;
    [[nodiscard]] virtual double Distance(
        const std::array<float, 3>& delta) = 0;
};

class GameWorldAtomicGeometry final {
public:
    [[nodiscard]] static std::uint32_t Process(
        std::uint32_t clump, std::uint32_t transform_mode,
        GameWorldBoundsBits& bounds,
        GameWorldAtomicGeometryState& state,
        IGameWorldAtomicGeometryRuntime& runtime);
    [[nodiscard]] static std::uint32_t UpdateAtomic(
        std::uint32_t atomic, GameWorldBoundsBits& bounds,
        GameWorldAtomicGeometryState& state,
        IGameWorldAtomicGeometryRuntime& runtime);
};

}  // namespace pal4
