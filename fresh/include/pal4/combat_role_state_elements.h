#pragma once
#include <cstdint>
#include <string>
#include <string_view>

namespace pal4 {
enum class CombatRoleStateClickEvent { ButtonClicked, MouseClicked };

// Original54F940 control construction. Actor, timer and event-list ownership
// belong to the surrounding game frame, not to the DLL control adapter.
class ICombatRoleStateElementsRuntime {
public:
    virtual ~ICombatRoleStateElementsRuntime() = default;
    virtual std::uint32_t ReadWord(std::uint32_t) = 0;
    virtual void WriteWord(std::uint32_t,std::uint32_t) = 0;
    virtual std::uint32_t Allocate(std::uint32_t) = 0;
    virtual std::uint32_t GetImage(std::string_view,std::string_view) = 0;
    virtual std::string ConvertToUtf8(const std::string&) = 0;
    virtual std::uint32_t GetWindowByName(std::uint32_t,const std::string&) = 0;
    virtual void SetFont(std::uint32_t,std::string_view) = 0;
    virtual float GetWidth(std::uint32_t) = 0;
    virtual std::uint32_t SubscribeClick(std::uint32_t,CombatRoleStateClickEvent,std::uint32_t) = 0;
    virtual void ReleaseSubscription(std::uint32_t) = 0;
    virtual std::uint32_t CleanupTemporaryString() = 0;
};
}
