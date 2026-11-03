#include "renderware_vector_cases.h"
#include "pal4/renderware_vector2_native.h"
#include "pal4/world_map_flight.h"
#include "pal4/world_map_fly_start.h"
#include <rwcore.h>
#include <bit>
#include <cmath>
#include <map>
#include <stdexcept>
#include <string>

namespace pal4::test {
namespace {
void Require(bool ok,const char* message) { if(!ok) throw std::runtime_error(message); }
void Near(float a,float b,const char* message) { Require(std::fabs(a-b)<.0003f,message); }
// UI, timer and script services are fixtures. The two actual game methods and
// native RenderWare vector adapter are linked without replacing their math.
struct MapRuntime final : IWorldMapFlyStartRuntime {
    static constexpr unsigned frame=0x5000,marker=0x7000,manager=0x8000;
    std::map<unsigned,unsigned> memory;
    std::pair<float,float> target,position{10.f,20.f};
    int disabled{},released{},queued{},removed{},cleaned{},script_calls{};
    std::string sprite;
    MapRuntime(float dx,float dy):target{10+dx,20+dy} {
        memory[frame+0x270]=2; memory[frame+0x238]=marker;
        memory[frame+0x268]=std::bit_cast<unsigned>(10.f);
        memory[manager+0x1a0]=0x8100;
        for(unsigned i=0;i<21;++i) memory[frame+0x40+4*i]=0x6000+i;
    }
    unsigned ReadWord(unsigned a) { return memory.at(a); }
    void WriteWord(unsigned a,unsigned v) { memory[a]=v; }
    float ReadFloat(unsigned a) { return std::bit_cast<float>(memory.at(a)); }
    void WriteFloat(unsigned a,float v) { memory[a]=std::bit_cast<unsigned>(v); }
    std::uint8_t ReadByte(unsigned) { throw std::runtime_error("unexpected string read"); }
    void WriteByte(unsigned,std::uint8_t) { throw std::runtime_error("unexpected string write"); }
    void Free(unsigned) { throw std::runtime_error("unexpected callback free"); }
    bool IsVisible(unsigned,bool inherited) { Require(!inherited,"visibility scope"); return true; }
    void SetEnabled(unsigned,bool enabled) { Require(!enabled,"disable destination"); ++disabled; }
    std::pair<float,float> GetPosition(unsigned w) { return w==marker?position:target; }
    void SetPosition(unsigned w,float x,float y) { Require(w==marker,"marker handle"); position={x,y}; }
    unsigned CopyLocationName(unsigned a) { Require(a==frame+0x94+32,"selected name"); return 0x9000; }
    void ReleaseLocationName(unsigned n) { Require(n==0x9000,"name handle"); ++released; }
    unsigned FrameManager() { return manager; }
    unsigned Allocate(unsigned n) { Require(n==32,"callback allocation size"); return 0xA000; }
    unsigned ConstructCallback(unsigned storage,unsigned object,unsigned function,unsigned name) {
        Require(storage==0xA000 && object==frame && function==0x484C10 && name==0x9000,"callback fields"); return storage;
    }
    unsigned ConstructTimerEntry(unsigned address,unsigned interval,unsigned callback) {
        Require(address==frame+0x258 && interval==std::bit_cast<unsigned>(10.f) && callback==0xA000,"timer fields"); return 0xB000;
    }
    void AddTimerEntry(unsigned queue,unsigned entry) { Require(queue==0x8100 && entry==0xB000,"timer insertion"); ++queued; }
    void DestroyTimerEntry(unsigned entry) { Require(entry==0xB000,"temporary timer lifetime"); }
    unsigned SequenceImage(std::string_view name) { sprite=name; return 0xC000; }
    void RemoveFlyMember(unsigned queue,unsigned name) { Require(queue==0x8100 && name==frame+0x258,"arrival timer removal"); ++removed; }
    void InitializeScripts() {}
    unsigned ScriptMode() { return 3; }
    unsigned EmptyText() { return 0xD000; }
    unsigned CreateInterpreter(unsigned mode,std::string_view object,unsigned name,unsigned path,std::string_view file,unsigned reader) {
        Require(mode==3 && object=="WorldMap" && name==0xD000 && path==name && file==WorldMapFlight::kScriptFile && reader==0x600D50,"arrival script boundary");
        ++script_calls; return 0;
    }
    unsigned ScriptEngine() { throw std::runtime_error("unexpected engine"); }
    bool AddScriptModule(unsigned,unsigned) { throw std::runtime_error("unexpected execution"); }
    unsigned FunctionName(unsigned) { throw std::runtime_error("unexpected function name"); }
    unsigned FullPath(unsigned) { throw std::runtime_error("unexpected script full path"); }
    void PushPreparedModule(unsigned,unsigned,unsigned) { throw std::runtime_error("unexpected prepared destination queue"); }
    void ReleaseInterpreter(unsigned) { throw std::runtime_error("unexpected interpreter release"); }
    void CallHide(unsigned,unsigned) { throw std::runtime_error("unexpected hide"); }
    unsigned CleanupAllScripts() { ++cleaned; return 0x1234; }
};
// Flight owns strings differently from the start callback allocator; preserve
// the two service interfaces while sharing the simulated game memory/UI state.
struct FlightView final : IWorldMapFlightRuntime {
    MapRuntime& state;
    explicit FlightView(MapRuntime& value):state(value) {}
    float ReadFloat(unsigned a) override { return state.ReadFloat(a); }
    void WriteFloat(unsigned a,float v) override { state.WriteFloat(a,v); }
    unsigned ReadWord(unsigned a) override { return state.ReadWord(a); }
    std::uint8_t ReadByte(unsigned a) override { return state.ReadByte(a); }
    void WriteByte(unsigned a,std::uint8_t v) override { state.WriteByte(a,v); }
    unsigned Free(unsigned) override { throw std::runtime_error("unexpected string free"); }
    void SetPosition(unsigned w,float x,float y) override { state.SetPosition(w,x,y); }
    unsigned FrameManager() override { return state.FrameManager(); }
    void RemoveFlyMember(unsigned q,unsigned n) override { state.RemoveFlyMember(q,n); }
    void InitializeScripts() override { state.InitializeScripts(); }
    unsigned ScriptMode() override { return state.ScriptMode(); }
    unsigned EmptyText() override { return state.EmptyText(); }
    unsigned CreateInterpreter(unsigned m,std::string_view o,unsigned n,unsigned p,std::string_view f,unsigned r) override { return state.CreateInterpreter(m,o,n,p,f,r); }
    unsigned ScriptEngine() override { return state.ScriptEngine(); }
    bool AddScriptModule(unsigned a,unsigned b) override { return state.AddScriptModule(a,b); }
    unsigned FunctionName(unsigned a) override { return state.FunctionName(a); }
    unsigned FullPath(unsigned a) override { return state.FullPath(a); }
    void PushPreparedModule(unsigned a,unsigned b,unsigned c) override { state.PushPreparedModule(a,b,c); }
    void ReleaseInterpreter(unsigned a) override { state.ReleaseInterpreter(a); }
    void CallHide(unsigned a,unsigned b) override { state.CallHide(a,b); }
    unsigned CleanupAllScripts() override { return state.CleanupAllScripts(); }
};
}
void RunWorldMapVectorCase() {
    NativeRenderWareVector2 math;
    for(float dx : {-3.f,3.f}) for(float dy : {-4.f,4.f}) {
        MapRuntime state(dx,dy); FlightView flight(state);
        WorldMapFlyStart::Start(MapRuntime::frame,state,math);
        Require(state.disabled==21 && state.released==1 && state.queued==1,"flight setup services");
        Require(state.sprite==(dx<0?(dy<0?"flya":"flyc"):(dy<0?"flyb":"flyd")),"native flight quadrant sprite");
        Near(state.ReadFloat(MapRuntime::frame+0x23c),5,"flight length");
        Near(state.ReadFloat(MapRuntime::frame+0x240),dx/50,"flight velocity x");
        Near(state.ReadFloat(MapRuntime::frame+0x244),dy/50,"flight velocity y");
        for(int step=1;step<=5;++step) {
            const auto result=WorldMapFlight::Advance(MapRuntime::frame,0,flight,math);
            Require(result==(step<5?0x100u:0x1234u),"native flight arrival tick");
            Require(state.removed==(step==5?1:0),"arrival before destination");
            Near(state.position.first,10+dx*step/5,"marker x"); Near(state.position.second,20+dy*step/5,"marker y");
        }
        Require(state.script_calls==1 && state.cleaned==1,"arrival script cleanup");
    }
    MapRuntime zero(0,0); FlightView flight(zero); RwError error{}; RwErrorGet(&error);
    WorldMapFlyStart::Start(MapRuntime::frame,zero,math);
    Require(zero.sprite=="flyd" && zero.ReadFloat(MapRuntime::frame+0x240)==0 && zero.ReadFloat(MapRuntime::frame+0x244)==0,"zero displacement setup");
    RwErrorGet(&error); Require(error.errorCode!=0,"zero displacement SDK error lost");
    Require(WorldMapFlight::Advance(MapRuntime::frame,0,flight,math)==0x1234 && zero.removed==1,"zero displacement first-tick arrival");
}
}
