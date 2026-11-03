#include "pal4/native_script_combat_config.h"
#include "pal4/native_combat_system_tail.h"
#include "pal4/native_angelscript_engine.h"
#include "pal4/native_game_script_callbacks.h"
#include <iostream>
#include <stdexcept>

int main() {
    const auto check=[](bool ok) { if(!ok) throw std::runtime_error("native combat configuration test"); };
    try {
        pal4::NativeScriptCombatConfig config;
        check(config.Snapshot().words == std::array<std::uint32_t,5>{0,0,0,0,1});
        check(config.Snapshot().flags == std::array<std::uint8_t,3>{1,0,0});
        pal4::NativeAngelScriptEngine engine;
        pal4::NativeGameScriptServices services{};
        services.combat_config=&config; services.combat_text=&config;
        pal4::NativeGameScriptCallbacks callbacks(services); engine.SetCallbacks(&callbacks);
        constexpr char code[]=R"(
            void BgmOnly() { giConfigCombatBgm("P44"); }
            void CameraOnly() { giConfigCombatCamera("CA6"); }
            void GroundOnly() { giConfigCombatGroundCamera("CA13"); }
            void First() { giConfigCombatBgm("P39"); giConfigCombatCamera("CA7");
                giConfigCombatGroundCamera("CA12"); giConfigCombatParam(true,4,2579,2); }
            void Replace() { giConfigCombatBgm(""); giConfigCombatCamera("CA8");
                giConfigCombatGroundCamera("CA13"); giConfigCombatParam(false,-1,-2,3); }
            void TypeZero() { giConfigCombatParam(true,0,0,0); }
            void TypeOne() { giConfigCombatParam(true,0,0,1); }
        )";
        check(engine.Raw()->AddScriptSection("combat","combat.cs",code,sizeof(code)-1)>=0);
        check(engine.Raw()->Build("combat")>=0);
        asIScriptContext* context{};check(engine.Raw()->CreateContext(&context)>=0 && context);
        const auto run=[&](const char* name) {
            check(context->Prepare(engine.FunctionId("combat",name))>=0);
            check(context->Execute()==asEXECUTION_FINISHED);
        };
        try {
            // Use the actual M07B sequence, checking each public SDK call
            // independently. No player-favor service is bound: these calls
            // must dispatch to combat text, even before a live tail exists.
            const auto opening=[&] {
                run("BgmOnly");
                auto value=config.Snapshot();
                check(value.texts[0].value=="P44");
                const auto prior_ground=value.texts[2].value;
                run("CameraOnly");value=config.Snapshot();
                check(value.texts[0].value=="P44" && value.texts[1].value=="CA6" &&
                      value.texts[2].value==prior_ground);
                run("GroundOnly");run("TypeZero");value=config.Snapshot();
                check(value.texts[0].value=="P44" && value.texts[1].value=="CA6" &&
                      value.texts[2].value=="CA13" && value.words[4]==0u);
            };
            opening();
            run("First");run("First");
            const auto first=config.Snapshot();
            check(first.words==std::array<std::uint32_t,5>{4,2579,0,0,2});
            check(first.flags==std::array<std::uint8_t,3>{1,1,1});
            check(first.texts[0].value=="P39" && first.texts[1].value=="CA7" && first.texts[2].value=="CA12");
            run("Replace");
            check(config.Snapshot().words==std::array<std::uint32_t,5>{0xffffffffu,0xfffffffeu,0,0,2});
            check(config.Snapshot().flags==std::array<std::uint8_t,3>{0,1,1});
            check(config.Snapshot().texts[0].value.empty() && config.Snapshot().texts[1].value=="CA8" && config.Snapshot().texts[2].value=="CA13");
            check(first.texts[0].value=="P39"); // Replacing the owner cannot mutate a copied snapshot.
            run("TypeZero");check(config.Snapshot().words[4]==0u);
            run("TypeOne");check(config.Snapshot().words[4]==1u);
            bool rejected=false;
            try { (void)config.ReadWord(config.CombatSystem()+436u+1u); }
            catch(const std::invalid_argument&) {rejected=true;}check(rejected);
            rejected=false;try { (void)config.ReadTextProperty(config.CombatSystem()+500u); }
            catch(const std::invalid_argument&) {rejected=true;}check(rejected);
            std::array<std::uint32_t,20> image{};image.front()=0x13579bdfu;image.back()=0x2468ace0u;
            pal4::NativeCombatSystemTail tail(std::span<std::uint32_t,18>(image.data()+1u,18u));
            config.Bind(tail);
            check(config.CombatSystem()==tail.Address()-432u && tail.Snapshot().words==config.Snapshot().words);
            opening();
            run("First");
            check(image[1]==4u && image[2]==2579u && image[5]==2u && tail.Snapshot().flags==std::array<std::uint8_t,3>{1,1,1});
            check(tail.Text(tail.Address()+20u)=="P39" && tail.Text(tail.Address()+36u)=="CA7" && tail.Text(tail.Address()+52u)=="CA12");
            config.SetCombatProperty110(1u);check(image[3]==1u);
            tail.Reset(); // Original EndCombat resets the same state read by subsequent script calls.
            check(config.Snapshot().words==std::array<std::uint32_t,5>{0,0,0,0,1});
            run("Replace");check(tail.Snapshot().words[4]==1u && tail.Snapshot().flags[0]==0u);
            check(image.front()==0x13579bdfu && image.back()==0x2468ace0u);
            config.Unbind();tail.Close();
            context->Release();
        } catch(...) {context->Release();throw;}
        std::cout<<"Compiled combat SDK configuration: original M01 values, replacement/empty text, snapshot lifetime, flags, signed words, type fallback and owner rejection passed\n";
        return 0;
    } catch(const std::exception& e) {std::cerr<<e.what()<<'\n';return 1;}
}
