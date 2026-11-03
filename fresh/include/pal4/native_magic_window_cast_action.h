#pragma once
#include <cstdint>
#include <functional>
#include <memory>
#include <string_view>
namespace pal4 {
class NativeMagicWindowLayout;class NativeMagicWindowRecords;class NativeGameDatabase;
class NativeRoleSelector;class NativePlayerSkillRuntime;struct PlayerManagerObjectState;
struct NativeMagicCastServices {
    std::function<void(std::string_view)> sound;
    std::function<void(std::string_view,float,float,std::uint32_t,std::uint32_t)> message;
};
// Borrows the page, immutable records and real player/selector/SkillSystem owners.
// Retire before those dependencies. Original magic records are selector context.
class NativeMagicWindowCastAction final {
public:
    NativeMagicWindowCastAction(NativeMagicWindowLayout&,PlayerManagerObjectState&,
        const NativeMagicWindowRecords&,const NativeGameDatabase&,NativeRoleSelector&,
        NativePlayerSkillRuntime&,NativeMagicCastServices);
    ~NativeMagicWindowCastAction();
    NativeMagicWindowCastAction(const NativeMagicWindowCastAction&)=delete;
    NativeMagicWindowCastAction& operator=(const NativeMagicWindowCastAction&)=delete;
    bool Select(std::uint32_t magic_key);
    void Close();
private:
    class Runtime;std::shared_ptr<Runtime> runtime_;
};
}
