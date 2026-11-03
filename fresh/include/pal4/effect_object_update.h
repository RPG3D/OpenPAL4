#pragma once

#include "pal4/effect_object.h"

#include <cstdint>

namespace pal4 {

class IEffectObjectUpdateRuntime {
public:
    virtual ~IEffectObjectUpdateRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    [[nodiscard]] virtual std::uint32_t Advance(std::uint32_t instance,
                                                 float elapsed) = 0;
    [[nodiscard]] virtual std::uint32_t Prepare(std::uint32_t instance) = 0;
    [[nodiscard]] virtual std::uint32_t Activate(std::uint32_t instance) = 0;
    virtual void AddFrameChild(std::uint32_t parent,
                               std::uint32_t child) = 0;
    virtual void RemoveFrameChild(std::uint32_t frame) = 0;
    [[nodiscard]] virtual std::uint32_t DestroyFrame(std::uint32_t frame) = 0;
    virtual void CleanupInstance(std::uint32_t instance,
                                 std::uint32_t context) = 0;
    virtual void DestroyInstance(std::uint32_t instance,
                                 std::uint32_t context) = 0;
};

class EffectObjectUpdateRuntimeStub final : public IEffectObjectUpdateRuntime {
public:
    [[nodiscard]] std::uint32_t ReadWord(std::uint32_t) override { return 0u; }
    [[nodiscard]] std::uint8_t ReadByte(std::uint32_t) override { return 0u; }
    void WriteWord(std::uint32_t, std::uint32_t) override {}
    void WriteByte(std::uint32_t, std::uint8_t) override {}
    [[nodiscard]] std::uint32_t Advance(std::uint32_t, float) override {
        return 0u;
    }
    [[nodiscard]] std::uint32_t Prepare(std::uint32_t) override { return 0u; }
    [[nodiscard]] std::uint32_t Activate(std::uint32_t) override { return 0u; }
    void AddFrameChild(std::uint32_t, std::uint32_t) override {}
    void RemoveFrameChild(std::uint32_t) override {}
    [[nodiscard]] std::uint32_t DestroyFrame(std::uint32_t) override {
        return 0u;
    }
    void CleanupInstance(std::uint32_t, std::uint32_t) override {}
    void DestroyInstance(std::uint32_t, std::uint32_t) override {}
};

class EffectObjectUpdater final {
public:
    // IDA 0x610850; entry_eax and context preserve native register inputs.
    [[nodiscard]] static std::uint32_t Update(
        EffectObjectState& state, double elapsed, std::uint32_t entry_eax,
        std::uint32_t context, IEffectObjectUpdateRuntime& runtime);
    // IDA 0x610a30, entered through the owner +36 facet.
    static void Start(EffectObjectState& state, std::uint8_t mode,
                      std::uint32_t context,
                      IEffectObjectUpdateRuntime& runtime);
};

}  // namespace pal4
