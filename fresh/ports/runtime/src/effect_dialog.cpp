#include "pal4/portable/effect_dialog.h"
#include "pal4/dialog_object.h"
#include "pal4/effect_system_callbacks.h"
#include <algorithm>
#include <bit>
#include <cmath>
#include <stdexcept>
#include <vector>
namespace pal4::portable {
namespace {
void Require(bool value,const char* message){if(!value)throw std::logic_error(message);}
constexpr unsigned kOwner=1,kDialog=2,kText=3,kContainer=4,kRoot=5,kParent=6;
constexpr unsigned kTasks=0x1000,kRecords=0x10000,kObjects=0x20000,kFrames=0x30000;
}
struct EffectDialog::Impl final:IDialogObjectResumeRuntime,IDialogObjectStartRuntime,
    IDialogObjectPauseRuntime,IDialogObjectUpdateRuntime,IDialogObjectActivationRuntime,
    IDialogObjectStopRuntime {
    EffectSequence sequence;
    ParticleRandom& random;
    DialogObjectState state{};
    std::vector<unsigned> references;
    std::vector<bool> queued,attached;
    std::size_t task_count;
    bool root_alive{},container_alive{},parent_attached{},distance_clip{};
    EffectMatrix parent=IdentityEffectMatrix(),local=IdentityEffectMatrix(),world=IdentityEffectMatrix();
    // Reset/Cleanup的ProcessEffectEvent是出队，与Activation/Resume的入队分离。
    struct ResetAdapter final:IDialogObjectResetRuntime {
        Impl& s;explicit ResetAdapter(Impl& owner):s(owner){}
        void CleanupObjectCollection(unsigned c) override{s.CleanupObjectCollection(c);}
        void DestroyTaskContainer(unsigned c) override{s.DestroyTaskContainer(c);}
        void Free(unsigned p) override{s.Free(p);}
        unsigned ReadWord(unsigned a,unsigned o) override{return s.ReadWord(a,o);}
        void WriteWord(unsigned a,unsigned o,unsigned v) override{s.WriteWord(a,o,v);}
        bool EffectsEnabled() override{return false;}
        bool EffectIsActive(unsigned e) override{return s.EffectIsActive(e);}
        void SetEffectMode(unsigned e,unsigned m) override{s.SetEffectMode(e,m);}
        void ReleaseEffectReferences(unsigned e) override{s.sequence.HideObject(s.Index(e));}
        void ProcessEffectEvent(unsigned owner,unsigned e) override{s.CheckOwner(owner);s.queued[s.Index(e)]=false;}
    };
    struct CleanupAdapter final:IDialogObjectEffectCleanupRuntime {
        Impl& s;explicit CleanupAdapter(Impl& owner):s(owner){}
        unsigned ReadWord(unsigned a,unsigned o) override{return s.ReadWord(a,o);}
        void WriteWord(unsigned a,unsigned o,unsigned v) override{s.WriteWord(a,o,v);}
        bool EffectsEnabled() override{return false;}
        bool EffectIsActive(unsigned e) override{return s.EffectIsActive(e);}
        unsigned EffectFrame(unsigned e) override{return s.EffectFrame(e);}
        unsigned FrameParent(unsigned f) override{return s.FrameParent(f);}
        void RemoveFrameChild(unsigned f) override{s.RemoveFrameChild(f);}
        void ProcessEffectEvent(unsigned owner,unsigned e) override{s.CheckOwner(owner);s.queued[s.Index(e)]=false;}
        void DestroyEffect(unsigned e) override{s.sequence.ReleaseObject(s.Index(e));}
    };
    Impl(EffectPlan plan,EffectTextures& textures,ParticleRandom& rng,ParticleCounts& pc,EffectObjectCounts& cc)
        :sequence(plan,textures,rng,pc,cc,false),random(rng),references(sequence.ObjectCount()),
         queued(sequence.ObjectCount()),attached(sequence.ObjectCount()),task_count(plan.task_ids.size()){
        Require(task_count>0 && task_count<=4096,"invalid Dialog task count");
        Require(plan.camera_start==1.f,"Dialog camera probability is not bound");
        distance_clip=plan.distance_clip;
        state.words[3]=kOwner;state.words[4]=kText;state.words[21]=std::bit_cast<unsigned>(plan.camera_start);
        state.words[24]=kTasks;state.words[25]=kTasks+static_cast<unsigned>(task_count)*8;
        state.words[28]=kRecords;state.words[29]=kRecords+static_cast<unsigned>(references.size())*20;
        for(unsigned i:{47u,48u,49u})state.words[i]=std::bit_cast<unsigned>(1.f);
    }
    ~Impl(){sequence.DestroyTasks();CleanupAdapter cleanup(*this);(void)DialogObject::StopEffects(state,cleanup);}
    std::size_t Index(unsigned e) const{Require(e>=kObjects && e-kObjects<references.size(),"invalid Dialog effect token");const auto i=e-kObjects;Require(sequence.HasObject(i),"released Dialog effect token");return i;}
    std::size_t Record(unsigned a) const{Require(a>=kRecords && (a-kRecords)%20==0 && (a-kRecords)/20<references.size(),"invalid Dialog object record");return (a-kRecords)/20;}
    void CheckOwner(unsigned owner) const{Require(owner==kOwner,"invalid Dialog owner");}
    void CheckContainer(unsigned c) const{Require(c==kContainer && container_alive,"invalid Dialog task container");}
    void RefreshWorld(){world=parent_attached?MultiplyEffectMatrices(parent,local):local;for(std::size_t i=0;i<attached.size();++i)if(attached[i] && sequence.HasObject(i))sequence.SetObjectParentTransform(i,world);}
    unsigned ReadWord(unsigned a,unsigned o) override{const auto i=Record(a);if(o==4)return kObjects+static_cast<unsigned>(i);Require(o==16,"unsupported Dialog record read");return references[i];}
    void WriteWord(unsigned a,unsigned o,unsigned v) override{
        if(a>=kTasks && a<kTasks+task_count*8){Require((a-kTasks)%8==0 && o==4 && v==0,"invalid Dialog task record clear");return;}
        const auto i=Record(a);Require(o==16 && (v==0 || v==kObjects+i),"invalid Dialog object reference");references[i]=v;
    }
    std::string_view ReadText(unsigned a) override{Require(a>=kObjects && a-kObjects<references.size(),"invalid Dialog text token");return sequence.ObjectName(a-kObjects);}
    unsigned ResolveTextProperty(unsigned manager,std::string_view name) override{Require(manager==kText,"invalid Dialog text manager");for(std::size_t i=0;i<references.size();++i)if(sequence.ObjectName(i)==name)return kObjects+static_cast<unsigned>(i);throw std::logic_error("missing Dialog effect template");}
    unsigned HandleEffectEvent(unsigned property,unsigned object,unsigned owner) override{
        Require(object==kDialog,"invalid Dialog context");CheckOwner(owner);Require(property>=kObjects && property-kObjects<references.size(),"invalid Dialog template token");
        sequence.EnsureObject(property-kObjects);return property;
    }
    bool EffectsEnabled() override{return false;}
    bool EffectIsActive(unsigned e) override{return sequence.IsDummy(Index(e));}
    void ProcessEffectEvent(unsigned owner,unsigned e) override{CheckOwner(owner);queued[Index(e)]=true;}
    unsigned EffectFrame(unsigned e) override{return kFrames+static_cast<unsigned>(Index(e));}
    void AddFrameChild(unsigned root,unsigned frame) override{Require(root==kRoot && root_alive && frame>=kFrames && frame-kFrames<attached.size(),"invalid Dialog child frame");attached[frame-kFrames]=true;RefreshWorld();}
    unsigned FrameParent(unsigned frame) override{if(frame==kRoot){Require(root_alive,"missing Dialog root frame");return parent_attached?kParent:0;}Require(frame>=kFrames && frame-kFrames<attached.size(),"invalid Dialog frame");return attached[frame-kFrames]?kRoot:0;}
    void RemoveFrameChild(unsigned frame) override{if(frame==kRoot){parent_attached=false;RefreshWorld();return;}Require(frame>=kFrames && frame-kFrames<attached.size(),"invalid Dialog frame");const auto i=frame-kFrames;attached[i]=false;sequence.SetObjectParentTransform(i,IdentityEffectMatrix());}
    void ResetBeforePlayback(DialogObjectState& s) override{ResetAdapter reset(*this);(void)DialogObject::ResetRuntimeState(s,reset);}
    void DestroyFrame(unsigned f) override{Require(f==kRoot && root_alive,"invalid Dialog frame destroy");for(std::size_t i=0;i<attached.size();++i)if(attached[i]){sequence.SetObjectParentTransform(i,IdentityEffectMatrix());attached[i]=false;}root_alive=false;parent_attached=false;}
    unsigned CreateFrame() override{Require(!root_alive,"Dialog root already exists");root_alive=true;return kRoot;}
    void RefreshFrame(DialogObjectState& s) override{
        Require(root_alive && s.words[43]==kRoot,"missing Dialog frame");
        auto point=[&](unsigned i){return Point3{std::bit_cast<float>(s.words[i]),std::bit_cast<float>(s.words[i+1]),std::bit_cast<float>(s.words[i+2])};};
        local=InstanceMatrix(point(44),point(50));const auto scale=point(47);for(unsigned c=0;c<3;++c)for(unsigned r=0;r<3;++r)local[c*4+r]*=scale[c];RefreshWorld();
    }
    void DestroyTaskContainer(unsigned c) override{CheckContainer(c);sequence.DestroyTasks();container_alive=false;}
    void Free(unsigned a) override{Require(a==kContainer && !container_alive,"invalid Dialog container free");}
    unsigned Allocate(unsigned bytes) override{Require(bytes==16 && !container_alive,"invalid Dialog container allocation");return kContainer;}
    unsigned InitializeTaskContainer(unsigned a) override{Require(a==kContainer && !container_alive,"invalid Dialog container initialization");container_alive=true;return a;}
    bool ActivateEffects(DialogObjectState& s) override{return DialogObject::ActivateEffects(s,kDialog,*this);}
    void StopEffects(DialogObjectState& s) override{sequence.DestroyTasks();CleanupAdapter cleanup(*this);(void)DialogObject::StopEffects(s,cleanup);}
    bool InitializeTasks(DialogObjectState&) override{CheckContainer(kContainer);sequence.BeginTasks(false);return true;}
    void ResetTaskSlots(DialogObjectState&) override{sequence.ClearTasks();}
    unsigned ReadEffectHandle(unsigned r) override{return ReadWord(r,16);}
    void SetEffectMode(unsigned e,unsigned mode) override{Require(mode<=1,"invalid Dialog effect mode");sequence.SetObjectMode(Index(e),mode!=0);}
    void WriteEffectWord(unsigned e,unsigned offset,unsigned value) override{(void)Index(e);Require((offset==16 && value==kOwner)||(offset==12 && value==kDialog),"unsupported Dialog effect metadata");}
    bool RestartEffect(unsigned e) override{sequence.ResetObject(Index(e));return true;}
    int Random15() override{return random.Next15();}
    bool ResumePlayback(DialogObjectState& s) override{return DialogObject::ResumePlayback(s,kDialog,*this);}
    bool PreparePlayback(DialogObjectState& s) override{return DialogObject::PreparePlayback(s,*this);}
    void PauseEffect(unsigned e) override{sequence.PreRenderObject(Index(e));}
    void BeginWaiting(unsigned c) override{CheckContainer(c);sequence.BeginWaiting();}
    int UpdateTaskContainer(unsigned c,float dt) override{CheckContainer(c);sequence.AdvanceTasks(dt);return 0;}
    bool TaskContainerReady(unsigned c) override{CheckContainer(c);return sequence.TasksFinished();}
    int ResetRuntimeState(DialogObjectState& s) override{ResetBeforePlayback(s);return 0;}
    std::array<float,3> ObjectPosition(unsigned) override{throw std::logic_error("Dialog frame tracking is not bound");}
    int SetAbsolutePosition(DialogObjectState&,const std::array<float,3>&) override{throw std::logic_error("Dialog frame tracking is not bound");}
    void CleanupObjectCollection(unsigned c) override{CheckContainer(c);sequence.ClearTasks();}
    void InitializeEffectSystem() override{}
    double EffectValue(unsigned e) override{return sequence.ObjectDuration(Index(e));}
};
EffectDialog::EffectDialog(EffectPlan p,EffectTextures& t,ParticleRandom& r,ParticleCounts& pc,EffectObjectCounts& cc):impl_(std::make_unique<Impl>(std::move(p),t,r,pc,cc)){}
EffectDialog::~EffectDialog()=default;
bool EffectDialog::Prepare(){return DialogObject::StartPlayback(impl_->state,*impl_);}
bool EffectDialog::BeginWaiting(){return DialogObject::PausePlayback(impl_->state,*impl_);}
void EffectDialog::AdvanceTasks(float dt){Require(std::isfinite(dt)&&dt>=0,"invalid Dialog delta");(void)DialogObject::UpdatePlayback(impl_->state,dt,*impl_);}
void EffectDialog::AdvanceObjects(double dt){Require(std::isfinite(dt)&&dt>=0,"invalid Dialog object delta");for(std::size_t i=0;i<impl_->queued.size();++i)if(impl_->queued[i])impl_->sequence.AdvanceObject(i,dt);}
void EffectDialog::Reset(){impl_->ResetBeforePlayback(impl_->state);}
void EffectDialog::ReturnToCache(){Require(!HasTasks() && QueuedObjects()==0,"reset Dialog before returning to cache");impl_->state.words[1]=(impl_->state.words[1]&~0xffu)|1u;impl_->state.words[20]&=~0xffu;if(impl_->root_alive && impl_->parent_attached)impl_->RemoveFrameChild(kRoot);}
double EffectDialog::Stop(bool detach){return DialogObject::StopPlayback(impl_->state,detach,*impl_);}
void EffectDialog::SetParentTransform(const EffectMatrix& matrix){ValidateEffectMatrix(matrix);Require(impl_->root_alive,"prepare Dialog before attaching parent");impl_->parent=matrix;impl_->parent_attached=true;impl_->RefreshWorld();}
void EffectDialog::RestoreDetachedTransform(const EffectMatrix& matrix){ValidateEffectMatrix(matrix);Require(impl_->root_alive && !impl_->parent_attached,"Dialog frame must be detached before restoring LTM");impl_->local=matrix;impl_->RefreshWorld();}
void EffectDialog::SetTransform(Point3 p,Point3 r,Point3 s){const auto matrix=InstanceMatrix(p,r);ValidateEffectMatrix(matrix);for(float v:s)Require(std::isfinite(v),"invalid Dialog scale");for(unsigned i=0;i<3;++i){impl_->state.words[44+i]=std::bit_cast<unsigned>(p[i]);impl_->state.words[47+i]=std::bit_cast<unsigned>(s[i]);impl_->state.words[50+i]=std::bit_cast<unsigned>(r[i]);}if(impl_->root_alive)impl_->RefreshFrame(impl_->state);}
bool EffectDialog::HasTasks() const{return impl_->state.words[2]!=0;}
bool EffectDialog::Prepared() const{return (impl_->state.words[20]&0xff)!=0;}
bool EffectDialog::Waiting() const{return (impl_->state.words[20]&0xff00)!=0;}
std::size_t EffectDialog::QueuedObjects() const{return static_cast<std::size_t>(std::count(impl_->queued.begin(),impl_->queued.end(),true));}
const EffectMatrix& EffectDialog::WorldTransform() const{return impl_->world;}
const EffectSequence& EffectDialog::Sequence() const{return impl_->sequence;}
bool EffectDialog::RenderAllowed(std::size_t index,Point3 camera,float radius) const{
    const auto& sequence=impl_->sequence;
    if(sequence.IsDummy(index))return false;
    if(!impl_->distance_clip)return true;
    Require(std::isfinite(radius) && radius>=0,"invalid effect distance radius");
    for(float value:camera)Require(std::isfinite(value),"invalid effect camera position");
    const auto& matrix=sequence.Particle(index)?sequence.Particle(index)->Transform():sequence.Cylinder(index)->Transform();
    struct Proximity final:pal4::IEffectProximityRuntime {
        Point3 camera;float squared;
        Proximity(Point3 p,float r):camera(p),squared(r*r){}
        std::uint8_t ReadByte(unsigned a) override{Require(a==0x1004,"invalid effect gate byte");return 1;}
        unsigned ReadWord(unsigned a) override{Require(a==0x2004,"invalid effect camera frame");return 0x3000;}
        float ReadFloat(unsigned a) override{Require(a==0x1008,"invalid effect radius field");return squared;}
        pal4::EffectPosition ObjectPosition(unsigned h) override{Require(h==0x3000,"invalid effect frame token");return {camera[0],camera[1],camera[2]};}
    } runtime(camera,radius);
    return pal4::EffectSystemCallbacks::ValidateObjectTransform(0x1000,0x2000,{matrix[12],matrix[13],matrix[14]},runtime);
}
}
