#pragma once

#include "pal4/screen_effect_dispatcher.h"

#include <cstdint>

namespace pal4 {

class IGameManagerCollectionRuntime {
public:
    virtual ~IGameManagerCollectionRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadDword(
        std::uint32_t address) = 0;
    virtual void Invoke(std::uint32_t object,
                        std::uint32_t slot_offset) = 0;
};

class GameManagerCollectionRuntimeStub final
    : public IGameManagerCollectionRuntime {
public:
    [[nodiscard]] std::uint32_t ReadDword(
        std::uint32_t address) override;
    void Invoke(std::uint32_t object,
                std::uint32_t slot_offset) override;
};

class GameManagerCollectionWalker final {
public:
    static constexpr std::uint32_t kNilNode = 0x8e4110u;

    [[nodiscard]] static std::uint32_t NotifySlot40(
        const ScreenEffectDispatcherState& state,
        IGameManagerCollectionRuntime& runtime);
    [[nodiscard]] static std::uint32_t NotifySlot44(
        const ScreenEffectDispatcherState& state,
        IGameManagerCollectionRuntime& runtime);

private:
    [[nodiscard]] static std::uint32_t Walk(
        const ScreenEffectDispatcherState& state,
        std::uint32_t slot_offset,
        IGameManagerCollectionRuntime& runtime);
};

}  // namespace pal4
