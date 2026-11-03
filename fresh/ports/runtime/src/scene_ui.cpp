#include "pal4/portable/scene_ui.h"
#include "pal4/ui_frame_manager_singleton.h"
#include <ostream>
#include <stdexcept>

namespace pal4::portable {
namespace {
class StateEvent final:public pal4::IUiFrameManagerStateEventRuntime {
public:
    explicit StateEvent(SceneUi& ui):ui_(ui){}
    std::uint32_t GameInstance() override{return 0x1000;}
    std::uint32_t ReadWord(std::uint32_t address) override {
        if(address!=0x118c)throw std::logic_error("invalid scene UI state address");
        return 0x2000;
    }
    std::uint32_t QueryGameState(std::uint32_t controller) override {
        if(controller!=0x2000) {throw std::logic_error("invalid scene UI controller");}
        return ui_.game_state;
    }
    std::uint8_t ProcessEvent(std::uint32_t tree,std::uint32_t key,std::uint32_t type,std::uint32_t zero) override {
        if(tree!=0x3104 || key!=0x8a80a4 || zero || type>1)throw std::logic_error("invalid scene HUD event");
        ui_.hud_visible=type!=0;if(type)ui_.help_topic=1;++ui_.scene_events;return 1;
    }
private:SceneUi& ui_;
};
}
void SceneUi::ShowScene(bool visible) {StateEvent runtime(*this);(void)pal4::UiFrameManagerSingleton::ProcessStateEvent(0x3000,visible,runtime);}
void SceneUi::HideInteraction(){jump_hint_visible=false;progress_visible=false;++interaction_resets;}
void WriteSceneUiReport(std::ostream& output,const SceneUi& ui) {
    output<<"{\"game_state\":"<<ui.game_state<<",\"hud_visible\":"<<(ui.hud_visible?"true":"false")
        <<",\"help_topic\":"<<ui.help_topic
        <<",\"minimap_expanded\":"<<(ui.minimap_expanded?"true":"false")
        <<",\"jump_hint_visible\":"<<(ui.jump_hint_visible?"true":"false")<<",\"progress_visible\":"<<(ui.progress_visible?"true":"false")
        <<",\"scene_events\":"<<ui.scene_events<<",\"interaction_resets\":"<<ui.interaction_resets
        <<",\"default_cursor\":"<<(ui.cursor=="default.cur"?"true":"false")<<"}";
    if(!output)throw std::runtime_error("cannot write scene UI report");
}
} // namespace pal4::portable
