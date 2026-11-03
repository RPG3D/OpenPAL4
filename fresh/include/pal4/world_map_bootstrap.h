#pragma once

#include <cstdint>

namespace pal4 {

class IWorldMapFrameRuntime;

class IWorldMapBootstrapRuntime {
public:
    virtual ~IWorldMapBootstrapRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    [[nodiscard]] virtual std::uint32_t ConstructMap(
        std::uint32_t storage, std::uint32_t name) = 0;
    virtual void Free(std::uint32_t storage) = 0;
    [[nodiscard]] virtual std::uint32_t FrameManager() = 0;
    [[nodiscard]] virtual std::uint32_t ProcessFrameEvent(
        std::uint32_t manager, std::uint32_t name,
        std::uint32_t factory) = 0;
};

class WorldMapBootstrap final {
public:
    static constexpr std::uint32_t kNameSource = 0x8b089cu;
    static constexpr std::uint32_t kCurrentName = 0x8e7990u;
    static constexpr std::uint32_t kFactoryAddress = 0x483b70u;

    [[nodiscard]] static std::uint32_t GetFrameName(
        IWorldMapBootstrapRuntime& runtime);
    [[nodiscard]] static std::uint32_t InitializeFrameName(
        IWorldMapBootstrapRuntime& runtime);
    [[nodiscard]] static std::uint32_t CreateMap(
        std::uint32_t name, IWorldMapBootstrapRuntime& runtime);
    [[nodiscard]] static std::uint32_t CreateMap(
        std::uint32_t name, IWorldMapBootstrapRuntime& runtime,
        IWorldMapFrameRuntime& frame_runtime);
    [[nodiscard]] static std::uint32_t GetFactory(
        IWorldMapBootstrapRuntime& runtime);
    [[nodiscard]] static std::uint32_t SetupFrame(
        IWorldMapBootstrapRuntime& runtime);
};

}  // namespace pal4
