#include "pal4/portable/actor.h"
#include <cmath>
#include <bit>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <type_traits>

namespace {
using namespace pal4::portable;
void Check(bool condition) { if (!condition) throw std::runtime_error("actor contract failed"); }
template<class T> void Add(std::string& b,const T& value) {
    static_assert(std::is_trivially_copyable_v<T>,"serialize transport fields, never an owning host object");
    b.append(reinterpret_cast<const char*>(&value),sizeof value);
}
void MatrixBytes(std::string& b,float translation) {
    const Matrix m{1,0,0,0,0,1,0,0,0,0,1,0,translation,0,0,1};
    for (float x:m) Add(b,x);
}
std::string Fixture() {
    std::string mesh="P4SC";
    for (std::uint32_t v:{1u,1u,3u}) Add(mesh,v);
    for (float x:{1.f,0.f,0.f,1.f,0.f,0.f,1.f,1.f,1.f,0.f}) Add(mesh,x);
    // P4SC v1 stores exactly32 bytes per draw. Draw now owns animation names;
    // its host layout must never become fixture bytes or a cache ABI.
    Add(mesh,0u);Add(mesh,3u);Add(mesh,-1);Add(mesh,-1);
    for(float value:{1.f,1.f,1.f,1.f})Add(mesh,value);
    for (int i=0;i<3;++i) Add(mesh,Vertex{{1,0,0},{0,0},{0,0},{1,1,1,1}});
    Add(mesh,0u);
    std::string b="P4AR";
    for (std::uint32_t v:{1u,static_cast<std::uint32_t>(mesh.size()),1u,1u,3u,1u}) Add(b,v);
    b+=mesh; Add(b,-1); MatrixBytes(b,2); Add(b,0u); MatrixBytes(b,-2);
    for (int i=0;i<3;++i) Add(b,SkinInfluence{{0,0,0,0},{1,0,0,0}});
    Add(b,4u); b+="test"; Add(b,1.f); Add(b,1u); Add(b,0u); Add(b,2u);
    Add(b,AnimationKey{0,{0,0,0,1},{4,0,0}});
    Add(b,AnimationKey{1,{0,0,1,0},{6,0,0}});
    return b;
}
Actor Parse(const std::string& b) { std::istringstream stream(b); return ReadActor(stream); }
void Reject(const std::string& b) {
    bool failed=false; try { (void)Parse(b); } catch (const std::runtime_error&) { failed=true; } Check(failed);
}
template<class T> void Patch(std::string& b,std::size_t p,T value) { std::memcpy(b.data()+p,&value,sizeof value); }
}
int main(int argc,char** argv) {
    try {
        if(argc==3 && std::string_view(argv[1])=="bounds"){
            std::ifstream stream(argv[2],std::ios::binary);const auto actor=ReadActor(stream);const auto box=ActorBindBox(actor);Check(box.has_value());
            std::cout<<"{\"box_bits\":[";bool first=true;for(const auto& vector:{box->maximum,box->minimum})for(float value:vector){if(!first)std::cout<<',';first=false;std::cout<<std::bit_cast<std::uint32_t>(value);}
            std::cout<<"],\"box\":[";first=true;std::cout.precision(9);for(const auto& vector:{box->maximum,box->minimum})for(float value:vector){if(!first)std::cout<<',';first=false;std::cout<<value;}std::cout<<"]}\n";return 0;
        }
        if (argc==2) {
            std::ifstream stream(argv[1],std::ios::binary); const auto actor=ReadActor(stream);
            std::vector<Vertex> posed;
            SampleActorVertices(actor,-1,0,posed);
            float bind_residual{};
            for (std::size_t i=0;i<posed.size();++i)
                for (int axis=0;axis<3;++axis)
                    bind_residual=std::max(bind_residual,std::abs(posed[i].position[axis]-actor.mesh.vertices[i].position[axis]));
            float maximum{};
            for (std::size_t clip=0;clip<actor.clips.size();++clip)
                for (float fraction:{0.f,.25f,.5f,.75f,1.f}) {
                    SampleActorVertices(actor,static_cast<int>(clip),fraction*actor.clips[clip].duration,posed);
                    for (std::size_t i=0;i<posed.size();++i)
                        for (int axis=0;axis<3;++axis)
                            maximum=std::max(maximum,std::abs(posed[i].position[axis]-actor.mesh.vertices[i].position[axis]));
                }
            std::cout<<"frames="<<actor.frames.size()<<" palettes="<<actor.palettes.size()<<" clips="<<actor.clips.size()
                     <<" vertices="<<posed.size()<<" bind_residual="<<bind_residual<<" maximum_animated_displacement="<<maximum<<'\n';
            if(const auto sphere=ActorBindSphere(actor))std::cout<<"atomic_spheres="<<actor.atomic_spheres.size()<<" bind_sphere="<<(*sphere)[0]<<','<<(*sphere)[1]<<','<<(*sphere)[2]<<','<<(*sphere)[3]<<'\n';
            return 0;
        }
        {
            // A nonzero exported root must not be added a second time to the
            // game instance pose; genuine child offsets must remain intact.
            Actor resource;const Matrix identity{1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};
            auto root=identity,child=identity;root[13]=4.5f;child[13]=2;
            resource.frames={{-1,root},{0,child}};resource.mesh.vertices.resize(1);
            resource.palettes={{1,identity}};resource.influences={{{0,0,0,0},{1,0,0,0}}};
            resource.mesh.textures={{1,1,{53,31,30,255},false},{1,1,{255,255,255,128},true}};
            auto instance=resource;PrepareActorInstanceRoot(instance);
            PrepareActorTextureFiltering(instance);PrepareActorTextureFiltering(instance);
            for(std::size_t i=0;i<instance.mesh.textures.size();++i){
                Check(instance.mesh.textures[i].filter==TextureFilter::linear);
                Check(resource.mesh.textures[i].filter==TextureFilter::trilinear);
                Check(instance.mesh.textures[i].rgba==resource.mesh.textures[i].rgba);
                Check(instance.mesh.textures[i].alpha==resource.mesh.textures[i].alpha);
            }
            std::vector<Vertex> before,after;std::vector<Matrix> frames;
            SampleActorVertices(resource,-1,0,before);
            SampleActorVertices(instance,-1,0,after,AnimationTimeMode::clamp,&frames);
            Check(before[0].position[1]==6.5f && after[0].position[1]==2);
            Check(frames[1][13]==2 && resource.frames[0].local==root && instance.frames[1].local==child);
            PrepareActorInstanceRoot(instance);Check(instance.frames[0].local==identity);
            bool rejected=false;try{Actor empty;PrepareActorInstanceRoot(empty);}catch(const std::exception&){rejected=true;}
            Check(rejected);
        }
        const auto bytes=Fixture(); auto actor=Parse(bytes); std::vector<Vertex> posed;
        SampleActorVertices(actor,-1,0,posed); Check(std::abs(posed[0].position[0]-1)<1e-5f);
        SampleActorVertices(actor,0,.5f,posed);
        Check(std::abs(posed[0].position[0]-5)<1e-5f && std::abs(posed[0].position[1]+1)<1e-5f);
        SampleActorVertices(actor,0,1.5f,posed); Check(std::abs(posed[0].position[0]-5)<1e-5f);
        // q and -q represent the same rotation; interpolation must take the short arc.
        actor.clips[0].tracks[0].keys[1].rotation={0,0,0,-1};
        SampleActorVertices(actor,0,.5f,posed); Check(std::abs(posed[0].position[0]-4)<1e-5f);
        actor.frames.push_back({0,{1,0,0,0,0,1,0,0,0,0,1,0,3,0,0,1}});
        actor.palettes[0].frame=1;
        std::vector<Matrix> frames;
        SampleActorVertices(actor,-1,0,posed,AnimationTimeMode::clamp,&frames); Check(std::abs(posed[0].position[0]-4)<1e-5f);
        Check(frames.size()==2 && frames[0][12]==2 && frames[1][12]==5);
        SampleActorVertices(actor,0,.5f,posed,AnimationTimeMode::clamp,&frames);
        Check(frames[0][12]==5 && frames[1][12]==8);
        {
            const auto detached=actor.clips[0];auto without_clips=actor;without_clips.clips.clear();
            std::vector<Vertex> independent;std::vector<Matrix> independent_frames;
            SampleActorClipVertices(without_clips,&detached,.5f,independent,AnimationTimeMode::clamp,&independent_frames);
            Check(independent.size()==posed.size() && std::equal(std::begin(independent[0].position),std::end(independent[0].position),std::begin(posed[0].position)) && independent_frames==frames);
            SampleActorClipVertices(without_clips,nullptr,0,independent,AnimationTimeMode::clamp,&independent_frames);
            Check(independent_frames[0][12]==2 && independent_frames[1][12]==5);
            auto bad=detached;bad.duration=0;bool rejected=false;try{SampleActorClipVertices(without_clips,&bad,0,independent);}catch(const std::exception&){rejected=true;}Check(rejected);
            bad=detached;bad.tracks[0].frame=99;rejected=false;try{SampleActorClipVertices(without_clips,&bad,0,independent);}catch(const std::exception&){rejected=true;}Check(rejected);
            unsigned loads=0;bool fail=false;std::weak_ptr<const AnimationClip> weak;
            without_clips.clips.push_back({detached.name,0,{}});
            without_clips.clip_loader=[&](std::string_view){++loads;if(fail)throw std::logic_error("clip read failed");auto value=std::make_shared<AnimationClip>(detached);weak=value;return value;};
            ActorClipLease lease;lease.Sample(without_clips,0,.5f,independent,AnimationTimeMode::clamp,&independent_frames);
            lease.Sample(without_clips,0,.75f,independent);Check(loads==1 && !weak.expired());
            without_clips.clips.push_back({"other",0,{}});fail=true;rejected=false;try{lease.Sample(without_clips,1,0,independent);}catch(const std::exception&){rejected=true;}Check(rejected && loads==2 && !weak.expired());
            fail=false;lease.Sample(without_clips,0,0,independent);Check(loads==2 && !weak.expired());
            lease.Sample(without_clips,-1,0,independent);Check(weak.expired());
            fail=true;rejected=false;try{lease.Sample(without_clips,0,0,independent);}catch(const std::exception&){rejected=true;}Check(rejected && loads==3);
            fail=false;lease.Sample(without_clips,0,0,independent);Check(loads==4 && !weak.expired());lease.Reset();Check(weak.expired());
        }
        Check(!ActorBindSphere(actor));
        actor.atomic_spheres={{1,{0,0,0,1}},{0,{-1,0,0,1}}};
        const auto merged=ActorBindSphere(actor);Check(merged && std::abs((*merged)[0]-1)<1e-5f && std::abs((*merged)[3]-3)<1e-5f);
        actor.atomic_spheres={{0,{0,0,0,10}},{0,{1,0,0,1}}};Check((*ActorBindSphere(actor))[3]==10);
        auto version2=bytes;Patch(version2,4,2u);Add(version2,1u);Add(version2,0u);Add(version2,std::array<float,4>{1,2,3,4});
        const auto restored=Parse(version2);Check(restored.atomic_spheres.size()==1 && (*ActorBindSphere(restored))[3]==4);Check((*ActorBindSphere(restored,true))[0]==3);
        Check(!restored.render_bindings && !Parse(bytes).render_bindings);
        auto version3=version2;Patch(version3,4,3u);
        const auto render_offset=version3.size();
        Add(version3,2u);Add(version3,std::array<std::uint8_t,4>{255,255,255,255});Add(version3,std::array<std::uint8_t,4>{128,64,32,16});
        for(unsigned v:{1u,3u,0u,0u,1u,1u,0u,4u,1u,0u,1u})Add(version3,v);
        const auto mapped=Parse(version3);Check(mapped.render_bindings.has_value());
        const auto& bindings=*mapped.render_bindings;
        Check(bindings.geometry_materials[0]==std::vector<std::uint32_t>({0,0,1}));
        Check(bindings.atomics[0].geometry==0 && bindings.atomics[0].flags==4);
        Check(bindings.draws[0].atomic==0 && bindings.draws[0].material_slot==1);
        Check(bindings.materials[1][3]==16);
        Check(!ActorBindBox(mapped));
        auto version4=version3;Patch(version4,4,4u);const auto bounds_offset=version4.size();
        Add(version4,1u);Add(version4,4u);for(const auto& point:std::array<std::array<float,3>,4>{{{-20,30,40},{2,-3,7},{0,0,0},{1,1,1}}})Add(version4,point);
        const auto original=Parse(version4);const auto direct_box=ActorBindBox(original,true);Check(direct_box && direct_box->maximum[0]==4 && direct_box->minimum[0]==-18);const auto box=ActorBindBox(original);Check(box && box->maximum==std::array<float,3>{2,30,40} && box->minimum==std::array<float,3>{-20,-3,0});
        auto hidden=original;hidden.frames.push_back({0,{.70710677f,.70710677f,0,0,-.70710677f,.70710677f,0,0,0,0,1,0,5,10,0,1}});
        hidden.atomic_spheres.push_back({1,{0,0,0,1}});hidden.render_bindings->atomics.push_back({0,0});(*hidden.bind_vertices)[0]={{{-1,0,0},{1,0,0},{0,1,0}}};
        const auto transformed=ActorBindBox(hidden);Check(transformed && std::abs(transformed->maximum[0]-5.70710677f)<1e-5f && std::abs(transformed->maximum[1]-10.70710677f)<1e-5f && transformed->minimum[0]==-1);
        for(std::size_t length=version3.size();length<version4.size();++length)Reject(version4.substr(0,length));
        for(const auto [offset,value]:std::array<std::pair<std::size_t,unsigned>,3>{{{0,2},{4,3000001},{4,0}}}){auto bad=version4;Patch(bad,bounds_offset+offset,value);Reject(bad);}
        auto invalid_bounds=version4;Patch(invalid_bounds,bounds_offset+8,std::bit_cast<float>(0x7fc00000u));Reject(invalid_bounds);
        for(std::size_t length=version2.size();length<version3.size();++length)Reject(version3.substr(0,length));
        for(const auto [offset,value]:std::array<std::pair<std::size_t,unsigned>,8>{{
            {0,65537},{12,8193},{16,65537},{20,2},{32,2},{36,1},{40,0},{52,3}}}) {
            auto invalid=version3;Patch(invalid,render_offset+offset,value);Reject(invalid);
        }
        auto invalid=version3;Patch(invalid,render_offset+48,1u);Reject(invalid); // absent Atomic
        invalid=version3;invalid+='x';Reject(invalid);
        for(std::size_t length=bytes.size();length<version2.size();++length)Reject(version2.substr(0,length));
        auto corrupt=version2;Patch(corrupt,bytes.size()+4,1u);Reject(corrupt);
        corrupt=version2;Patch(corrupt,version2.size()-4,-1.f);Reject(corrupt);
        for (std::size_t length=0;length<bytes.size();++length) Reject(bytes.substr(0,length));
        auto bad=bytes; bad+='x'; Reject(bad);
        bad=bytes; Patch(bad,12,1025u); Reject(bad);
        bad=bytes; Patch(bad,28+224,0); Reject(bad); // self-parent cycle
        bad=bytes; Patch(bad,28+224+68+68,1u); Reject(bad); // palette reference
        bad=bytes; Patch(bad,28+224+68+68+16,2.f); Reject(bad); // invalid skin sum
        // 原407样本中的0x3f800001应原样保留；无归一化或钳制。
        const auto one_ulp=std::bit_cast<float>(0x3f800001u);
        auto rounded=bytes;Patch(rounded,28+224+68+68+16,one_ulp);
        std::istringstream rounded_input(rounded);const auto rounded_actor=ReadActor(rounded_input);
        Check(std::bit_cast<std::uint32_t>(rounded_actor.influences[0].weights[0])==0x3f800001u);
        bad=bytes;Patch(bad,28+224+68+68+16,1.003f);Reject(bad);
        bad=bytes; Patch(bad,bytes.size()-64,2.f); Reject(bad); // key time out of range
        bad=bytes; Patch(bad,bytes.size()-60,2.f); Reject(bad); // non-unit quaternion
        std::cout<<"actor hierarchy, inverse bind, quaternion interpolation, loop and corruption contracts passed\n";
        return 0;
    } catch (const std::exception& e) { std::cerr<<e.what()<<'\n'; return 1; }
}
