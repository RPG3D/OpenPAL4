#pragma once
#include "pal4/script_shadow_control.h"

namespace pal4 {
class NativePlayerManagerSession;
class NativeScriptShadowRuntime final : public IScriptShadowRuntime {
public:
    NativeScriptShadowRuntime(NativePlayerManagerSession&,
                             std::function<std::uint32_t()> arena);
    std::uint32_t PlayerManager() override;
    std::uint32_t ForAllPlayers(std::uint32_t, const PlayerCallback&, std::uint32_t) override;
    std::uint32_t ArenaManager() override;
    std::uint32_t ReadWord(std::uint32_t) override;
    void SetPlayerShadow(std::uint32_t, std::uint32_t, std::uint32_t) override;
private:
    NativePlayerManagerSession& players_;
    std::function<std::uint32_t()> arena_;
};
}
