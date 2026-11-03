#pragma once

#include <cstdint>

namespace pal4 {

class IWorldMapEventRuntime {
public:
    virtual ~IWorldMapEventRuntime() = default;
    [[nodiscard]] virtual std::uint32_t BeginName() = 0;
    virtual void FormatButtonName(std::uint32_t buffer,
                                  std::uint32_t prefix,
                                  std::uint32_t number) = 0;
    virtual void FormatRoleName(std::uint32_t buffer,
                                std::uint32_t prefix) = 0;
    [[nodiscard]] virtual std::uint32_t FindWindow(
        std::uint32_t frame, std::uint32_t name_buffer) = 0;
    [[nodiscard]] virtual std::uint32_t EndName(
        std::uint32_t name_buffer) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(
        std::uint32_t bytes) = 0;
    [[nodiscard]] virtual std::uint32_t Subscribe(
        std::uint32_t window, std::uint32_t event,
        std::uint32_t callback) = 0;
    virtual void ReleaseConnection(std::uint32_t connection) = 0;
    virtual void SetEnabled(std::uint32_t window, bool enabled) = 0;
    virtual void SetupMouseHandlers(std::uint32_t mouse,
                                    std::uint32_t window) = 0;
};

class IWorldMapMouseDispatchRuntime {
public:
    virtual ~IWorldMapMouseDispatchRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t CallVirtual(
        std::uint32_t object, std::uint32_t function) = 0;
};

class WorldMapEventHandlers final {
public:
    static constexpr std::uint32_t kButtonCount = 21u;
    static constexpr std::uint32_t kCallbackVtable = 0x843cc0u;
    static constexpr std::uint32_t kOnLocationClick = 0x484a20u;
    static constexpr std::uint32_t kOnKeyDown = 0x484bd0u;
    static constexpr std::uint32_t kClickedEvent = 0x8411b8u;
    static constexpr std::uint32_t kKeyDownEvent = 0x84103cu;

    [[nodiscard]] static std::uint32_t Initialize(
        std::uint32_t frame, IWorldMapEventRuntime& runtime);
    [[nodiscard]] static std::uint32_t ForwardMouseSlot0(
        std::uint32_t frame, IWorldMapMouseDispatchRuntime& runtime);
};

}  // namespace pal4
