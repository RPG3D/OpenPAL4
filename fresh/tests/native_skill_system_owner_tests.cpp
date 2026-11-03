#include "pal4/native_skill_system_owner.h"
#include "pal4/native_combat_event_logger.h"
#include "pal4/native_player_combat_events.h"
#include "pal4/random_number_generator.h"
#include "pal4/skill_system_event_registry.h"
#include "pal4/combat_event_log_entry.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>
namespace {
void Check(bool v,const char* message) { if(!v) throw std::runtime_error(message); }
template<class F> void Reject(F&& f) {
    bool rejected{}; try { f(); } catch(const std::exception&) { rejected=true; }
    Check(rejected,"expected explicit failure");
}
std::uint32_t Word(std::uint32_t a) { return *reinterpret_cast<const std::uint32_t*>(a); }
std::string Read(const std::filesystem::path& path) {
    std::ifstream input(path); Check(input.is_open(),"open actual native log");
    return {std::istreambuf_iterator<char>(input),std::istreambuf_iterator<char>()};
}
void Run(const std::filesystem::path& directory) {
    Check(pal4::FormatCombatLogEntry({3u,2u,1u,4u,4u,126u},2u,"message","detail")==
        "04/05/2026 01:02:03 (InfL2)\tmessage\tdetail\n","shared exact log text policy");
    pal4::NativePlayerCombatEvents events;
    const auto hp=events.Lookup(2u);
    Check(hp!=0u,"native77-object registry initialized");
    pal4::RandomNumberGenerator random;
    random.Seed(123u);
    pal4::NativeSkillSystemOwner system(random,events);
    Reject([&]{(void)system.FindCallback(9000);});
    // Existing regular file used as parent guarantees fopen failure without
    // depending on ACLs or a particular absent directory on the host.
    const auto blocker=directory/"blocked-parent.txt";
    { std::ofstream out(blocker); out<<"not a directory"; }
    Reject([&]{(void)system.Construct(blocker);});
    Check(!system.Address() && !system.OwnedAllocations() && random.StateAt(0u)==123u,
          "log-open factory failure cleans partial allocation before registry seed");
    const auto a=system.Construct(directory);
    Check(a && system.Address()==a && Word(a)==2u && system.OwnedAllocations()==2u,
          "actual2360-byte lifecycle owns system and callback vector");
    const auto begin=Word(a+2348u),end=Word(a+2352u),capacity=Word(a+2356u);
    Check(end-begin==39u*8u && capacity-begin==64u*8u,"actual sorted registry capacity");
    for(const auto& entry:pal4::SkillSystemEventRegistry::Entries())
        Check(system.FindCallback(static_cast<std::int32_t>(entry.id))==entry.callback,"all39 callbacks resolve via shared binary search");
    Check(!system.FindCallback(-1) && !system.FindCallback(9002),"missing signed callback lookup");
    Check(events.Lookup(2u)==hp,"581760 reinitialization retains guarded event identity");
    Check(random.Index()==624u && random.StateAt(0u)==system.Seed(),"actual timeGetTime seeds borrowed game RNG");
    pal4::RandomNumberGenerator expected; expected.Seed(system.Seed());
    Check(random.Next()==expected.Next(),"initialization shares the same game RNG sequence");
    Reject([&]{(void)system.Construct(directory);});
    system.Log(2u,"native-system-info"); system.Log(3u,"must-be-filtered");
    auto text=Read(directory/"CombatNSkill.log");
    Check(text.find("Pal4 CombatSystem - Event log")!=std::string::npos &&
          text.find("CombatSystem skLogger created.")!=std::string::npos &&
          text.find("(InfL2)\tnative-system-info")!=std::string::npos &&
          text.find("must-be-filtered")==std::string::npos,"startup flush and actual system threshold/file output");
    system.Destroy(); system.Destroy();
    Check(!system.Address() && !system.OwnedAllocations() && events.Lookup(2u)==hp,"shared lifecycle cleanup leaves borrowed event registry alive");
    Reject([&]{system.Log(1u,"destroyed");});
    (void)system.Construct(directory);
    text=Read(directory/"CombatNSkill.log");
    Check(text.find("native-system-info")==std::string::npos,"mode0 truncates prior log on reconstructed service lifetime");
    system.Destroy();
    for(const auto seed:{0u,0xffffffffu,0x1105u}){
        (void)system.Construct(directory,seed);
        Check(system.Seed()==seed && random.Index()==624u,"explicit initialization clock uses the actual shared seeding boundary");
        pal4::RandomNumberGenerator replay;replay.Seed(seed);
        for(unsigned n=0;n<1300u;++n)Check(random.Next()==replay.Next(),"controlled initialization preserves stream across two refresh boundaries");
        system.Destroy();
    }
    // Exercise modern append at the original mode boundary, including buffered
    // messages filtered using the threshold at flush time.
    pal4::NativeCombatEventLogger logger;
    logger.Construct(); logger.SetThreshold(0u);
    logger.SetLogFile(directory/"append.log",0u); logger.Log(0u,"first");
    logger.SetLogFile(directory/"append.log",1u); logger.Log(0u,"second");
    logger.Destroy();
    const auto appended=Read(directory/"append.log");
    Check(appended.find("first")!=std::string::npos && appended.find("second")!=std::string::npos &&
          appended.find("skLogger created")==std::string::npos,"public native stream append and buffered-level filtering");
}
}
int main(int argc,char** argv) {
    try {
        Check(argc==2,"output directory argument required");
        const std::filesystem::path directory(argv[1]); std::filesystem::create_directories(directory);
        Run(directory); std::cout<<"Native SkillSystem lifecycle/log/registry/shared RNG/cleanup pass\n"; return 0;
    } catch(const std::exception& e) { std::cerr<<e.what()<<'\n'; return 1; }
}
