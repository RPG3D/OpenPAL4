#pragma once
#include "pal4/portable/actor.h"
#include <functional>
#include <array>
#include <cstdint>
#include <iosfwd>
#include <memory>
#include <span>
namespace pal4::portable {
using ActorMemberReader=std::function<std::optional<std::vector<std::uint8_t>>(std::string_view)>;
using ActorMessageCallback=std::function<void(std::span<const std::byte>)>;
using ActorClipLoader=std::function<std::shared_ptr<const AnimationClip>(std::string_view)>;
class ActorPlayback {
public:
    ActorPlayback(const Actor& actor,std::string model,ActorMemberReader read,ActorMessageCallback fallback={},bool retain_source_for_diagnostics=false,ActorClipLoader clip_loader={});
    ~ActorPlayback();
    ActorPlayback(const ActorPlayback&)=delete;
    ActorPlayback& operator=(const ActorPlayback&)=delete;
    // 借用实际对象的31-word内嵌动画视图。调用方保留vtable和集合头字节，
    // 动作/暂停/HOLD等其余字段由此播放器独占发布；解绑后才能释放视图。
    std::uint64_t BindAnimationImage(std::span<std::uint32_t,31>,std::uint32_t logical_address,std::uint32_t clump_token);
    // 正常解绑恢复绑定前全部31个word。外部覆盖时仅解除借用并返回false，
    // 不覆盖错误现场；调用方不可将未恢复图像直接交给原动画析构。
    bool UnbindAnimationImage(std::uint64_t binding);
    bool OwnsAnimationImage(std::uint64_t binding) const;
    bool Select(std::string_view action);
    bool SelectBlended(std::string_view action,float seconds);
    void Advance(float elapsed);
    // Original render-side embedded-animation callback; does not advance time.
    bool ProcessRenderAnimation(std::uint32_t camera);
    void SetRestart(bool enabled);
    void SetPause(bool enabled);
    void Seek(float seconds);
    float Duration() const;
    bool Completed() const;
    void AttachHoldListener();
    void RemoveHoldListener();
    std::string_view MessageName() const;
    std::string_view Action() const;
    int Clip() const;
    float Time() const;
    void Sample(std::vector<Vertex>& output,std::vector<Matrix>* frames=nullptr);
    void WriteReport(std::ostream& output,bool include_source=true) const;
private:struct Impl;std::unique_ptr<Impl> impl_;
};
}
