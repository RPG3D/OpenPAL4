#pragma once

#include "pal4/matfx_world_sector_pipeline.h"

#include <array>
#include <cstdint>
#include <string>
#include <string_view>

namespace pal4 {

class IWorldManagerLightmapRuntime {
public:
    virtual ~IWorldManagerLightmapRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::string_view ReadCString(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t OpenFile(std::string_view path, std::string_view mode) = 0;
    virtual void ReadFile(std::uint32_t destination, std::uint32_t bytes,
                          std::uint32_t count, std::uint32_t file) = 0;
    virtual void CloseFile(std::uint32_t file) = 0;
};

class IWorldManagerSceneInitializationRuntime : public IWorldManagerLightmapRuntime {
public:
    virtual ~IWorldManagerSceneInitializationRuntime() = default;
    [[nodiscard]] virtual std::uint32_t GameStateAddress() = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::string_view ReadCString(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t LoadPackage(
        std::string_view path, std::uint32_t callback) = 0;
    virtual void ProcessScript(std::uint32_t script) = 0;
    virtual void CleanupAllScripts() = 0;
    virtual void DestroyScript(std::uint32_t script) = 0;
    [[nodiscard]] virtual std::string ConcatenateWorldBase(
        std::uint32_t first, std::uint32_t second) = 0;
    [[nodiscard]] virtual std::uint32_t FinishWorldBase(
        std::string_view value) = 0;
    [[nodiscard]] virtual std::uint32_t GameInstance() = 0;
    virtual void InitializeWorld(
        std::uint32_t world, std::string_view base,
        std::string_view region, std::string_view scene,
        std::uint32_t game_context) = 0;
    virtual void AddLight(std::uint32_t world,
                          std::array<float, 4> color) = 0;
    [[nodiscard]] virtual std::uint32_t OpenFile(
        std::string_view path, std::string_view mode) = 0;
    virtual void ReadFile(std::uint32_t destination, std::uint32_t bytes,
                          std::uint32_t count, std::uint32_t file) = 0;
    virtual void CloseFile(std::uint32_t file) = 0;
    [[nodiscard]] virtual IMatFxWorldSectorPipelineRuntime& PipelineRuntime() = 0;
    virtual void BindWorldSectors(
        std::uint32_t world, std::uint32_t callback,
        std::uint32_t pipeline) = 0;
    virtual void AddClump(std::uint32_t world,
                          std::uint32_t clump) = 0;
    virtual void ProcessPluginAtomics(
        std::uint32_t clump, std::uint32_t options) = 0;
    virtual void ProcessClumpAtomics(std::uint32_t clump) = 0;
};

class WorldManagerSceneInitializer final {
public:
    static constexpr std::uint32_t kPackageCallback = 0x6706e0u;
    static constexpr std::uint32_t kSectorCallback = 0x51db30u;

    [[nodiscard]] static std::uint32_t Initialize(
        std::uint32_t world, std::string_view region,
        std::string_view scene,
        IWorldManagerSceneInitializationRuntime& runtime);
    [[nodiscard]] static bool LoadLightmapConfig(
        std::uint32_t world,
        IWorldManagerLightmapRuntime& runtime);
    [[nodiscard]] static std::uint32_t ClearPipeline(
        std::uint32_t world,
        IWorldManagerSceneInitializationRuntime& runtime);
};

}  // namespace pal4
