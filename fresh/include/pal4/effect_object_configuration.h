#pragma once

#include "pal4/effect_object.h"

#include <cstdint>
#include <string_view>

namespace pal4 {

class IEffectObjectConfigurationRuntime {
public:
    virtual ~IEffectObjectConfigurationRuntime() = default;
    [[nodiscard]] virtual float ReadFloat(
        std::uint32_t document, std::string_view key,
        float default_value) = 0;
    [[nodiscard]] virtual std::uint32_t GetChildByIndex(
        std::uint32_t document, std::uint32_t index) = 0;
    [[nodiscard]] virtual std::uint32_t CreateInstance() = 0;
    [[nodiscard]] virtual bool InitializeInstance(
        std::uint32_t instance, std::uint32_t child) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual bool UpdateInstance(std::uint32_t instance) = 0;
    virtual void CleanupInstance(std::uint32_t instance,
                                 std::uint32_t context) = 0;
    virtual void DestroyInstance(std::uint32_t instance,
                                 std::uint32_t context) = 0;
    [[nodiscard]] virtual std::uint32_t CreateFrame() = 0;
    virtual void UpdateFrameProperties(EffectObjectState& state) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(
        std::uint32_t address) = 0;
    virtual void WriteByte(std::uint32_t address,
                           std::uint8_t value) = 0;
};

class EffectObjectConfigurationRuntimeStub final
    : public IEffectObjectConfigurationRuntime {
public:
    [[nodiscard]] float ReadFloat(
        std::uint32_t, std::string_view,
        float value) override { return value; }
    [[nodiscard]] std::uint32_t GetChildByIndex(
        std::uint32_t, std::uint32_t) override { return 0u; }
    [[nodiscard]] std::uint32_t CreateInstance() override { return 0u; }
    [[nodiscard]] bool InitializeInstance(
        std::uint32_t, std::uint32_t) override { return false; }
    void WriteWord(std::uint32_t, std::uint32_t) override {}
    [[nodiscard]] bool UpdateInstance(std::uint32_t) override { return false; }
    void CleanupInstance(std::uint32_t, std::uint32_t) override {}
    void DestroyInstance(std::uint32_t, std::uint32_t) override {}
    [[nodiscard]] std::uint32_t CreateFrame() override { return 0u; }
    void UpdateFrameProperties(EffectObjectState&) override {}
    [[nodiscard]] std::uint8_t ReadByte(
        std::uint32_t) override { return 0u; }
    void WriteByte(std::uint32_t, std::uint8_t) override {}
};

class EffectObjectConfiguration final {
public:
    // IDA 0x610440; context is native EBP forwarded on cleanup.
    [[nodiscard]] static std::uint8_t Load(
        EffectObjectState& state, std::uint32_t document,
        std::uint32_t context,
        IEffectObjectConfigurationRuntime& runtime);
};

}  // namespace pal4
