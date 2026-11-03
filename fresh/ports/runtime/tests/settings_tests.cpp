#include "pal4/portable/settings.h"
#include <algorithm>
#include <chrono>
#include <fstream>
#include <iostream>
#include <stdexcept>
using namespace pal4::portable;
void Check(bool value){if(!value)throw std::runtime_error("settings contract failed");}
struct Missing:IScriptContextServices {
    ScriptResponse Invoke(const ScriptRequest& request) override {throw std::runtime_error("unbound service "+std::to_string(request.call.system_id));}
};
struct Store:ISettingsStore {
    Settings saved{};unsigned saves{};bool fail{};
    Settings Load() override {return saved;}
    void Save(const Settings& value) override {if(fail)throw std::runtime_error("storage full");saved=value;++saves;}
};
int main() {
    try {
        // Independent zlib CRC32 fixtures; old v1 migrates without losing movie flags.
        const std::array<std::uint8_t,16> legacy{0x50,0x34,0x53,0x54,1,0,0,0,1,0,0,0x80,0x17,0x6d,0x2e,0x3b};
        const std::array<std::uint8_t,20> v2{80,52,83,84,2,0,0,0,1,0,0,128,1,0,0,0,212,189,171,217};
        const std::array<std::uint8_t,32> fixture{80,52,83,84,3,0,0,0,1,0,0,128,1,0,0,0,1,0,0,0,0,0,0,128,2,0,0,0,109,172,3,88};
        const Settings expected{0x80000001,true,{1,0x80000000,2}};
        const std::array<std::uint8_t,64> v4{80,52,83,84,4,0,0,0,1,0,0,128,1,0,0,0,1,0,0,0,0,0,0,128,2,0,0,0,1,0,0,0,0,0,0,128,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,128,82,15,34,155};
        auto help_expected=expected;help_expected.help_seen={1,0x80000000,0,0,0,0,0,0x80000000};
        Check(DecodeSettings(v2)==Settings{0x80000001,true});
        Check(DecodeSettings(legacy)==Settings{0x80000001,true});
        Check(DecodeSettings(fixture)==expected);
        Check(EncodeSettings(help_expected)==v4 && DecodeSettings(v4)==help_expected);
        Check(help_expected.HelpSeen(0) && help_expected.HelpSeen(63) && help_expected.HelpSeen(255));
        Check(!help_expected.HelpSeen(1) && !help_expected.HelpSeen(256) && !help_expected.HelpSeen(0xffffffff));
        for(std::size_t n=0;n<v4.size();++n){
            bool bad=false;try{DecodeSettings(std::span(v4).first(n));}catch(const std::runtime_error&){bad=true;}Check(bad);
            auto damaged=v4;damaged[n]^=1;bad=false;try{DecodeSettings(damaged);}catch(const std::runtime_error&){bad=true;}Check(bad);
        }
        std::array<std::uint8_t,65> overlong{};std::copy(v4.begin(),v4.end(),overlong.begin());
        bool overlong_rejected=false;try{DecodeSettings(overlong);}catch(const std::runtime_error&){overlong_rejected=true;}Check(overlong_rejected);
        for(std::size_t count=0;count<fixture.size();++count) {
            bool rejected=false;try{DecodeSettings(std::span(fixture).first(count));}catch(const std::runtime_error&){rejected=true;}Check(rejected);
        }
        for(std::size_t index=0;index<fixture.size();++index) {
            auto corrupt=fixture;corrupt[index]^=1;bool rejected=false;
            try{DecodeSettings(corrupt);}catch(const std::runtime_error&){rejected=true;}Check(rejected);
        }
        std::array<std::uint8_t,33> extra{};std::copy(fixture.begin(),fixture.end(),extra.begin());
        bool rejected=false;try{DecodeSettings(extra);}catch(const std::runtime_error&){rejected=true;}Check(rejected);
        Missing missing;Store store;Settings state{};SettingsServices service(missing,state,store);
        const auto open=[&](std::int32_t bit){return service.Invoke({{1,1},ScriptCall{110,0,{bit}}});};
        Check(!open(0).suspend && state.movie_flags==1 && store.saved==state);
        open(31);open(0);Check(state.movie_flags==0x80000001 && service.Writes()==3 && store.saves==3);
        open(-1);open(32);Check(store.saves==3);
        store.fail=true;rejected=false;try{open(4);}catch(const std::runtime_error&){rejected=true;}
        Check(rejected && state.movie_flags==0x80000001 && store.saved.movie_flags==0x80000001 && service.Writes()==3);
        store.fail=false;ScriptGlobals globals{};ScriptStack stack(globals);
        stack.Compile("flag","void Main(){giOpenMovieFlag(0);global_mvar=10100;giNewGame();global_mvar=99;}");
        stack.Push("flag","void Main()",service);
        Check(stack.Tick(0)==ScriptState::failed && globals[0]==10100 && stack.Error()=="unbound service 57");

        const auto path=std::filesystem::temp_directory_path()/("pal4-settings-test-"+
            std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())+".set");
        auto temp=path;temp+=".tmp";auto backup=path;backup+=".bak";
        Check(!std::filesystem::exists(path) && !std::filesystem::exists(temp));
        struct Cleanup {
            std::filesystem::path path,temp,backup;
            ~Cleanup(){std::error_code error;std::filesystem::remove(temp,error);std::filesystem::remove(path,error);std::filesystem::remove(backup,error);}
        } cleanup{path,temp,backup};
        FileSettingsStore files(path);Check(files.Load()==Settings{});
        files.Save({1});Check(FileSettingsStore(path).Load().movie_flags==1);
        files.Save({0x80000001});Check(FileSettingsStore(path).Load().movie_flags==0x80000001 && !std::filesystem::exists(temp));
        FileSettingsStore bad(path/"child.set");rejected=false;
        try{bad.Save({4});}catch(const std::exception&){rejected=true;}Check(rejected && files.Load().movie_flags==0x80000001);
        {std::ofstream partial(temp,std::ios::binary);partial<<"partial";}
        Check(files.Load().movie_flags==0x80000001);
        std::filesystem::rename(path,backup); // 模拟旧提交已移走、新文件尚未提交时重启。
        Check(!std::filesystem::exists(path) && files.Load().movie_flags==0x80000001);
        files.Save({5});Check(files.Load().movie_flags==5 && !std::filesystem::exists(backup));
        std::filesystem::create_directory(backup);{std::ofstream marker(backup/"owned.txt");marker<<"keep";}
        rejected=false;try{files.Save({7});}catch(const std::filesystem::filesystem_error&){rejected=true;}
        Check(rejected && files.Load().movie_flags==5 && std::filesystem::exists(backup/"owned.txt"));
        std::filesystem::remove(backup/"owned.txt");std::filesystem::remove(backup);
        auto corrupt=fixture;corrupt[8]^=4;
        {std::ofstream output(path,std::ios::binary|std::ios::trunc);output.write(reinterpret_cast<const char*>(corrupt.data()),corrupt.size());}
        rejected=false;try{files.Load();}catch(const std::runtime_error&){rejected=true;}
        Check(rejected && std::filesystem::file_size(path)==32);
        {std::ofstream output(path,std::ios::binary|std::ios::trunc);output.write(reinterpret_cast<const char*>(legacy.data()),legacy.size());}
        auto migrated=files.Load();Check(migrated.movie_flags==0x80000001 && migrated.tutorials_enabled);migrated.tutorials_enabled=false;files.Save(migrated);
        Check(FileSettingsStore(path).Load()==migrated && std::filesystem::file_size(path)==64);
        // All historical formats migrate with seen bits clear and old values intact.
        for(const auto bytes:{std::span<const std::uint8_t>(legacy),std::span<const std::uint8_t>(v2),std::span<const std::uint8_t>(fixture)}){
            {std::ofstream output(path,std::ios::binary|std::ios::trunc);output.write(reinterpret_cast<const char*>(bytes.data()),bytes.size());}
            const auto previous=files.Load();Check(previous.help_seen==Settings{}.help_seen);
            files.Save(previous);Check(files.Load()==previous && std::filesystem::file_size(path)==64);
        }
        Store help_store;Settings help_state=expected;SettingsServices help_service(missing,help_state,help_store);
        for(const auto id:{0u,31u,32u,63u,255u})Check(help_service.MarkHelpSeen(id) && help_state.HelpSeen(id));
        Check(help_store.saves==5 && help_service.Writes()==5);
        Check(!help_service.MarkHelpSeen(0) && !help_service.MarkHelpSeen(256) && !help_service.MarkHelpSeen(0xffffffff) && help_store.saves==5);
        const auto before_failure=help_state;help_store.fail=true;rejected=false;
        try{help_service.MarkHelpSeen(100);}catch(const std::runtime_error&){rejected=true;}
        Check(rejected && help_state==before_failure && help_store.saved==before_failure && help_service.Writes()==5);
        files.Save(help_state);Check(files.Load()==help_state && help_state.movie_flags==expected.movie_flags && help_state.pal_test_results==expected.pal_test_results);
        Store tutorial_store;Settings tutorial_state{7};SettingsServices tutorial_settings(missing,tutorial_state,tutorial_store);
        tutorial_settings.SaveTutorial(10);Check(tutorial_store.saves==1 && tutorial_store.saved==tutorial_state);
        tutorial_settings.SaveTutorial(128);Check(tutorial_store.saves==1);
        tutorial_settings.SetTutorialsEnabled(false);Check(!tutorial_state.tutorials_enabled && tutorial_state.movie_flags==7 && tutorial_store.saves==2);
        tutorial_store.fail=true;rejected=false;try{tutorial_settings.SetTutorialsEnabled(true);}catch(const std::runtime_error&){rejected=true;}Check(rejected && !tutorial_state.tutorials_enabled && tutorial_settings.Writes()==2);
        // Getter uses live settings, preserves word bits, and never saves on a query.
        state.pal_test_results={1,0x80000000,2};
        const auto read=[&](std::int32_t index){return std::get<std::int32_t>(service.Invoke({{9,1},ScriptCall{220,0,{index}}}).value);};
        Check(read(0)==1 && read(1)==(-2147483647-1) && read(2)==2 && read(-1)==0 && read(3)==0);
        const auto saves_before=store.saves;
        state.pal_test_results[0]=0;Check(read(0)==0 && store.saves==saves_before);
        FileSettingsStore(path).Save(state);Check(FileSettingsStore(path).Load()==state);
        const auto writes_before=service.Writes();service.SetTutorialsEnabled(false);
        Check(state.pal_test_results==store.saved.pal_test_results && service.Writes()==writes_before+1);
        ScriptGlobals reward_globals{};ScriptStack reward_stack(reward_globals);
        reward_stack.Compile("rewards","void Probe(){global_mvar=0;if(giGetPalTestResult(0)==1)global_mvar=1;if(giGetPalTestResult(2)==1)global_mvar=2;}");
        Check(reward_stack.Push("rewards","void Probe()",service).has_value());
        Check(reward_stack.Tick(0)==ScriptState::finished && reward_globals[0]==0);
        Check(reward_stack.PopFinished());state.pal_test_results[2]=1;
        Check(reward_stack.Push("rewards","void Probe()",service).has_value());
        Check(reward_stack.Tick(0)==ScriptState::finished && reward_globals[0]==2);
        std::cout<<"movie flag bounds, CRC, commit rollback, replacement, restart and corruption contracts passed\n";
    }catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
}
