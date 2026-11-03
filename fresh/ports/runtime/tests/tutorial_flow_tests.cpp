#include "pal4/portable/tutorial.h"
#include "pal4/portable/settings.h"
#include "pal4/portable/game_services.h"
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <source_location>
#include <sstream>
#include <stdexcept>
using namespace pal4::portable;
namespace {
void Check(bool value,const std::source_location where=std::source_location::current()) {
    if(!value)throw std::runtime_error("tutorial flow failed at line "+std::to_string(where.line()));
}
struct Directory {
    std::filesystem::path path=std::filesystem::temp_directory_path()/
        ("pal4-tutorial-flow-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    Directory(){std::filesystem::create_directory(path);}
    ~Directory(){std::error_code ignored;std::filesystem::remove_all(path,ignored);}
};
void FinishTutorial(ScriptStack& stack,TutorialServices& tutorial,std::uint64_t generation) {
    const auto view=tutorial.View();Check(view.visible && view.waiting);
    Check(!tutorial.Close(generation+1,view.id) && !tutorial.Close(generation,view.id+1));
    Check(!tutorial.Advance());Check(tutorial.Close(generation,view.id));
    Check(!tutorial.Close(generation,view.id));
    const auto token=tutorial.Advance();Check(token && stack.Complete(*token));
    Check(!tutorial.Advance());
}
}
int main(){try{
    Directory directory;const auto path=directory.path/"pal4-port.set";
    // Independent old-format fixture; actual store must migrate without losing movie flags.
    const unsigned char legacy[]={0x50,0x34,0x53,0x54,1,0,0,0,1,0,0,0x80,0x17,0x6d,0x2e,0x3b};
    {std::ofstream file(path,std::ios::binary);file.write(reinterpret_cast<const char*>(legacy),sizeof(legacy));}
    FileSettingsStore store(path);auto state=store.Load();Check(state==Settings{0x80000001,true});
    state.help_seen[7]=0x80000000; // Help and tutorial owners must preserve each other's state.
    std::ostringstream trace;GameServices game(trace);unsigned unlocks{};game.unlock_player=[&]{++unlocks;};
    SettingsServices settings(game,state,store);const TutorialCatalog catalog{{10,"original tutorial"}};
    TutorialServices* owner=nullptr;unsigned saves{};
    TutorialServices tutorial(settings,catalog,23,state.tutorials_enabled,[&](unsigned key){
        Check(owner && !owner->View().visible && unlocks==0);
        settings.SaveTutorial(key);++saves;
        Check(std::filesystem::file_size(path)==64 && FileSettingsStore(path).Load()==state && state.HelpSeen(255));
    });owner=&tutorial;
    ScriptGlobals globals{};ScriptStack stack(globals);
    // Unit fixture only: never installed in place of the game's original CSB.
    stack.Compile("flow","void Probe(){giShowTutorial(10);giPlayerUnLock();}");
    Check(stack.Push("flow","void Probe()",tutorial).has_value());
    Check(stack.Tick(0)==ScriptState::suspended);
    Check(saves==1 && settings.Writes()==1 && unlocks==0 && tutorial.View().visible);
    for(unsigned n=0;n<3;++n){Check(!tutorial.Advance());Check(stack.Tick(0)==ScriptState::suspended && unlocks==0);}
    FinishTutorial(stack,tutorial,23);Check(unlocks==0);
    Check(stack.Tick(0)==ScriptState::finished && unlocks==1);
    Check(state.movie_flags==0x80000001 && FileSettingsStore(path).Load()==state);
    Check(stack.PopFinished());
    // Already-read tutorial still releases its own wait on the next update, without a window or save.
    Check(stack.Push("flow","void Probe()",tutorial).has_value());Check(stack.Tick(0)==ScriptState::suspended);
    Check(!tutorial.View().visible && saves==1);const auto repeated=tutorial.Advance();Check(repeated && stack.Complete(*repeated));
    Check(stack.Tick(0)==ScriptState::finished && unlocks==2);
    // Disabled option survives disk restart and suppresses only the tutorial window.
    settings.SetTutorialsEnabled(false);const auto disabled_state=FileSettingsStore(path).Load();
    Check(!disabled_state.tutorials_enabled && disabled_state.movie_flags==0x80000001);
    TutorialServices disabled(settings,catalog,24,disabled_state.tutorials_enabled,[](unsigned){throw std::runtime_error("unexpected disabled save");});
    ScriptStack disabled_stack(globals);disabled_stack.Compile("off","void Probe(){giShowTutorial(10);giPlayerUnLock();}");
    Check(disabled_stack.Push("off","void Probe()",disabled).has_value());Check(disabled_stack.Tick(0)==ScriptState::suspended);
    Check(!disabled.View().visible);const auto skipped=disabled.Advance();Check(skipped && disabled_stack.Complete(*skipped));
    Check(disabled_stack.Tick(0)==ScriptState::finished && unlocks==3);
    // Fail the real filesystem save: no invisible wait token and no unlock may escape the error.
    settings.SetTutorialsEnabled(true);const auto backup=std::filesystem::path(path.string()+".bak");
    std::filesystem::create_directory(backup);{std::ofstream block(backup/"block");block<<"owned";}
    TutorialServices failing(settings,catalog,25,true,[&](unsigned key){settings.SaveTutorial(key);});
    ScriptStack failed_stack(globals);failed_stack.Compile("bad","void Probe(){giShowTutorial(10);giPlayerUnLock();}");
    Check(failed_stack.Push("bad","void Probe()",failing).has_value());
    Check(failed_stack.Tick(0)==ScriptState::failed && !failed_stack.Error().empty());
    Check(!failing.View().visible && !failing.View().waiting && !failing.Advance() && unlocks==3);
    Check(failed_stack.Tick(0)==ScriptState::failed && unlocks==3);
    Check(FileSettingsStore(path).Load()==state && state.movie_flags==0x80000001);
    // UI-origin tutorials share the seen gate but never own a script token.
    {const TutorialCatalog ui_catalog{{1,"party tutorial"}};bool fail=true;unsigned writes{};
        TutorialServices ui(settings,ui_catalog,26,true,[&](unsigned key){Check(key==1);if(fail)throw std::runtime_error("fixture UI save failure");++writes;});
        bool rejected=false;try{ui.ShowFromUi(1);}catch(const std::runtime_error&){rejected=true;}
        Check(rejected && !ui.View().visible && !ui.View().waiting && !ui.Advance());
        fail=false;Check(ui.ShowFromUi(1) && writes==1 && ui.View().visible && !ui.View().waiting);
        const auto view=ui.View();Check(!ui.Close(25,view.id) && !ui.Close(26,view.id+1));
        Check(ui.Close(26,view.id) && !ui.Advance() && !ui.ShowFromUi(1) && writes==1);
    }
    std::cout<<"tutorial -> file settings -> script wait -> unlock integration passed\n";
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
