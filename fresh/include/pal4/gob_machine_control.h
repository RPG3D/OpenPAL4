#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <string_view>

namespace pal4 {

class IGobMachineVisibilityRuntime {
public:
    virtual ~IGobMachineVisibilityRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t ClearTaskCollection(
        std::uint32_t collection) = 0;
    virtual void SetVisible(std::uint32_t object,
                            std::uint32_t visible) = 0;
    [[nodiscard]] virtual std::uint32_t TriggerTaskRun(
        std::uint32_t object, const std::array<float, 3>& position) = 0;
};

class IGobMachineConfigRuntime {
public:
    // Original virtual +0x98: LocalFloat selects its own inherited task node.
    [[nodiscard]] virtual std::string_view MachineConfigSection() const {
        return "PAL4_GameObject-machine";
    }
    virtual ~IGobMachineConfigRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t FindXmlChild(
        std::uint32_t parent, std::string_view name) = 0;
    virtual void ApplyBaseParameters(std::uint32_t object) = 0;
    [[nodiscard]] virtual std::int32_t ReadXmlInt(
        std::uint32_t node, std::string_view name,
        std::int32_t fallback) = 0;
    [[nodiscard]] virtual std::int32_t XmlChildCount(
        std::uint32_t node) = 0;
    [[nodiscard]] virtual std::uint32_t XmlChildAt(
        std::uint32_t node, std::int32_t index) = 0;
    [[nodiscard]] virtual std::string ReadXmlString(
        std::uint32_t node, std::string_view name,
        std::string_view fallback) = 0;
    virtual void AssignActionName(std::uint32_t record,
                                  std::string_view value) = 0;
};

class IGobMachineControlRuntime : public IGobMachineVisibilityRuntime,
                                  public IGobMachineConfigRuntime {
public:
    ~IGobMachineControlRuntime() override = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) override = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) override = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    virtual void StartTaskRun(std::uint32_t object) = 0;
    virtual void MarkEventProcessed(std::uint32_t event) = 0;
    virtual void SetPosition(std::uint32_t object,
                             std::uint32_t vector, std::uint32_t mode) = 0;
    virtual void SetRotation(std::uint32_t object,
                             std::uint32_t vector, std::uint32_t mode) = 0;
    virtual void InvokeTaskSlot(std::uint32_t task,
                                std::uint32_t slot) = 0;

};

}  // namespace pal4
