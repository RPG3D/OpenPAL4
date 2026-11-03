#include "pal4/portable/camera_tracks.h"
#include <lodepng.h>
#include "pal4/portable/scene_data.h"
#include "pal4/camera_task_object.h"
#include "pal4/script_camera_sequences.h"
#include "pal4/gom_task_base.h"
#include "pal4/gom_task_container.h"
#include "camera_track_geometry.h"
#include "pal4/float_interpolator.h"
#include <algorithm>
#include <bit>
#include <cmath>
#include <cstring>
#include <iomanip>
#include <map>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <type_traits>

namespace pal4::portable {
namespace {
void Require(bool valid,const char* error){if(!valid)throw std::runtime_error(error);}
bool Name(std::string_view name){return !name.empty() && name.size()<=64 && name.find_first_not_of("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_-")==std::string_view::npos;}
using namespace pal4;
// 原任务基类是真实C++对象；字节读写只访问它的对象表示，不把字节数组强转成类。
struct TaskImage {GomTaskBaseState base;std::array<std::byte,0x98-sizeof(GomTaskBaseState)> tail{};};
static_assert(sizeof(TaskImage)==0x98 && std::is_trivially_copyable_v<TaskImage>);
}
struct CameraTracks::Impl final:IScriptCameraSequenceRuntime,ICameraTaskPlaybackRuntime,
    IGomTaskContainerRuntime,IGomTaskBaseRuntime {
    static constexpr std::uint32_t subsystem=1,camera=0x1000,container_id=0x1010,camera_data=0x2000,
        cloud=2,executor=0x3000,record_base=0x1000000,xml_base=0x2000000,child_base=0x3000000,memory_base=0x10000000;
    struct Block {std::vector<std::byte> bytes;std::unique_ptr<TaskImage> task;};
    struct Wait {CameraSequenceWaitState state;ScriptToken token;bool blocking{},queued{},closed{};std::string error;};
    GameCameraSystem& cameras;CameraMemberReader read;
    detail::CameraTrackGeometry geometry;GomTaskBase base{*this};GomTaskContainer tasks{*this,base};GomTaskContainerState queue;
    std::map<std::uint32_t,Block> blocks;std::uint32_t next_memory{memory_base};
    std::vector<SceneCamera> records;std::vector<std::uint8_t> source;std::size_t source_bytes{};std::uint32_t source_crc{};bool retain_source{};
    std::vector<std::unique_ptr<Wait>> waits;
    std::string world,scene,file,selected;std::uint32_t active_task{},active_record{},prepare_calls{},run_calls{},completed{},frames{};
    std::uint32_t wait_calls{},wait_completed{};
    TaskImage last_task{};ScriptToken request_token{};std::array<float,3> listener_position{},listener_forward{},listener_up{};
    explicit Impl(GameCameraSystem& c,CameraMemberReader r,bool retain):cameras(c),read(std::move(r)),retain_source(retain){(void)tasks.Initialize(queue,0);}
    ~Impl() override{try{(void)tasks.Destroy(queue);}catch(...){}}
    static std::uint32_t Record(std::size_t index){Require(index<4096,"camera record handle limit");return record_base+static_cast<std::uint32_t>(index)*0x1000;}
    std::size_t RecordIndex(std::uint32_t address) const {
        Require(address>=record_base && (address-record_base)%0x1000==0 && (address-record_base)/0x1000<records.size(),"invalid camera record handle");return (address-record_base)/0x1000;
    }
    std::span<std::byte> Memory(std::uint32_t address,std::size_t count) {
        auto it=blocks.upper_bound(address);Require(it!=blocks.begin(),"unknown camera allocation");--it;
        auto bytes=it->second.task?std::as_writable_bytes(std::span(it->second.task.get(),1)):std::span(it->second.bytes);
        const auto offset=address-it->first;Require(offset<=bytes.size() && count<=bytes.size()-offset,"camera allocation bounds");return bytes.subspan(offset,count);
    }
    std::uint32_t AllocateMemory(std::uint32_t bytes) override {
        Require(bytes>0 && bytes<0x1000 && next_memory<0xf0000000,"camera allocation limit");
        const auto token=next_memory;next_memory+=0x1000;Block block;block.bytes.resize(bytes);blocks.emplace(token,std::move(block));return token;
    }
    std::uint32_t FreeMemory(std::uint32_t token) override {if(!token)return 0;Require(blocks.erase(token)==1,"invalid camera allocation free");return 0;}
    std::uint32_t ReadWord(std::uint32_t address) override {
        if(address>=camera_data+0x3c && address<=camera_data+0x88)return cameras.TaskWord(0,address-camera_data);
        if(address>=record_base && address<xml_base) {
            const auto token=record_base+((address-record_base)/0x1000)*0x1000;const auto index=RecordIndex(token);const auto offset=address-token;
            if(offset==0x40)return xml_base+static_cast<std::uint32_t>(index)*0x1000;
            Require(offset>=0xc && offset<=0x3c && offset%4==0,"invalid camera record field");return records[index].values[(offset-0xc)/4];
        }
        const auto bytes=Memory(address,4);std::uint32_t value;std::memcpy(&value,bytes.data(),4);return value;
    }
    void WriteWord(std::uint32_t address,std::uint32_t value) override{auto bytes=Memory(address,4);std::memcpy(bytes.data(),&value,4);}
    std::uint8_t ReadByte(std::uint32_t address) override{return std::to_integer<std::uint8_t>(Memory(address,1)[0]);}
    void WriteByte(std::uint32_t address,std::uint8_t value) override{Memory(address,1)[0]=std::byte(value);}
    void CopyCString(std::uint32_t destination,const char* text,std::uint32_t bytes) override{auto target=Memory(destination,bytes);std::memcpy(target.data(),text,bytes);}
    GomTaskBaseState& TaskAt(std::uint32_t address) override {const auto it=blocks.find(address);Require(it!=blocks.end() && bool(it->second.task),"invalid camera task handle");return it->second.task->base;}
    void BaseConstruct(std::uint32_t object) override{(void)base.Construct(TaskAt(object));}
    std::uint32_t BaseDestruct(std::uint32_t object) override{return base.Destroy(TaskAt(object));}
    void DeleteTask(std::uint32_t task,std::uint32_t flags) override {
        Require(flags==1 && task==active_task,"invalid camera task deletion");last_task=*blocks.at(task).task;
        (void)CameraTaskObject::Destruct(task,*this);(void)FreeMemory(task);active_task=0;
    }
    bool IsWaitComplete(std::uint32_t task) override{return base.IsWaitComplete(TaskAt(task));}
    void BeginRun(std::uint32_t task) override{Require(CameraTaskObject::Enter(task,*this,geometry)!=0,"camera task enter failed");}
    void Tick(std::uint32_t task,float elapsed) override{(void)CameraTaskObject::Update(task,elapsed,*this);}
    bool IsDurationComplete(std::uint32_t task) override{return ReadByte(task+0x3c)!=0;}
    void Finish(std::uint32_t task) override{(void)TaskAt(task);}
    std::uint32_t CameraSubsystem() override{return subsystem;}
    std::uint32_t ActiveCameraObject(std::uint32_t owner) override{Require(owner==subsystem,"invalid camera subsystem");return camera;}
    void ClearCameraCollection(std::uint32_t value) override {
        Require(value==camera,"invalid camera collection");(void)tasks.CleanupAndReady(queue);
        // 原等待器观察整个容器；替换任务后，旧等待同样等待新容器完成。
    }
    std::string_view ArenaWorld() override{return world;}
    std::string_view ArenaScene() override{return scene;}
    std::uint32_t CloudManager() override{return cloud;}
    std::uint32_t LoadCameraFile(std::uint32_t owner,std::string_view w,std::string_view s,std::string_view name) override {
        Require(owner==cloud && Name(w) && Name(s) && Name(name),"invalid camera file request");
        file=std::string(w)+"\\"+std::string(s)+"\\"+std::string(name)+".cam";records.clear();std::vector<std::uint8_t>().swap(source);source_bytes=0;source_crc=0;
        const auto bytes=read(file);if(!bytes)return 0;
        Require(bytes->size()<=8*1024*1024,"camera file exceeds limit");
        std::istringstream input(std::string(bytes->begin(),bytes->end()));records=ReadSceneCameras(input);
        Require(records.size()<=4096,"too many camera records");
        for(const auto& record:records){
            for(unsigned i:{0u,1u,2u,3u,4u,5u,8u,10u})Require(std::isfinite(std::bit_cast<float>(record.values[i])),"nonfinite camera record");
            const auto duration=std::bit_cast<float>(record.values[10]);Require(record.values[9]<=1 && duration>=0 && (record.values[11]!=0 || duration>0),"invalid camera duration/anchor");
        }
        source_bytes=bytes->size();source_crc=lodepng_crc32(bytes->data(),bytes->size());if(retain_source)source=*bytes;return 1;
    }
    std::uint32_t FindCloud(std::uint32_t owner,std::string_view name) override{Require(owner==cloud,"invalid camera record owner");for(std::size_t i=0;i<records.size();++i)if(records[i].name==name)return Record(i);return 0;}
    std::uint32_t FirstCloud(std::uint32_t) override{throw std::logic_error("camera circle sequence is not bound");}
    std::uint32_t NextCloud(std::uint32_t) override{throw std::logic_error("camera circle sequence is not bound");}
    CameraSequenceControl ReadCloudControl(std::uint32_t record) override{const auto& r=records[RecordIndex(record)];return {r.values[10],r.values[11]};}
    Wait& FindWait(CameraSequenceWaitState& state){for(auto& wait:waits)if(&wait->state==&state)return *wait;throw std::logic_error("invalid camera wait handle");}
    CameraSequenceWaitState* AllocateWait(std::uint32_t size) override {Require(size==sizeof(CameraSequenceWaitState),"invalid camera wait size");auto wait=std::make_unique<Wait>();wait->token=request_token;waits.push_back(std::move(wait));return &waits.back()->state;}
    std::uint32_t ScriptExecutor() override{return executor;}
    void ConstructWait(CameraSequenceWaitState& state,std::uint8_t mode,std::uint32_t owner) override{Require(mode<=1 && owner==executor+ScriptCameraSequences::kQueueOffset,"invalid camera wait binding");FindWait(state).blocking=mode!=0;}
    std::uint32_t CreateCameraTask() override {
        Require(!active_task,"camera task was not cleared");const auto token=AllocateMemory(0x98);auto& block=blocks.at(token);block.bytes.clear();block.task=std::make_unique<TaskImage>();active_task=token;
        (void)CameraTaskObject::Construct(token,*this);return token;
    }
    std::uint32_t TaskContainer(std::uint32_t owner) override{Require(owner==camera,"invalid camera task owner");return container_id;}
    void AddCameraTask(std::uint32_t owner,std::uint32_t task,std::string_view name,const std::array<std::uint32_t,3>& options) override {
        Require(owner==container_id && name=="synchroCameraRun-CLD" && options==std::array<std::uint32_t,3>{3,0,0},"invalid camera task timing");
        const std::string text(name);(void)tasks.AddTaskAndInitialize(queue,container_id,TaskAt(task),task,text.c_str(),{3,0,0});
    }
    std::uint32_t CameraInternalId(std::uint32_t owner) override{Require(owner==camera,"invalid camera object");return camera_data;}
    void ApplyCameraRecord(std::uint32_t task,std::uint32_t data,std::uint32_t record) override{Require(task==active_task && data==camera_data,"invalid camera task configure");active_record=record;selected=records[RecordIndex(record)].name;(void)CameraTaskObject::Configure(task,data,record,*this);}
    void MarkProcessed(std::uint32_t owner) override{Require(owner==camera,"invalid camera schedule owner");(void)tasks.BeginWaiting(queue);}
    void FlushWait(CameraSequenceWaitState& state) override{(void)FindWait(state);}
    void CloseWait(CameraSequenceWaitState& state) override{FindWait(state).closed=true;}
    std::uint32_t AddWait(std::uint32_t owner,CameraSequenceWaitState* state) override{Require(owner==executor+ScriptCameraSequences::kQueueOffset && state,"invalid camera wait queue");FindWait(*state).queued=true;return 1;}
    bool CameraCompleted(std::uint32_t owner) override{Require(owner==camera,"invalid camera completion owner");return queue.ready && queue.count==0;}
    CameraSequenceVector ReadVector(std::uint32_t address) override{return {std::bit_cast<float>(ReadWord(address)),std::bit_cast<float>(ReadWord(address+4)),std::bit_cast<float>(ReadWord(address+8))};}
    std::uint32_t AudioManager() override{return 3;}
    std::uint32_t AudioHandle(std::uint32_t manager) override{Require(manager==3,"invalid audio listener manager");return 4;}
    void SetListenerPosition(std::uint32_t handle,CameraSequenceVector value) override{Require(handle==4,"invalid audio listener handle");listener_position={value.x,value.y,value.z};}
    std::uint32_t SetListenerOrientation(std::uint32_t handle,CameraSequenceVector forward,CameraSequenceVector up) override{Require(handle==4,"invalid audio listener handle");listener_forward={forward.x,forward.y,forward.z};listener_up={up.x,up.y,up.z};return 1;}
    std::uint32_t FindXmlChild(std::uint32_t document,std::string_view name) override {
        Require(document>=xml_base && (document-xml_base)%0x1000==0,"invalid camera XML handle");const auto i=(document-xml_base)/0x1000;Require(i<records.size(),"missing camera XML");
        const auto& children=records[i].config.Root().children;
        for(std::size_t n=0;n<children.size();++n)if(children[n].name==name){Require(n<256,"camera XML child limit");return child_base+static_cast<std::uint32_t>(i)*0x1000+static_cast<std::uint32_t>(n)*4;}
        return 0;
    }
    std::optional<float> ConfigFloat(std::uint32_t child,std::string_view key) override {
        if(!child)return {};
        Require(child>=child_base && (child-child_base)%4==0,"invalid camera XML child");
        const auto i=(child-child_base)/0x1000,n=(child-child_base)%0x1000/4;
        Require(i<records.size() && n<records[i].config.Root().children.size(),"unknown camera XML child");
        const auto* attr=records[i].config.Root().children[n].FindAttribute(key);if(!attr)return {};
        const auto result=attr->ToFloat(0);Require(std::isfinite(result),"nonfinite camera XML float");return result;
    }
    void SetLookAt(std::uint32_t data,CameraTaskVector value,std::uint32_t flag) override{Require(data==camera_data,"invalid track camera");cameras.TrackVector(0,false,{value.x,value.y,value.z},flag);}
    void SetPosition(std::uint32_t data,CameraTaskVector value,std::uint32_t flag) override{SetLookAt(data,value,flag);}
    void SetEye(std::uint32_t data,CameraTaskVector value,std::uint32_t flag) override{Require(data==camera_data,"invalid track camera");cameras.TrackVector(0,true,{value.x,value.y,value.z},flag);}
    void Scalar(std::uint32_t data,unsigned parameter,float value,std::uint32_t flag){Require(data==camera_data,"invalid track camera");cameras.TrackScalar(0,parameter,value,flag);}
    void SetAngle(std::uint32_t data,float value,std::uint32_t flag) override{Scalar(data,0,value,flag);}
    void SetSecondAngle(std::uint32_t data,float value,std::uint32_t flag) override{Scalar(data,1,value,flag);}
    void SetThirdAngle(std::uint32_t data,float value,std::uint32_t flag) override{Scalar(data,2,value,flag);}
    void SetScalar(std::uint32_t data,float value,std::uint32_t flag) override{Scalar(data,3,value,flag);}
    float InterpolateFloat(float a,float b,float t,std::uint32_t mode) override {
        struct Math:IFloatInterpolationRuntime{double CosineApproximation(float) override{throw std::logic_error("camera task requires linear interpolation");}} math;
        return static_cast<float>(FloatInterpolator::Interpolate(a,b,t,static_cast<int>(mode),math));
    }
};
CameraTracks::CameraTracks(GameCameraSystem& cameras,CameraMemberReader read,bool retain_source):impl_(std::make_unique<Impl>(cameras,std::move(read),retain_source)){}
CameraTracks::~CameraTracks()=default;
ScriptResponse CameraTracks::Prepare(std::string_view world,std::string_view scene,std::string_view name) {
    Require(Name(world) && Name(scene) && Name(name),"invalid camera prepare name");impl_->world=world;impl_->scene=scene;++impl_->prepare_calls;
    const bool result=pal4::ScriptCameraSequences(*impl_).Prepare(name);return {std::int32_t(result),false};
}
ScriptResponse CameraTracks::RunSingle(ScriptToken token,std::string_view name,bool wait) {
    Require(Name(name),"invalid camera record name");impl_->request_token=token;++impl_->run_calls;
    const bool result=pal4::ScriptCameraSequences(*impl_).RunSingle(name,wait?1:0);return {std::int32_t(result),result && wait};
}
std::vector<CameraTrackCompletion> CameraTracks::Advance(float elapsed) {
    Require(std::isfinite(elapsed) && elapsed>=0,"invalid camera track time");if(impl_->queue.count)++impl_->frames;
    (void)impl_->tasks.Update(impl_->queue,elapsed);std::vector<CameraTrackCompletion> done;
    for(auto& wait:impl_->waits)if(wait->queued){
        const bool standalone=wait->state.vtable_address==pal4::ScriptCameraSequences::kWaitVtable;
        if(!wait->closed){
            if(standalone)pal4::ScriptCameraSequences(*impl_).UpdateWaitHandler(wait->state);
            else (void)pal4::ScriptCameraSequences(*impl_).UpdateAudioListener(wait->state);
        }
        if(wait->closed){if(wait->blocking)done.push_back({wait->token,wait->error});if(standalone)++impl_->wait_completed;else ++impl_->completed;wait->queued=false;}
    }
    std::erase_if(impl_->waits,[](const auto& wait){return !wait->queued;});return done;
}
bool CameraTracks::Playing() const{return impl_->queue.count!=0;}
ScriptResponse CameraTracks::WaitCurrent(ScriptToken token){impl_->request_token=token;++impl_->wait_calls;(void)pal4::ScriptCameraSequences(*impl_).CreateWaitHandler();return {{},true};}
bool CameraTracks::Waiting() const{return !impl_->waits.empty();}
void CameraTracks::WriteReport(std::ostream& out,bool include_source) const {
    const auto& s=*impl_;out<<std::setprecision(9)<<"{\"prepare_calls\":"<<s.prepare_calls<<",\"run_calls\":"<<s.run_calls<<",\"completed\":"<<s.completed
        <<",\"wait_calls\":"<<s.wait_calls<<",\"wait_completed\":"<<s.wait_completed<<",\"pending_waits\":"<<s.waits.size()
        <<",\"frames\":"<<s.frames<<",\"member\":\"";
    for(char c:s.file){if(c=='\\')out<<"\\\\";else out<<c;}
    out<<"\",\"bytes\":"<<s.source_bytes<<",\"source_crc32\":"<<s.source_crc<<",\"records\":"<<s.records.size()<<",\"selected\":\""<<s.selected<<"\",\"tasks\":"<<s.queue.count<<",\"ready\":"<<(s.queue.ready?"true":"false");
    const TaskImage* task=s.active_task?s.blocks.at(s.active_task).task.get():&s.last_task;
    const auto word=[&](unsigned offset){std::uint32_t value;std::memcpy(&value,reinterpret_cast<const std::byte*>(task)+offset,4);return value;};
    out<<",\"phase\":"<<task->base.phase<<",\"task_time\":"<<std::bit_cast<float>(word(0x40))<<",\"duration\":"<<std::bit_cast<float>(word(0x44))<<",\"task_completed\":"<<(word(0x3c)&255?"true":"false");
    const auto pose=s.cameras.Pose();const auto array=[&](std::string_view name,const auto& values){out<<",\""<<name<<"\":[";for(std::size_t i=0;i<values.size();++i){if(i)out<<',';out<<values[i];}out<<']';};
    array("eye",pose.eye);array("target",pose.target);array("angles",pose.angles);array("listener_position",s.listener_position);array("listener_forward",s.listener_forward);array("listener_up",s.listener_up);out<<",\"distance\":"<<pose.distance;
    array("matrix",s.cameras.Matrix(16.f/9));
    if(include_source && s.retain_source){out<<",\"source_hex\":\"";constexpr char hex[]="0123456789abcdef";for(auto b:s.source)out<<hex[b>>4]<<hex[b&15];out<<'"';}
    out<<'}';
}
}
