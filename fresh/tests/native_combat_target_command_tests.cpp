#include "pal4/native_combat_target_command_services.h"
#include "pal4/native_game_database.h"
#include "pal4/native_property_record_catalog.h"
#include "pal4/native_skill_system_owner.h"
#include "pal4/native_player_combat_events.h"
#include "pal4/native_script_event_router.h"
#include "pal4/input_manager.h"
#include "pal4/random_number_generator.h"
#include <algorithm>
#include <iostream>
#include <stdexcept>
namespace {
void Check(bool v,const char* text){if(!v)throw std::runtime_error(text);}
struct Actors final : pal4::INativeCombatTargetSelectionActors {
    std::array<std::uint32_t,130> combat{};
    std::array<std::uint32_t,205> actor{};
    std::uint32_t refresh{},unselect{};
    const char name[6]="actor";
    std::uint32_t CombatSystemAddress() override{return reinterpret_cast<std::uint32_t>(combat.data());}
    std::uint32_t RefreshSelection(std::uint32_t c) override{
        ++refresh;reinterpret_cast<std::uint32_t*>(c)[16]=reinterpret_cast<std::uint32_t>(actor.data());return 1u;
    }
    std::uint32_t SetTargetSelected(std::uint32_t a,std::uint32_t v) override{
        Check(a==reinterpret_cast<std::uint32_t>(actor.data()) && !v,"actual supplied actor receiver");++unselect;return 0u;
    }
    std::uint32_t ActorNameAddress(std::uint32_t a) override{
        Check(a==reinterpret_cast<std::uint32_t>(actor.data()),"name lookup validates supplied actor owner");return reinterpret_cast<std::uint32_t>(name);
    }
};
std::size_t Column(const pal4::NativeGameDbTable& t,const char* name){
    auto it=std::find_if(t.fields.begin(),t.fields.end(),[&](const auto& f){return f.name==name;});
    Check(it!=t.fields.end(),"reference original column");return it-t.fields.begin();
}
}
int main(int argc,char** argv){try{
    Check(argc==3,"usage: native_combat_target_command_tests GAME_ROOT OUTPUT_DIR");
    pal4::NativeGameDatabase database;Check(database.Load(argv[1]),"load actual pal4db.db");
    pal4::NativePropertyRecordCatalog property(database);
    pal4::InputManager input;pal4::RandomNumberGenerator random;pal4::NativePlayerCombatEvents properties;
    pal4::NativeSkillSystemOwner skill(random,properties);
    Actors actors;pal4::NativeCombatTargetCommandServices commands(database,property,input,skill,actors);
    pal4::NativeScriptEventRouter router;pal4::NativeCombatTargetSelectionImage image;
    pal4::NativeCombatTargetSelection selection(router,commands,image);
    image.words[10]=reinterpret_cast<std::uint32_t>(actors.actor.data())+716u;
    image.words[13]=4u;Check(commands.DetermineTargetMode(selection.Address())==1u &&
       (*reinterpret_cast<std::uint8_t*>(selection.Address()+80u))==1u,"command4 works without any DB/error singleton acquisition");
    image.words[13]=99u;bool rejected=false;try{(void)commands.DetermineTargetMode(selection.Address());}catch(const std::logic_error&){rejected=true;}
    Check(rejected,"invalid command requires initialized actual SkillSystem");
    std::filesystem::create_directories(argv[2]);Check(skill.Construct(argv[2])!=0u,"construct actual SkillSystem");
    std::size_t rows{},dead{},valid{},invalid{};std::array<std::uint32_t,4> playable{};
    const std::array<const char*,3> tables{"Magic","Stunt","Property"};
    for(std::uint32_t kind=0u;kind<3u;++kind){
        const auto& table=database.Tables().at(tables[kind]);const auto id=Column(table,"ID"),ai=Column(table,"AI_CMD_TYPE"),target=Column(table,"TARGET");
        for(const auto& row:table.rows){
            const auto key=std::get<std::uint32_t>(row[id]),category=std::get<std::uint32_t>(row[ai]),type=std::get<std::uint32_t>(row[target]);
            image.words[12]=key;image.words[13]=kind;
            const auto mode=commands.DetermineTargetMode(selection.Address());
            Check(mode==(type==1u?0u:type==2u?1u:2u),"every original record target mode");
            Check(*reinterpret_cast<std::uint8_t*>(selection.Address()+80u)==(category==5u?0u:1u),"every original record death eligibility");
            ++rows;if(category==5u)++dead;if(mode==2u)++invalid;else{++valid;playable={image.words[10],0u,key,kind};}
        }
        image.words[12]=0xfffffff0u;image.words[13]=kind;
        Check(commands.DetermineTargetMode(selection.Address())==2u &&
             *reinterpret_cast<std::uint8_t*>(selection.Address()+80u)==1u,"missing original record returns mode2 and restricted eligibility");
    }
    Check(rows && dead && valid && invalid,"original DB covers live/dead/valid/invalid modes");
    input.WriteByte(0x68u,255u);input.WriteDword(0x21d4u,1u);input.WriteDword(0x21c8u,1u);input.WriteDword(0x10u,123u);
    (void)router.Dispatch("start_select_target",reinterpret_cast<std::uint32_t>(playable.data()));
    Check(actors.refresh==1u && !input.ReadByte(0x68u) && !input.ReadDword(0x21d4u) &&
          !input.ReadDword(0x21c8u) && input.ReadDword(0x10u)==123u,
          "real start event performs DB selection then actual InputManager reset preserving parameters");
    (void)router.Dispatch("stop_select_target",0u);Check(actors.unselect==1u && !image.words[16],"actual service adapter forwards actor cleanup");
    rejected=false;try{(void)commands.InitializeInput(commands.InputManagerAddress()+4u);}catch(const std::invalid_argument&){rejected=true;}Check(rejected,"foreign input owner rejected");
    selection.Close();skill.Destroy();
    std::cout<<"Native target commands: original rows="<<rows<<" AI5="<<dead<<" valid="<<valid<<" invalid="<<invalid
             <<", shared mode algorithm and real input reset/event integration pass; roster/actors remain protocol fixture\n";
    return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
