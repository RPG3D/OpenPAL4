#pragma once

#include "pal4/effect_system_cleanup.h"

#include <cstdint>
#include <string_view>

namespace pal4 {

class IEffectSystemActiveFrameRuntime : public IEffectSystemPruneRuntime {
public:
    ~IEffectSystemActiveFrameRuntime() override = default;
    [[nodiscard]] virtual std::uint32_t NextActiveNode(
        std::uint32_t tree, std::uint32_t node);
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    virtual void UpdatePlayback(std::uint32_t effect,
                                double seconds) = 0;
    virtual void StartPlayback(std::uint32_t effect) = 0;
    virtual void PausePlayback(std::uint32_t effect) = 0;
    virtual void ResetDialog(std::uint32_t effect) = 0;
    virtual void ReturnEffect(std::uint32_t effect,
                              std::uint32_t mode) = 0;
    [[nodiscard]] virtual EffectSystemHandle CopyHandle(
        std::uint32_t id, std::uint32_t system) = 0;
    virtual void ResetTemporaryHandle(EffectSystemHandle& handle) = 0;
    virtual void NotifyStarted(std::uint32_t system,
                               const EffectSystemHandle& handle,
                               std::int32_t repeat_count) = 0;
    virtual void NotifyFinished(std::uint32_t system,
                                const EffectSystemHandle& handle,
                                std::int32_t repeat_count) = 0;
    [[nodiscard]] virtual std::uint32_t FindXmlAttribute(
        std::uint32_t xml, std::string_view key) = 0;
    virtual void SetXmlInt(std::uint32_t attribute,
                           std::uint32_t value) = 0;
    virtual void AddXmlInt(std::uint32_t xml,
                           std::string_view key,
                           std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t EraseActiveNode(
        std::uint32_t tree, std::uint32_t node) = 0;
};

}  // namespace pal4
