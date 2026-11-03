#pragma once

#include "pal4/screen_effect_dispatcher.h"

#include <cstdint>

namespace pal4 {

class IGameManagerResetRuntime {
public:
    virtual ~IGameManagerResetRuntime() = default;
    virtual void CallHandler(std::uint32_t handler,
                             std::uint32_t slot_offset) = 0;
    [[nodiscard]] virtual std::uint32_t FirstNode(
        std::uint32_t sentinel) = 0;
    [[nodiscard]] virtual std::uint32_t NodeObject(std::uint32_t node) = 0;
    virtual void InvokeShutdown(std::uint32_t object,
                                std::uint32_t slot_offset) = 0;
    virtual void DeleteObject(std::uint32_t object,
                              std::uint8_t flags) = 0;
    [[nodiscard]] virtual std::uint32_t RemoveNode(
        ScreenEffectDispatcherState& state, std::uint32_t node) = 0;
    virtual void CleanupCamera(std::uint32_t camera) = 0;
    [[nodiscard]] virtual std::uint32_t DestroyCameraRecord(
        std::uint32_t camera) = 0;
};

class GameManagerResetRuntimeStub final : public IGameManagerResetRuntime {
public:
    void CallHandler(std::uint32_t handler,
                     std::uint32_t slot_offset) override;
    [[nodiscard]] std::uint32_t FirstNode(
        std::uint32_t sentinel) override;
    [[nodiscard]] std::uint32_t NodeObject(std::uint32_t node) override;
    void InvokeShutdown(std::uint32_t object,
                        std::uint32_t slot_offset) override;
    void DeleteObject(std::uint32_t object,
                      std::uint8_t flags) override;
    [[nodiscard]] std::uint32_t RemoveNode(
        ScreenEffectDispatcherState& state, std::uint32_t node) override;
    void CleanupCamera(std::uint32_t camera) override;
    [[nodiscard]] std::uint32_t DestroyCameraRecord(
        std::uint32_t camera) override;
};

class GameManagerReset final {
public:
    static void StopActive(ScreenEffectDispatcherState& state,
                           IGameManagerResetRuntime& runtime);
    [[nodiscard]] static std::uint32_t Reset(
        ScreenEffectDispatcherState& state, std::uint32_t incoming_eax,
        IGameManagerResetRuntime& runtime);
};

}  // namespace pal4
