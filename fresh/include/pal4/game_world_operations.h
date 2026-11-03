#pragma once

#include "pal4/game_world_object.h"

#include <array>
#include <cstdint>

namespace pal4 {

class IGameWorldCameraRuntime {
public:
    virtual ~IGameWorldCameraRuntime() = default;
    virtual void RemoveCamera(std::uint32_t world,std::uint32_t camera) = 0;
    virtual void AddCamera(std::uint32_t world,std::uint32_t camera) = 0;
};
class IGameWorldOperationsRuntime : public IGameWorldCameraRuntime {
public:
    virtual ~IGameWorldOperationsRuntime() = default;
    virtual void DestroyLight(std::uint32_t light) = 0;
    [[nodiscard]] virtual std::uint32_t CreateLight() = 0;
    virtual void SetLightColor(
        std::uint32_t light, const std::array<std::uint32_t, 4>& rgba_bits) = 0;
    virtual void SetLightRadius(std::uint32_t light,
                                std::uint32_t radius_bits) = 0;
    virtual void SetLightConeAngle(std::uint32_t light,
                                   std::uint32_t angle_bits) = 0;
    virtual void AddLight(std::uint32_t world, std::uint32_t light) = 0;
    virtual void RemoveCamera(std::uint32_t world,
                              std::uint32_t camera) = 0;
    virtual void AddCamera(std::uint32_t world,
                           std::uint32_t camera) = 0;
    [[nodiscard]] virtual std::uint32_t GetClumpWorld(
        std::uint32_t clump) = 0;
    virtual void AddClump(std::uint32_t world,
                          std::uint32_t clump) = 0;
    virtual void RemoveClump(std::uint32_t world,
                             std::uint32_t clump) = 0;
};

class GameWorldOperations final {
public:
    static constexpr std::uint32_t kLightRadiusBits = 0x43fa0000u;
    static constexpr std::uint32_t kLightConeAngleBits = 0x3f5f66f3u;

    [[nodiscard]] static bool AddLight(
        GameWorldObjectState& state,
        const std::array<std::uint32_t, 4>& rgba_bits,
        IGameWorldOperationsRuntime& runtime);
    [[nodiscard]] static bool SetCamera(
        GameWorldObjectState& state, std::uint32_t camera,
        IGameWorldOperationsRuntime& runtime);
    [[nodiscard]] static bool SetCameraBinding(
        GameWorldObjectState& state,std::uint32_t camera,IGameWorldCameraRuntime& runtime);
    [[nodiscard]] static bool AddClump(
        const GameWorldObjectState& state, std::uint32_t clump,
        IGameWorldOperationsRuntime& runtime);
    [[nodiscard]] static bool RemoveClump(
        const GameWorldObjectState& state, std::uint32_t clump,
        IGameWorldOperationsRuntime& runtime);
};

}  // namespace pal4
