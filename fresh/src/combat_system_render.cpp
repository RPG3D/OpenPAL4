#include "pal4/combat_system_render.h"

#include <iomanip>
#include <sstream>

namespace pal4 {

std::uint32_t CombatSystemRender::PlaySoundEffect(
    const std::uint32_t sound, const std::uint32_t second,
    const std::uint32_t third, ICombatSystemRenderRuntime& runtime) {
    runtime.PlaySound(sound, second, third);
    return sound;
}

void CombatSystemRender::Render(const std::uint32_t object,
                                 const std::uint32_t frame,
                                 ICombatSystemRenderRuntime& runtime) {
    class Services final : public ICombatSystemFrameServices {
        std::uint32_t object;ICombatSystemRenderRuntime& runtime;
    public:
        Services(std::uint32_t o,ICombatSystemRenderRuntime& r):object(o),runtime(r){}
        bool Inactive()override{return runtime.ReadByte(object+516u)!=0u;}
        void World(std::uint32_t f)override{runtime.RenderWorld(f);}
        void GameObject(std::uint32_t f)override{runtime.RenderGameObject(runtime.ReadWord(object+424u),f);}
        void Actors(std::uint32_t f)override{runtime.RenderActors(runtime.ReadWord(f));}
        void Ui(std::uint32_t f)override{runtime.RenderUi(f);}
        void Console(std::uint32_t f)override{runtime.RenderConsole(runtime.ReadWord(f));}
        void Debug(std::uint32_t f)override{CombatSystemRender::DrawDebugInfo(object,f,runtime);}
    } services(object,runtime);
    RenderFrame(runtime.ReadWord(object+508u),frame,services);
}
void CombatSystemRender::RenderFrame(std::uint32_t state,std::uint32_t frame,ICombatSystemFrameServices& services){
    if(state==0u){services.Ui(frame);services.Console(frame);return;}
    if(state>6u)return;
    if(!services.Inactive()){services.World(frame);services.GameObject(frame);services.Actors(frame);}
    services.Ui(frame);services.Console(frame);services.Debug(frame);
}

void CombatSystemRender::DrawDebugInfo(
    const std::uint32_t object, const std::uint32_t frame,
    ICombatSystemDebugRuntime& runtime) {
    static_cast<void>(frame);
    if (runtime.ReadByte(object + 518u) == 0u) return;

    const auto game_object = runtime.ReadWord(object + 424u);
    const auto rendering_objects = runtime.ReadWord(game_object + 24u);
    const auto game_state = runtime.GetGameState();
    const auto effect_count = runtime.CountEffects(game_object);
    const auto fps = static_cast<std::int32_t>(
        runtime.ReadWord(game_state + 1348u));
    std::ostringstream line;
    line << "FPS: " << std::setw(3) << std::setfill('0') << fps
         << " PlayingEffect:" << rendering_objects
         << " RenderingObject:" << effect_count;
    runtime.DrawText(line.str(), 0u, 0u, 400u, 20u,
                     1u, 0xff00ff00u, 16u);
    runtime.DrawText("Bgm:" + runtime.GetBgmName(), 0u, 30u, 400u, 50u,
                     1u, 0xff00ff00u, 16u);
    runtime.DrawText("World:" + runtime.ReadCString(
                         runtime.ReadWord(object + 128u)),
                     0u, 60u, 400u, 80u, 1u, 0xff00ff00u, 16u);
}

}  // namespace pal4
