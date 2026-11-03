#pragma once
#include "pal4/pal_actor_control_game_event.h"
#include <functional>
namespace pal4::portable {
struct InteractionHintView {
    std::uint64_t generation{},id{},revision{};bool visible{};
    std::string text;float x{},y{},duration{};
};
// 文本lookup返回已解码UTF8；窗口不改变角色索引或脚本状态。
class InteractionHints final:public pal4::IActorControlGameEventRuntime {
public:
    using Lookup=std::function<std::optional<std::string>(std::uint32_t)>;
    using Play=std::function<void(std::string_view,std::uint32_t)>;
    InteractionHints(std::uint64_t generation,Lookup,Play);
    const InteractionHintView& View() const{return view_;}
    void ShowMessage(std::string_view text,float x,float y,float duration);
    void Advance(float seconds);
    void LayoutChanged(){if(view_.visible)++view_.revision;}
    bool Close(std::uint64_t generation,std::uint64_t id);
    std::uint32_t CreateHintWindow() override;
    std::optional<std::string> TriggerText(std::uint32_t id) override;
    std::string ConvertToUtf8(std::string_view text) override;
    void InitializeHintWindow(std::uint32_t,std::string_view,float,float,float,std::uint32_t,std::uint32_t) override;
    void PlayHintSound(std::string_view,std::uint32_t) override;
private:
    Lookup lookup_;Play play_;InteractionHintView view_;float elapsed_{};
};
}
