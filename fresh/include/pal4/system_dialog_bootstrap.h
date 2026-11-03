#pragma once

#include <cstdint>

namespace pal4 {

class ISystemDialogFrameRuntime;

class ISystemDialogBootstrapRuntime {
public:
    virtual ~ISystemDialogBootstrapRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    [[nodiscard]] virtual std::uint32_t ConstructDialog(
        std::uint32_t storage, std::uint32_t name) = 0;
    virtual void Free(std::uint32_t storage) = 0;
    [[nodiscard]] virtual std::uint32_t FrameManager() = 0;
    [[nodiscard]] virtual std::uint32_t ProcessFrameEvent(
        std::uint32_t manager, std::uint32_t name,
        std::uint32_t factory) = 0;
};

class SystemDialogBootstrap final {
public:
    static constexpr std::uint32_t kNameSource = 0x8b0888u;
    static constexpr std::uint32_t kCurrentName = 0x8e7988u;
    static constexpr std::uint32_t kFactoryAddress = 0x4835a0u;

    [[nodiscard]] static std::uint32_t GetFrameName(
        ISystemDialogBootstrapRuntime& runtime);
    [[nodiscard]] static std::uint32_t InitializeFrameName(
        ISystemDialogBootstrapRuntime& runtime);
    [[nodiscard]] static std::uint32_t CreateDialog(
        std::uint32_t name, ISystemDialogBootstrapRuntime& runtime);
    [[nodiscard]] static std::uint32_t CreateDialog(
        std::uint32_t name, ISystemDialogBootstrapRuntime& runtime,
        ISystemDialogFrameRuntime& frame_runtime);
    [[nodiscard]] static std::uint32_t GetFactory(
        ISystemDialogBootstrapRuntime& runtime);
    [[nodiscard]] static std::uint32_t SetupFrame(
        ISystemDialogBootstrapRuntime& runtime);
};

}  // namespace pal4
