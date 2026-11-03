#pragma once

#include <cstdint>

namespace pal4 {

class IPlayerManagerSlotScriptRuntime {
public:
    virtual ~IPlayerManagerSlotScriptRuntime() = default;
    [[nodiscard]] virtual std::uint32_t GetValue(
        std::uint32_t object, std::uint32_t index) = 0;
    [[nodiscard]] virtual std::uint32_t GetMaximum(
        std::uint32_t object, std::uint32_t index) = 0;
    [[nodiscard]] virtual std::uint32_t SetValue(
        std::uint32_t object, std::uint32_t index,
        std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t ReadDword(
        std::uint32_t address) = 0;
    virtual void WriteDword(std::uint32_t address,
                            std::uint32_t value) = 0;
    virtual void ProcessInventory(std::uint32_t slots,
                                  std::uint32_t copy,
                                  std::uint32_t original) = 0;
    virtual void ProcessSkills(std::uint32_t object,
                               std::uint32_t mode,
                               std::uint32_t marker) = 0;
};

class PlayerManagerSlotScriptRuntimeStub :
    public IPlayerManagerSlotScriptRuntime {
public:
    std::uint32_t GetValue(std::uint32_t,
                           std::uint32_t) override;
    std::uint32_t GetMaximum(std::uint32_t,
                             std::uint32_t) override;
    std::uint32_t SetValue(std::uint32_t, std::uint32_t,
                           std::uint32_t) override;
    std::uint32_t ReadDword(std::uint32_t) override;
    void WriteDword(std::uint32_t, std::uint32_t) override;
    void ProcessInventory(std::uint32_t, std::uint32_t,
                          std::uint32_t) override;
    void ProcessSkills(std::uint32_t, std::uint32_t,
                       std::uint32_t) override;
};

class PlayerManagerSlotScriptData final {
public:
    // IDA 0x43f4b0: refresh a script subobject and clamp saved properties.
    [[nodiscard]] static std::uint32_t Refresh(
        std::uint32_t object, IPlayerManagerSlotScriptRuntime& runtime);
};

}  // namespace pal4
