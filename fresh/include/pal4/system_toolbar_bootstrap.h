#pragma once

#include <cstdint>

namespace pal4 {

class ISystemToolbarFrameRuntime;

class ISystemToolbarBootstrapRuntime {
public:
    virtual ~ISystemToolbarBootstrapRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    [[nodiscard]] virtual std::uint32_t ConstructFrame(
        std::uint32_t storage, std::uint32_t name) = 0;
    virtual void Free(std::uint32_t storage) = 0;
    [[nodiscard]] virtual std::uint32_t FrameManager() = 0;
    [[nodiscard]] virtual std::uint32_t ProcessFrameEvent(
        std::uint32_t manager, std::uint32_t name,
        std::uint32_t factory) = 0;
};

class SystemToolbarBootstrap final {
public:
    static constexpr std::uint32_t kNameSource = 0x8b07d8u;
    static constexpr std::uint32_t kCurrentName = 0x8e7980u;
    static constexpr std::uint32_t kFactoryAddress = 0x482550u;

    [[nodiscard]] static std::uint32_t GetFrameName(
        ISystemToolbarBootstrapRuntime& runtime);
    [[nodiscard]] static std::uint32_t InitializeFrameName(
        ISystemToolbarBootstrapRuntime& runtime);
    [[nodiscard]] static std::uint32_t CreateFrame(
        std::uint32_t name, ISystemToolbarBootstrapRuntime& runtime);
    [[nodiscard]] static std::uint32_t CreateFrame(
        std::uint32_t name, ISystemToolbarBootstrapRuntime& runtime,
        ISystemToolbarFrameRuntime& frame_runtime);
    [[nodiscard]] static std::uint32_t CreateUI(
        ISystemToolbarBootstrapRuntime& runtime);
    [[nodiscard]] static std::uint32_t SetupFrame(
        ISystemToolbarBootstrapRuntime& runtime);
};

}  // namespace pal4
