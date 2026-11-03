#include "pal4/portable/uv_animation.h"
#include "rw_binary.h"
#include <rw.h>
#include <algorithm>
#include <cmath>
#include <map>
#include <mutex>
#include <set>

namespace pal4::portable {
namespace {
using rw::Need;
// librw's NULL backend never creates a platform window or GL context. Keep the
// singleton alive until all dictionaries die, and serialize its global state.
struct Library {
    std::mutex mutex;
    Library(){
        Need(::rw::Engine::init(nullptr)!=0,"UV library initialization failed");
        ::rw::registerUVAnimPlugin();
        Need(::rw::Engine::open(nullptr)!=0,"UV library open failed");
        Need(::rw::Engine::start()!=0,"UV library start failed");
    }
    ~Library(){::rw::Engine::stop();::rw::Engine::close();::rw::Engine::term();}
};
Library& Runtime(){static Library value;return value;}

// Validate stream bounds and graph invariants before invoking a legacy library
// reader. This is not a second animation evaluator. Initial links are unused by
// the library's first interpolation pairs, as confirmed by the SDK oracle.
void Validate(std::span<const std::uint8_t> bytes){
    rw::Input in(bytes);Need(bytes.size()<=8*1024*1024,"UV dictionary exceeds memory budget");
    const auto root=in.ReadChunk(0,in.Size());Need(root.id==0x2b&&root.end==in.Size(),"invalid UV dictionary");
    const auto parts=in.Children(root);const auto header=rw::First(parts,1);
    Need(header.end-header.begin==4,"invalid UV dictionary header");
    const auto count=in.U32(header.begin);Need(count>0&&count<=64,"invalid UV animation count");
    unsigned actual=0;std::set<std::string> names;
    for(auto part:parts){if(part.id!=0x1b)continue;++actual;
        rw::Range(part.begin,88,part.end);const auto base=part.begin;
        Need(in.U32(base)==0x100,"unsupported UV animation version");
        Need(in.U32(base+4)==0x1c0||in.U32(base+4)==0x1c1,"unsupported UV keyframe scheme");
        const auto frames=in.U32(base+8);Need(frames>=2&&frames<=65536,"invalid UV frame count");
        Need(part.end-base==88+std::size_t(frames)*32,"invalid UV animation size");
        Need(in.U32(base+12)==0,"unsupported UV animation flags");
        const auto duration=in.Float(base+16);Need(duration>0,"invalid UV duration");
        const auto name=in.Text(base+24,base+56);
        Need(!name.empty()&&name.size()<32&&names.insert(name).second,"invalid or duplicate UV animation name");
        unsigned nodes=0;
        for(unsigned i=1;i<frames;++i)if(in.U32(base+88+i*32+28)==0){nodes=i;break;}
        Need(nodes>0&&nodes<=8&&frames>=2*nodes,"UV frame topology has no initial interpolation pairs");
        // Custom data reserves eight map entries. Only getNumNodes() entries
        // are read by the public library evaluator; inactive tail slots in
        // original BJ dictionaries contain non-channel values in that unused tail.
        for(unsigned i=0;i<nodes;++i)Need(in.U32(base+56+i*4)<2,"unsupported active UV channel");
        std::vector<bool> used(frames);double previous_time=0;
        for(unsigned i=0;i<frames;++i){const auto p=base+88+i*32;const auto time=in.Float(p);
            Need(time>=previous_time&&time<=duration,"UV frame times are not ordered");previous_time=time;
            for(unsigned j=0;j<6;++j)(void)in.Float(p+4+j*4);
            if(i<nodes){
                Need(time==0,"UV initial frame time is not zero");
                Need(in.U32(p+28)!=0,"UV initial link prematurely terminates node discovery");
                continue;
            }
            const auto previous=in.U32(p+28);
            Need(previous<i&&!used[previous],"UV frame link is invalid or branched");used[previous]=true;
            Need(time>in.Float(base+88+previous*32),"UV interpolation interval is empty");
            if(i<2*nodes)Need(previous==i-nodes,"UV initial node pairing differs from library contract");
        }
    }
    Need(actual==count,"UV animation count mismatch");
}
}
struct UvAnimationDictionary::Impl {
    ::rw::UVAnimDictionary* dictionary{};
    explicit Impl(std::span<const std::uint8_t> bytes){
        Validate(bytes);auto& runtime=Runtime();std::lock_guard lock(runtime.mutex);
        ::rw::StreamMemory stream;stream.open(const_cast<std::uint8_t*>(bytes.data()),static_cast<::rw::uint32>(bytes.size()));
        Need(::rw::findChunk(&stream,::rw::ID_UVANIMDICT,nullptr,nullptr),"UV dictionary chunk missing");
        dictionary=::rw::UVAnimDictionary::streamRead(&stream);stream.close();
        Need(dictionary!=nullptr,"UV library could not decode dictionary");
    }
    ~Impl(){auto& runtime=Runtime();std::lock_guard lock(runtime.mutex);if(dictionary)dictionary->destroy();}
};
UvAnimationDictionary::UvAnimationDictionary(std::span<const std::uint8_t> bytes):impl_(std::make_unique<Impl>(bytes)){}
UvAnimationDictionary::~UvAnimationDictionary()=default;
std::vector<DrawAppearance> UvAnimationDictionary::Evaluate(const Scene& scene,float seconds) const {
    Need(std::isfinite(seconds)&&seconds>=0,"invalid UV animation time");
    auto& runtime=Runtime();std::lock_guard lock(runtime.mutex);
    std::vector<DrawAppearance> output;output.reserve(scene.draws.size());
    std::map<std::string,std::array<UvTransform,2>> evaluated;
    for(const auto& draw:scene.draws){
        DrawAppearance result;result.visible=true;std::copy_n(draw.color,4,result.color.begin());
        for(const auto& name:draw.uv_animations){if(name.empty())continue;
            auto found=evaluated.find(name);
            if(found==evaluated.end()){
                auto* anim=impl_->dictionary->find(name.c_str());Need(anim!=nullptr,"bound material UV animation is missing");
                auto* interp=::rw::AnimInterpolator::create(anim->getNumNodes(),anim->interpInfo->interpKeyFrameSize);
                Need(interp!=nullptr,"UV interpolator allocation failed");
                std::array<::rw::Matrix,2> transforms;for(auto& matrix:transforms)matrix.setIdentity();
                interp->setCurrentAnim(anim);
                // Upstream addTime discards the remainder when wrapping. Pass
                // an absolute reduced time and let its registered callbacks
                // perform all interpolation and matrix construction.
                float time=seconds;if(time>anim->duration){time=std::fmod(time,anim->duration);if(time==0)time=anim->duration;}
                interp->addTime(time);
                auto* custom=::rw::UVAnimCustomData::get(anim);
                for(int i=0;i<interp->numNodes;++i){::rw::Matrix matrix;matrix.setIdentity();interp->applyCB(&matrix,interp->getInterpFrame(i));transforms[custom->nodeToUVChannel[i]].transform(&matrix,::rw::COMBINEPRECONCAT);}
                interp->destroy();
                std::array<UvTransform,2> value;
                for(unsigned i=0;i<2;++i){const auto& m=transforms[i];value[i]={m.right.x,m.right.y,m.up.x,m.up.y,m.pos.x,m.pos.y};}
                found=evaluated.emplace(name,value).first;
            }
            // Slot composition uses the same library matrix operations as
            // UVAnim::applyUpdate; do not replace it with private SDK math.
            for(unsigned channel=0;channel<2;++channel){
                const auto& a=result.uv[channel];const auto& b=found->second[channel];
                ::rw::Matrix left,right;left.setIdentity();right.setIdentity();
                auto assign=[](::rw::Matrix& m,const UvTransform& v){m.right.x=v[0];m.right.y=v[1];m.up.x=v[2];m.up.y=v[3];m.pos.x=v[4];m.pos.y=v[5];m.update();};
                assign(left,a);assign(right,b);left.transform(&right,::rw::COMBINEPRECONCAT);
                result.uv[channel]={left.right.x,left.right.y,left.up.x,left.up.y,left.pos.x,left.pos.y};
            }
        }
        output.push_back(result);
    }
    return output;
}
}
