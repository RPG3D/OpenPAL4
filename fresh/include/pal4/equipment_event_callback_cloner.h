#pragma once

#include <cstdint>

namespace pal4 {

class IEquipmentEventCallbackCloneRuntime {
public:
    virtual ~IEquipmentEventCallbackCloneRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Allocate(
        std::uint32_t bytes) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
};

class EquipmentEventCallbackCloneRuntimeStub :
    public IEquipmentEventCallbackCloneRuntime {
public:
    std::uint32_t Allocate(std::uint32_t) override { return 0; }
    std::uint32_t ReadWord(std::uint32_t) override { return 0; }
    void WriteWord(std::uint32_t, std::uint32_t) override {}
};

class EquipmentEventCallbackCloner final {
public:
    [[nodiscard]] static std::uint32_t CloneCommand(
        std::uint32_t source,
        IEquipmentEventCallbackCloneRuntime& runtime);
    [[nodiscard]] static std::uint32_t CloneEvent(
        std::uint32_t source,
        IEquipmentEventCallbackCloneRuntime& runtime);
};

}  // namespace pal4
