#pragma once

#include <cstdint>

#include "pal4/equipment_window.h"

namespace pal4 {

class IEquipmentMouseMoveRegistrationRuntime {
public:
    virtual ~IEquipmentMouseMoveRegistrationRuntime() = default;
    [[nodiscard]] virtual std::uint32_t AllocateCallback(
        std::uint32_t bytes) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t SubscribeMouseMove(
        std::uint32_t window, std::uint32_t callback) = 0;
    [[nodiscard]] virtual std::uint32_t ReferenceCount(
        std::uint32_t reference) = 0;
    virtual void SetReferenceCount(std::uint32_t reference,
                                   std::uint32_t count) = 0;
    virtual void DeleteReference(std::uint32_t reference,
                                 std::uint32_t flags) = 0;
};
class IEquipmentMouseMoveHandlingRuntime {
public:
    virtual ~IEquipmentMouseMoveHandlingRuntime()=default;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual bool IsDisabled(std::uint32_t window) = 0;
    virtual void SetHoverFlag(std::uint32_t window, bool hovered) = 0;
    virtual void RequestRedraw(std::uint32_t window) = 0;
    [[nodiscard]] virtual int CallMemberVirtual(std::uint32_t member,
                                                 std::uint32_t slot) = 0;
};
class IEquipmentMouseMoveRuntime : public IEquipmentMouseMoveRegistrationRuntime,
    public IEquipmentMouseMoveHandlingRuntime {
public:
    ~IEquipmentMouseMoveRuntime() override=default;
};

class EquipmentMouseMoveRuntimeStub : public IEquipmentMouseMoveRuntime {
public:
    std::uint32_t AllocateCallback(std::uint32_t) override { return 0; }
    void WriteWord(std::uint32_t, std::uint32_t) override {}
    std::uint32_t SubscribeMouseMove(std::uint32_t,
                                     std::uint32_t) override { return 0; }
    std::uint32_t ReferenceCount(std::uint32_t) override { return 0; }
    void SetReferenceCount(std::uint32_t, std::uint32_t) override {}
    void DeleteReference(std::uint32_t, std::uint32_t) override {}
    std::uint32_t ReadWord(std::uint32_t) override { return 0; }
    bool IsDisabled(std::uint32_t) override { return true; }
    void SetHoverFlag(std::uint32_t, bool) override {}
    void RequestRedraw(std::uint32_t) override {}
    int CallMemberVirtual(std::uint32_t, std::uint32_t) override {
        return 0;
    }
};

class EquipmentMouseMoveEvents final {
public:
    static constexpr std::uint32_t kCallbackVtable = 0x8438f0u;
    static constexpr std::uint32_t kCallbackHandler = 0x4543a0u;

    [[nodiscard]] static bool Register(EquipmentWindowState& state,
                                       std::uint32_t member_address,
                                       IEquipmentMouseMoveRegistrationRuntime& runtime);
    [[nodiscard]] static bool Handle(EquipmentWindowState& state,
                                     std::uint32_t event,
                                     IEquipmentMouseMoveHandlingRuntime& runtime);
    [[nodiscard]] static int InvokeMemberVirtual(
        std::uint32_t member,
        IEquipmentMouseMoveHandlingRuntime& runtime);
};

}  // namespace pal4
