#pragma once

#include <cstdint>

namespace pal4 {

class IWorldMapCallbackRuntime {
public:
    virtual ~IWorldMapCallbackRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t InvokeThiscall(
        std::uint32_t function, std::uint32_t receiver) = 0;
};

class WorldMapCallbackBridge final {
public:
    static constexpr std::uint32_t kBaseVtable = 0x8435d0u;
    static constexpr std::uint32_t kWorldMapVtable = 0x843cc8u;

    // IDA 0x485230.
    [[nodiscard]] static std::uint32_t Clone(
        std::uint32_t source, IWorldMapCallbackRuntime& runtime);
    // IDA 0x485270.
    [[nodiscard]] static std::uint32_t Invoke(
        std::uint32_t callback, IWorldMapCallbackRuntime& runtime);
};

class IUIFrameManagerAccessorRuntime {
public:
    virtual ~IUIFrameManagerAccessorRuntime() = default;
    [[nodiscard]] virtual std::uint32_t GetInstance() = 0;
};

class UIFrameManagerAccessor final {
public:
    // IDA 0x485220: tail thunk to UIFrameManager_GetInstance.
    [[nodiscard]] static std::uint32_t Get(
        IUIFrameManagerAccessorRuntime& runtime);
};

}  // namespace pal4
