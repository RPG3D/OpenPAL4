#pragma once

#include "pal4/effect_system_object_f.h"

#include <cstdint>

namespace pal4 {

class IEffectSystemObjectFShutdownRuntime {
public:
    virtual ~IEffectSystemObjectFShutdownRuntime() = default;
    virtual void ClearComponent(EffectSystemObjectFState& state,
                                std::uint32_t offset) = 0;
    virtual void CleanupXml(EffectSystemObjectFState& state,
                            std::uint32_t offset) = 0;
    virtual void DeinitializeObject(std::uint32_t object) = 0;
    virtual void FreeObject(std::uint32_t object) = 0;
    virtual void ProcessEvent(std::uint32_t system,
                              EffectSystemObjectFState& state) = 0;
    [[nodiscard]] virtual std::int32_t FinalizeObject(
        EffectSystemObjectFState& state, std::uint32_t argument) = 0;
};

class EffectSystemObjectFShutdownRuntimeStub final
    : public IEffectSystemObjectFShutdownRuntime {
public:
    void ClearComponent(EffectSystemObjectFState&,
                        std::uint32_t) override {}
    void CleanupXml(EffectSystemObjectFState&,
                    std::uint32_t) override {}
    void DeinitializeObject(std::uint32_t) override {}
    void FreeObject(std::uint32_t) override {}
    void ProcessEvent(std::uint32_t,
                      EffectSystemObjectFState&) override {}
    [[nodiscard]] std::int32_t FinalizeObject(
        EffectSystemObjectFState&, std::uint32_t) override { return 0; }
};

class EffectSystemObjectFShutdown final {
public:
    // IDA 0x605e00.
    [[nodiscard]] static std::int32_t ShutdownAndFinalize(
        EffectSystemObjectFState& state,
        IEffectSystemObjectFShutdownRuntime& runtime);
};

}  // namespace pal4
