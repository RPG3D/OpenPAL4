#pragma once

#include <cstdint>
#include <vector>

namespace pal4 {

class ISystemManagerAoERuntime {
public:
    virtual ~ISystemManagerAoERuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual float ObjectRate(std::uint32_t object) = 0;
    virtual void SetObjectProperty(std::uint32_t object, float value) = 0;
    [[nodiscard]] virtual float GameDbFloat(std::uint32_t index) = 0;
    [[nodiscard]] virtual std::uint32_t RandomNumber() = 0;
    [[nodiscard]] virtual std::uint32_t ObjectGroup(std::uint32_t object) = 0;
    virtual void FinalizeManager(std::uint32_t manager) = 0;
    [[nodiscard]] virtual std::uint32_t ReleaseOrdered(
        const std::vector<std::uint32_t>& objects) = 0;
    [[nodiscard]] virtual std::uint32_t InvokeManagerSlot4(
        std::uint32_t manager) = 0;
};

class SystemManagerAoE final {
public:
    // IDA 0x5280d0 and 0x5283d0.
    [[nodiscard]] static std::uint32_t Distribute(
        std::uint32_t manager, std::uint32_t group_argument,
        ISystemManagerAoERuntime& runtime);
    [[nodiscard]] static std::uint32_t InvokeManager(
        std::uint32_t manager, ISystemManagerAoERuntime& runtime);
};

}  // namespace pal4
