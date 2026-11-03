#pragma once

#include "pal4/object_movement_processor.h"

#include <array>
#include <cstdint>

namespace pal4 {

class IObjectMovementResetRuntime {
public:
    virtual ~IObjectMovementResetRuntime() = default;
    virtual void CallSlot24(ObjectMovementState& state) = 0;
    virtual void CallScalarSetter(ObjectMovementState& state,
                                  std::uint32_t slot,
                                  std::uint32_t bits) = 0;
    [[nodiscard]] virtual std::array<std::uint32_t, 3> GlobalVector() = 0;
    [[nodiscard]] virtual std::uint32_t MemberVtable(
        std::uint32_t member_address) = 0;
    [[nodiscard]] virtual std::uint32_t CallSlot18(
        ObjectMovementState& state, std::array<std::uint32_t, 3>& output) = 0;
    [[nodiscard]] virtual std::uint32_t CallMemberSlot4(
        std::uint32_t captured_vtable, std::uint32_t member_address,
        std::uint32_t vector_token) = 0;
};

class ObjectMovementResetRuntimeStub final : public IObjectMovementResetRuntime {
public:
    void CallSlot24(ObjectMovementState& state) override;
    void CallScalarSetter(ObjectMovementState& state, std::uint32_t slot,
                          std::uint32_t bits) override;
    std::array<std::uint32_t, 3> GlobalVector() override;
    std::uint32_t MemberVtable(std::uint32_t member_address) override;
    std::uint32_t CallSlot18(
        ObjectMovementState& state,
        std::array<std::uint32_t, 3>& output) override;
    std::uint32_t CallMemberSlot4(std::uint32_t captured_vtable,
                                  std::uint32_t member_address,
                                  std::uint32_t vector_token) override;
};

class ObjectMovementReset final {
public:
    [[nodiscard]] static std::uint32_t Reset(
        ObjectMovementState& state, IObjectMovementResetRuntime& runtime);
};

}  // namespace pal4
