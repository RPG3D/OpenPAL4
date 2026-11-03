#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace pal4 {

struct GobMachineTaskParameter {
    std::string key;
    std::variant<std::int32_t, float, std::string> value;
};

struct GobMachineTaskDefinition {
    std::int32_t type = -1;
    std::int32_t wait_mode = 0;
    float wait_time = 0.0f;
    float expect_time = 0.0f;
    std::vector<GobMachineTaskParameter> parameters;
};

class IGobMachineTaskRuntime {
public:
    virtual ~IGobMachineTaskRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    virtual void WriteFloat(std::uint32_t address, float value) = 0;
    virtual void ClearPrimaryTasks(std::uint32_t container) = 0;
    virtual void EraseTaskPointerRange(std::uint32_t record,
                                      std::uint32_t begin,
                                      std::uint32_t end) = 0;
    [[nodiscard]] virtual std::uint32_t GetMachineConfigNode(
        std::uint32_t object) = 0;
    [[nodiscard]] virtual std::uint32_t FindXmlChild(
        std::uint32_t parent, std::string_view name) = 0;
    [[nodiscard]] virtual std::int32_t XmlChildCount(
        std::uint32_t parent) = 0;
    [[nodiscard]] virtual std::uint32_t XmlChildAt(
        std::uint32_t parent, std::int32_t index) = 0;
    [[nodiscard]] virtual std::int32_t ReadXmlInt(
        std::uint32_t node, std::string_view key,
        std::int32_t fallback) = 0;
    [[nodiscard]] virtual float ReadXmlFloat(
        std::uint32_t node, std::string_view key,
        float fallback) = 0;
    [[nodiscard]] virtual std::string ReadXmlString(
        std::uint32_t node, std::string_view key,
        std::string_view fallback) = 0;
    [[nodiscard]] virtual std::uint32_t CreateTask(
        std::int32_t type) = 0;
    virtual void RegisterTask(std::uint32_t container,
                              std::uint32_t task,
                              const GobMachineTaskDefinition& definition) = 0;
    virtual void InitializeTask(std::uint32_t task,
                                std::uint32_t object,
                                std::uint32_t context,
                                const GobMachineTaskDefinition& definition) = 0;
    virtual void AppendTaskPointer(std::uint32_t record,
                                   std::uint32_t task) = 0;
    [[nodiscard]] virtual float RandomWaitDuration(
        float minimum, float maximum) = 0;
};

}  // namespace pal4
