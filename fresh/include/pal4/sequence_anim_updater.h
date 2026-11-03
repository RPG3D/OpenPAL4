#pragma once

#include "pal4/sequence_anim_object.h"

#include <array>
#include <cstdint>

namespace pal4 {

class ISequenceAnimUpdateRuntime {
public:
    virtual ~ISequenceAnimUpdateRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadColor(
        const SequenceAnimObjectState& state) = 0;
    [[nodiscard]] virtual std::int32_t Ftol(double value) = 0;
    virtual void SetColor(SequenceAnimObjectState& state,
                          std::uint32_t rgba) = 0;
    [[nodiscard]] virtual std::uint32_t Geometry(std::uint32_t resource) = 0;
    virtual void UpdateGeometry(std::uint32_t geometry,
                                const std::array<std::uint32_t, 4>& corners) = 0;
    [[nodiscard]] virtual std::uint32_t SystemResource(
        std::uint32_t system) = 0;
    virtual void UpdateState(std::uint32_t resource, double elapsed,
                             std::uint32_t system_resource) = 0;
};

class SequenceAnimUpdateRuntimeStub final : public ISequenceAnimUpdateRuntime {
public:
    [[nodiscard]] std::uint32_t ReadColor(
        const SequenceAnimObjectState&) override { return 0u; }
    [[nodiscard]] std::int32_t Ftol(double) override { return 0; }
    void SetColor(SequenceAnimObjectState&, std::uint32_t) override {}
    [[nodiscard]] std::uint32_t Geometry(std::uint32_t) override { return 0u; }
    void UpdateGeometry(std::uint32_t,
                        const std::array<std::uint32_t, 4>&) override {}
    [[nodiscard]] std::uint32_t SystemResource(std::uint32_t) override { return 0u; }
    void UpdateState(std::uint32_t, double, std::uint32_t) override {}
};

class SequenceAnimUpdater final {
public:
    // IDA 0x616210.
    static void Update(SequenceAnimObjectState& state, double elapsed,
                       ISequenceAnimUpdateRuntime& runtime);
};

}  // namespace pal4
