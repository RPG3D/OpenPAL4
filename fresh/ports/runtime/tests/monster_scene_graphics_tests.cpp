#include "pal4/portable/monster_scene_graphics.h"
#include "pal4/portable/monster_scene_resources.h"
#include "pal4/portable/monster_scene_runtime.h"
#include "pal4/portable/package.h"
#include "pal4/portable/scene_movement.h"
#include "pal4/base_object_activity.h"
#include <EGL/egl.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <set>
#include <sstream>
#include <stdexcept>

using namespace pal4::portable;
namespace {
void Check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
struct Context {
    EGLDisplay display=eglGetDisplay(EGL_DEFAULT_DISPLAY);
    EGLContext context=EGL_NO_CONTEXT;EGLSurface surface=EGL_NO_SURFACE;
    Context(){
        Check(eglInitialize(display,nullptr,nullptr),"EGL initialization failed");
        const EGLint attributes[]{EGL_SURFACE_TYPE,EGL_PBUFFER_BIT,EGL_RENDERABLE_TYPE,EGL_OPENGL_ES3_BIT,
            EGL_RED_SIZE,8,EGL_GREEN_SIZE,8,EGL_BLUE_SIZE,8,EGL_ALPHA_SIZE,8,EGL_DEPTH_SIZE,24,EGL_NONE};
        EGLConfig config{};EGLint count{};
        Check(eglChooseConfig(display,attributes,&config,1,&count)&&count==1,"EGL config failed");
        const EGLint dimensions[]{EGL_WIDTH,800,EGL_HEIGHT,600,EGL_NONE};
        const EGLint version[]{EGL_CONTEXT_CLIENT_VERSION,3,EGL_NONE};
        surface=eglCreatePbufferSurface(display,config,dimensions);
        context=eglCreateContext(display,config,EGL_NO_CONTEXT,version);
        Check(eglMakeCurrent(display,surface,surface,context),"EGL context failed");
    }
    ~Context(){
        eglMakeCurrent(display,EGL_NO_SURFACE,EGL_NO_SURFACE,EGL_NO_CONTEXT);
        eglDestroyContext(display,context);eglDestroySurface(display,surface);eglTerminate(display);
    }
};
class UnboundSceneServices final:public IMonsterPursuitSceneServices {
    [[noreturn]] static void Missing(){throw std::logic_error("model render fixture cannot run scene gameplay");}
public:
    pal4::MonsterProximityActor ControlledActor() override{Missing();}
    pal4::MonsterPursuitTarget Target(std::uint32_t) override{Missing();}
    bool PlayerControlBusy() override{Missing();}
    bool ProcessObjectInteraction(pal4::MonsterBehaviorManagerState&,std::uint32_t,std::uint32_t,float) override{Missing();}
    void DispatchCombatPositions(std::uint32_t,float) override{Missing();}
    void InitializeCombat(std::uint32_t,std::int32_t) override{Missing();}
    void ClearHistoryRecord(std::uint32_t) override{Missing();}
    void ProcessActionWithScript(std::uint32_t,float) override{Missing();}
    void VisitArenaObjects(std::uint32_t) override{Missing();}
};
class RenderServices final:public IMonsterSceneRenderServices {
public:
    std::function<void(SceneMonsterHandle,std::uint32_t)> draw;
    // Fixture camera fits one selected model. This does not validate PC frustum culling.
    bool FrustumTestSphere(SceneMonsterHandle,std::uint32_t,std::array<float,4>) override{return true;}
    void DrawClump(SceneMonsterHandle h,std::uint32_t clump) override{draw(h,clump);}
    void Output(std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t) override{throw std::logic_error("unbound output callback");}
    void Path(std::uint32_t,std::uint32_t,std::uint32_t) override{throw std::logic_error("unbound path callback");}
    std::uint32_t Move(std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t) override{throw std::logic_error("unbound move callback");}
};
template<class Read> auto Records(ArenaSource& source,const std::string& world,const char* member,Read read){
    const auto bytes=source.ReadSceneMember(world+"\\1\\"+member);
    Check(bytes.has_value(),"original scene record missing");
    std::istringstream input(std::string(bytes->begin(),bytes->end()));return read(input);
}
std::vector<std::uint8_t> Capture(){
    std::vector<std::uint8_t> pixels(800*600*4);
    glReadPixels(0,0,800,600,GL_RGBA,GL_UNSIGNED_BYTE,pixels.data());
    Check(glGetError()==GL_NO_ERROR,"monster drawing produced a GLES error");return pixels;
}
void Save(const std::filesystem::path& path,const std::vector<std::uint8_t>& pixels){
    std::ofstream out(path,std::ios::binary);out<<"P6\n800 600\n255\n";
    for(int y=599;y>=0;--y)for(int x=0;x<800;++x)
        out.write(reinterpret_cast<const char*>(&pixels[(y*800+x)*4]),3);
    Check(bool(out),"cannot write probe capture");
}
void Run(const std::filesystem::path& data,const std::filesystem::path& output,const std::string& world,std::size_t expected){
    auto source=std::make_shared<ArenaSource>(data);
    const auto records=Records(*source,world,"mstInfo.mst",ReadSceneMonsters);
    const auto paths=Records(*source,world,"pathway.pwy",ReadScenePaths);
    auto collision=source->ReadOriginalCollision(world,"1");
    CollisionSurface floor(collision.floor);floor.EnableNavigation();SceneMovement ground(collision.floor);
    MonsterOrientationCaches caches;UnboundSceneServices scene;
    std::ifstream actor_file(data/"palactor.cpk",std::ios::binary);Check(bool(actor_file),"original actor package missing");
    PackageReader actor_package(actor_file);std::uint32_t identifier=3;
    MonsterSceneRuntime runtime(61,source,records,paths,[]{throw std::logic_error("render fixture cannot consume RNG");return 0u;});
    runtime.CreateActors({actor_package,ground,caches,floor,collision,scene,2.f,
        [](SceneMonsterHandle h){return MonsterSceneActorTokens{0x10000u+h.index*0x10000u,0x200000u+h.index*8u,1,1};},
        [&](SceneMonsterHandle h){return runtime.Monsters().LocalSphere(h)[3];},[&]{return identifier++;},
        [](Point3,Point3)->bool{throw std::logic_error("render fixture cannot query movement collision");},
        [](MonsterHelperHandle)->std::vector<MonsterMovementObstacle>{throw std::logic_error("render fixture cannot query obstacles");}});
    auto& actors=runtime.Monsters();Check(actors.Size()==expected,"original monster count differs");
    for(std::size_t i=0;i<runtime.ActorCount();++i){
        auto& actor=runtime.Actor(i);pal4::BaseObjectState base;
        std::copy_n(actor.State().words.begin(),base.words.size(),base.words.begin());
        (void)pal4::BaseObjectActivity::SetStateFlag(base,1);
        actor.State().words[0x1f0/4]=base.Read(0x1f0);actor.State().words[9]=1; // Explicit visual/world-entry fixture.
    }
    DistanceOptimization distance;distance.SetEnabled(false); // Explicit visual fixture, no game settings changed.
    auto graphics_owner=std::make_unique<MonsterSceneGraphics>(actors,distance);auto& graphics=*graphics_owner;
    std::vector<Matrix> projections;
    for(std::size_t i=0;i<actors.Size();++i){
        const auto h=actors.Handle(i);
        Check(actors.SelectAction(h,"Z01"),"explicit probe animation missing");
        actors.AdvanceAnimation(h,.125f);
        // This isolated model test uses a local origin, not a fake scene/AI frame.
        actors.SetMotionPosition(h,{0,0,0});actors.SetMotionRotation(h,{0,0,0});
        Scene pose=actors.Model(h).mesh;actors.Playback(h).Sample(pose.vertices);
        for(unsigned a=0;a<3;++a)pose.minimum[a]=pose.maximum[a]=pose.vertices.front().position[a];
        for(const auto& v:pose.vertices)for(unsigned a=0;a<3;++a){
            pose.minimum[a]=std::min(pose.minimum[a],v.position[a]);
            pose.maximum[a]=std::max(pose.maximum[a],v.position[a]);
        }
        OrbitCamera camera;camera.Fit(pose);camera.Zoom(.35f);projections.push_back(camera.Matrix(4.f/3));
    }
    const auto draw=[&](std::size_t i){
        const auto h=actors.Handle(i);const auto time=actors.AnimationTime(h);
        GlesRenderer::BeginFrame(800,600);
        RenderServices services;unsigned submissions{};
        services.draw=[&](SceneMonsterHandle selected,std::uint32_t clump){
            Check(selected==h && clump==0x200000u+h.index*8u,"render frame dispatched another instance");
            Check(graphics.Draw(h,projections[i],{0,0,0})>0,"monster emitted no triangles");++submissions;
        };
        const auto traversal=actors.OriginalTraversalOrder();
        const auto found=std::find(traversal.begin(),traversal.end(),i);Check(found!=traversal.end(),"original actor order missing");
        const auto result=runtime.Actor(static_cast<std::size_t>(found-traversal.begin())).Render(0x8000,services);
        Check(result.active_objects==1 && result.render_successes==1 && result.animation_calls==1 && result.character_updates==1 && submissions==1,
              "shared monster render frame did not complete its original callback chain");
        Check(actors.AnimationTime(h)==time,"drawing advanced gameplay animation time");
        return Capture();
    };
    std::vector<std::vector<std::uint8_t>> reference;
    std::set<std::string> saved;
    {
        Context context;graphics.Upload();Check(graphics.Uploaded(),"graphics upload not published");
        for(std::size_t i=0;i<actors.Size();++i){
            auto pixels=draw(i);unsigned changed{};
            for(std::size_t p=0;p<pixels.size();p+=4)
                if(std::abs(int(pixels[p])-26)+std::abs(int(pixels[p+1])-38)+std::abs(int(pixels[p+2])-43)>20)++changed;
            Check(changed>100,"monster frame has no visible model pixels");
            const auto& name=actors.Source(actors.Handle(i)).resource;
            if(saved.insert(name).second)Save(output/(world+"-model-"+std::to_string(i)+".ppm"),pixels);
            reference.push_back(std::move(pixels));
        }
        const auto first=actors.Handle(0);
        const auto& mesh=actors.Model(first).mesh;
        const float shift=std::max(1.f,(mesh.maximum[0]-mesh.minimum[0])*.3f);
        actors.SetMotionPosition(first,{shift,0,0});
        Check(draw(0)!=reference[0],"drawing ignored the current world transform");
        actors.SetMotionPosition(first,{0,0,0});
        Check(draw(0)==reference[0],"restoring the transform did not restore the image");
        bool rejected=false;
        try{graphics.Draw({62,0},projections[0],{0,0,0});}catch(const std::logic_error&){rejected=true;}
        Check(rejected,"graphics accepted a stale scene generation");
        graphics.Release();Check(!graphics.Uploaded(),"released GPU state still published");
        rejected=false;
        try{graphics.Draw(actors.Handle(0),projections[0],{0,0,0});}catch(const std::logic_error&){rejected=true;}
        Check(rejected,"drawing used released GPU objects");
        graphics.Upload();
        for(std::size_t i=0;i<actors.Size();++i)Check(draw(i)==reference[i],"re-upload changed the rendered pose");
        // Real context destruction follows. Never delete old GL names in a new context.
        graphics.Abandon();Check(!graphics.Uploaded(),"abandoned GPU state still published");
    }
    {
        Context context;graphics.Upload();
        for(std::size_t i=0;i<actors.Size();++i)Check(draw(i)==reference[i],"new context changed the rendered pose");
        graphics.Release();
    }
    const auto instance_count=actors.Size(),model_count=actors.ModelCount();
    graphics_owner.reset();runtime.Close();Check(runtime.Closed(),"render fixture did not release original actor owners");
    std::cout<<world<<" instances="<<instance_count<<" models="<<model_count
        <<" visible pixels, unchanged clocks, stale handles, release/reupload and new context passed\n";
}
}
int main(int argc,char** argv){
    try{
        if(argc!=3)throw std::invalid_argument("expected <original-gamedata> <capture-directory>");
        std::filesystem::create_directories(argv[2]);
        Run(argv[1],argv[2],"M01",8);Run(argv[1],argv[2],"M02",12);return 0;
    }catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
}
