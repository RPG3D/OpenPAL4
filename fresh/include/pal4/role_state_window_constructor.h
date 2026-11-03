#pragma once

#include "pal4/role_state_window_initializer.h"

#include <cstdint>

namespace pal4 {

class IRoleStateWindowConstructorRuntime {
public:
    virtual ~IRoleStateWindowConstructorRuntime() = default;
    virtual void LoadLayout(RoleStateWindowState& state,
                            std::uint32_t name) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t Subscribe(
        std::uint32_t window, RoleMouseEvent event,
        std::uint32_t callback) = 0;
    virtual void ReleaseConnection(std::uint32_t connection) = 0;
};

class RoleStateWindowConstructor final {
public:
    static constexpr std::uint32_t kObjectVtable = 0x843ab0u;
    static constexpr std::uint32_t kDialogVtable = 0x843a98u;
    static constexpr std::uint32_t kButtonUpHandler = 0x476fc0u;
    static constexpr std::uint32_t kButtonMoveHandler = 0x4773f0u;
    static constexpr std::uint32_t kCallbackVtable = 0x843a90u;

    [[nodiscard]] static std::uint32_t Construct(
        RoleStateWindowState& state, std::uint32_t owner,
        std::uint32_t name, IRoleStateWindowInitRuntime& init_runtime,
        IRoleStateWindowConstructorRuntime& runtime);
};

}  // namespace pal4
