#pragma once
#include "pal4/property_window_lifecycle.h"
#include <functional>
#include <memory>
#include <string_view>
namespace pal4 {
class NativeInventorySession;
class NativeRoleSelector;
class NativePlayerSkillRuntime;
struct PlayerManagerObjectState;
struct NativePropertyWindowUseServices {
    std::function<void(std::string_view,std::uint32_t)> sound;
    std::function<void(PropertyWindowState&,std::uint32_t,bool)> refresh;
    std::function<void(PropertyWindowState&)> earthball;
    std::function<void(PropertyWindowState&)> magic;
};
// Live item -> actual role selector -> original immediate Skill chain -> consume
// one item and refresh. All borrowed owners/page/services must outlive Close.
class NativePropertyWindowUse final {
public:
    NativePropertyWindowUse(NativeInventorySession&,PlayerManagerObjectState&,
        NativeRoleSelector&,NativePlayerSkillRuntime&,NativePropertyWindowUseServices);
    ~NativePropertyWindowUse();
    NativePropertyWindowUse(const NativePropertyWindowUse&)=delete;
    NativePropertyWindowUse& operator=(const NativePropertyWindowUse&)=delete;
    bool Show(PropertyWindowState&,std::uint32_t item);
    void Close();
private:
    class Runtime;
    std::shared_ptr<Runtime> runtime_;
};
}
