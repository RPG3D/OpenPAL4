#define HANDMADE_MATH_NO_SSE
#include <HandmadeMath.h>
#include "pal4/portable/actor.h"
#include <algorithm>
#include <bit>
#include <cmath>
#include <cstring>
#include <istream>
#include <sstream>
#include <stdexcept>

namespace pal4::portable {
namespace {
void Require(bool valid, const char* message) { if (!valid) throw std::runtime_error(message); }
void Read(std::istream& stream, void* target, std::size_t bytes) {
    Require(bool(stream.read(static_cast<char*>(target),static_cast<std::streamsize>(bytes))),"truncated actor resource");
}
template<class T> T Read(std::istream& stream) { T value{}; Read(stream,&value,sizeof value); return value; }
void Finite(float value) { Require(std::isfinite(value),"non-finite actor data"); }
void Validate(const Matrix& matrix) {
    for (float value:matrix) Finite(value);
    Require(matrix[3]==0 && matrix[7]==0 && matrix[11]==0 && matrix[15]==1,"invalid affine actor matrix");
}
Matrix Multiply(const Matrix& a,const Matrix& b) {
    Matrix result{};
    for (int column=0; column<4; ++column)
        for (int row=0; row<4; ++row)
            for (int k=0; k<4; ++k) result[column*4+row]+=a[k*4+row]*b[column*4+k];
    return result;
}
AnimationKey BlendKey(const AnimationKey& a,const AnimationKey& b,float t) {
    auto q=b.rotation;
    float dot{};
    for (int i=0;i<4;++i) dot+=a.rotation[i]*q[i];
    if (dot<0) { for (auto& x:q) x=-x; dot=-dot; }
    float wa=1-t, wb=t;
    if (dot<.9995f) {
        const float angle=std::acos(std::clamp(dot,0.f,1.f));
        wa=std::sin((1-t)*angle)/std::sin(angle); wb=std::sin(t*angle)/std::sin(angle);
    }
    float norm{};
    for (int i=0;i<4;++i) { q[i]=a.rotation[i]*wa+q[i]*wb; norm+=q[i]*q[i]; }
    Require(norm>1e-12f,"degenerate animation quaternion");
    for (auto& x:q) x/=std::sqrt(norm);
    AnimationKey key{};key.rotation=q;
    for(int i=0;i<3;++i)key.translation[i]=a.translation[i]*(1-t)+b.translation[i]*t;
    return key;
}
Matrix KeyMatrix(const AnimationKey& key) {
    const auto [x,y,z,w]=key.rotation;
    Matrix m{1-2*(y*y+z*z),2*(x*y+z*w),2*(x*z-y*w),0,
             2*(x*y-z*w),1-2*(x*x+z*z),2*(y*z+x*w),0,
             2*(x*z+y*w),2*(y*z-x*w),1-2*(x*x+y*y),0,0,0,0,1};
    for (int i=0;i<3;++i) m[12+i]=key.translation[i];
    return m;
}
AnimationKey TrackKey(const AnimationTrack& track,float time) {
    const auto right=std::upper_bound(track.keys.begin(),track.keys.end(),time,[](float t,const AnimationKey& key){return t<key.time;});
    const auto& a=right==track.keys.begin()?track.keys.front():*(right-1);
    const auto& b=right==track.keys.end()?track.keys.back():*right;
    return BlendKey(a,b,b.time>a.time?std::clamp((time-a.time)/(b.time-a.time),0.f,1.f):0.f);
}
void Skin(const Actor& actor,std::vector<Matrix> transforms,std::vector<Vertex>& output,std::vector<Matrix>* frames);
}
Actor ReadActor(std::istream& stream) {
    static_assert(std::endian::native==std::endian::little);
    static_assert(sizeof(AnimationKey)==32 && sizeof(SkinInfluence)==32);
    char magic[4]{}; Read(stream,magic,4);
    const auto version=Read<std::uint32_t>(stream);
    Require(!std::memcmp(magic,"P4AR",4) && (version>=1 && version<=4),"unsupported actor resource");
    const auto scene_bytes=Read<std::uint32_t>(stream), frames=Read<std::uint32_t>(stream),
        palettes=Read<std::uint32_t>(stream), vertices=Read<std::uint32_t>(stream), clips=Read<std::uint32_t>(stream);
    Require(scene_bytes>0 && scene_bytes<=64u*1024*1024 && frames>0 && frames<=1024 &&
            palettes>0 && palettes<=65536 && vertices>0 && vertices<=3000000 && clips<=128,"invalid actor counts");
    std::uint64_t budget=scene_bytes+std::uint64_t(vertices)*sizeof(SkinInfluence)+std::uint64_t(palettes)*68;
    Require(budget<=256ull*1024*1024,"actor memory budget exceeded");
    std::string scene_data(scene_bytes,'\0'); Read(stream,scene_data.data(),scene_data.size());
    std::istringstream scene_stream(scene_data);
    Actor actor; actor.mesh=ReadScene(scene_stream);
    Require(actor.mesh.vertices.size()==vertices,"actor vertex count mismatch");
    actor.frames.resize(frames);
    for (std::uint32_t i=0;i<frames;++i) {
        auto& frame=actor.frames[i]; frame.parent=Read<std::int32_t>(stream);
        Read(stream,frame.local.data(),64); Validate(frame.local);
        Require(frame.parent>=-1 && frame.parent<static_cast<std::int32_t>(i),"actor hierarchy has invalid parent or cycle");
    }
    actor.palettes.resize(palettes);
    for (auto& palette:actor.palettes) {
        palette.frame=Read<std::uint32_t>(stream); Read(stream,palette.inverse_bind.data(),64);
        Validate(palette.inverse_bind); Require(palette.frame<frames,"invalid skin frame");
    }
    actor.influences.resize(vertices);
    Read(stream,actor.influences.data(),actor.influences.size()*sizeof(SkinInfluence));
    for (const auto& influence:actor.influences) {
        float total{};
        for (int i=0;i<4;++i) {
            const float weight=influence.weights[i]; Finite(weight); total+=weight;
            // 原407有比1大一个float单位的权重；不钳制，保持原始蒙皮输入。
            Require(weight>=0 && influence.bones[i]<palettes,"invalid skin influence");
        }
        Require(std::abs(total-1)<=.002f,"invalid total skin weight");
    }
    actor.clips.resize(clips);
    std::vector<std::string> names;
    for (auto& clip:actor.clips) {
        const auto length=Read<std::uint32_t>(stream);
        Require(length>0 && length<=64,"invalid animation name length");
        clip.name.resize(length); Read(stream,clip.name.data(),length);
        Require(clip.name.find_first_not_of("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_-")==std::string::npos,
                "invalid animation name");
        Require(std::find(names.begin(),names.end(),clip.name)==names.end(),"duplicate animation name"); names.push_back(clip.name);
        clip.duration=Read<float>(stream); Finite(clip.duration);
        Require(clip.duration>0,"invalid animation duration");
        const auto tracks=Read<std::uint32_t>(stream); Require(tracks>0 && tracks<=frames,"invalid track count");
        std::vector<bool> assigned(frames); clip.tracks.resize(tracks);
        for (auto& track:clip.tracks) {
            track.frame=Read<std::uint32_t>(stream);
            Require(track.frame<frames && !assigned[track.frame],"invalid or duplicate track frame"); assigned[track.frame]=true;
            const auto count=Read<std::uint32_t>(stream);
            budget+=std::uint64_t(count)*sizeof(AnimationKey);
            Require(count>0 && count<=1000000 && budget<=256ull*1024*1024,"invalid key count or actor budget");
            track.keys.resize(count); Read(stream,track.keys.data(),track.keys.size()*sizeof(AnimationKey));
            float previous=-1;
            for (const auto& key:track.keys) {
                Finite(key.time); float norm{};
                for (float x:key.rotation) { Finite(x); norm+=x*x; }
                for (float x:key.translation) Finite(x);
                Require(std::abs(norm-1)<.01f && key.time>previous && key.time>=0 && key.time<=clip.duration+.001f,"invalid animation key");
                previous=key.time;
            }
            Require(track.keys.front().time==0 && std::abs(track.keys.back().time-clip.duration)<=.001f,"incomplete animation track");
        }
    }
    if(version>=2) {
        const auto count=Read<std::uint32_t>(stream);
        Require(count>0 && count<=8192,"invalid actor bounds count");actor.atomic_spheres.resize(count);
        for(auto& atomic:actor.atomic_spheres) {
            atomic.frame=Read<std::uint32_t>(stream);Read(stream,atomic.sphere.data(),16);
            Require(atomic.frame<frames,"invalid bounds frame");
            for(float v:atomic.sphere)Finite(v);
            Require(atomic.sphere[3]>=0,"negative actor sphere radius");
        }
    }
    if(version>=3) {
        auto& bindings=actor.render_bindings.emplace();
        const auto materials=Read<std::uint32_t>(stream);
        Require(materials<=65536,"invalid actor material count");
        bindings.materials.resize(materials);
        for(auto& color:bindings.materials)Read(stream,color.data(),4);
        const auto geometries=Read<std::uint32_t>(stream);
        Require(geometries>0 && geometries<=8192,"invalid actor geometry count");
        bindings.geometry_materials.resize(geometries);
        for(auto& slots:bindings.geometry_materials) {
            const auto count=Read<std::uint32_t>(stream);
            budget+=std::uint64_t(count)*4;
            Require(count<=65536 && budget<=256ull*1024*1024,"invalid actor material slots or budget");
            slots.resize(count);
            for(auto& material:slots) {
                material=Read<std::uint32_t>(stream);
                Require(material<materials,"invalid actor shared material reference");
            }
        }
        const auto atomics=Read<std::uint32_t>(stream);
        Require(atomics==actor.atomic_spheres.size(),"actor atomic binding count mismatch");
        bindings.atomics.resize(atomics);
        for(auto& atomic:bindings.atomics) {
            atomic.geometry=Read<std::uint32_t>(stream);atomic.flags=Read<std::uint32_t>(stream);
            Require(atomic.geometry<geometries,"invalid actor geometry reference");
        }
        const auto draws=Read<std::uint32_t>(stream);
        Require(draws==actor.mesh.draws.size(),"actor draw binding count mismatch");
        bindings.draws.resize(draws);
        for(auto& draw:bindings.draws) {
            draw.atomic=Read<std::uint32_t>(stream);draw.material_slot=Read<std::uint32_t>(stream);
            Require(draw.atomic<atomics,"invalid actor draw atomic");
            const auto& atomic=bindings.atomics[draw.atomic];
            Require((atomic.flags&4u)!=0 && draw.material_slot<bindings.geometry_materials[atomic.geometry].size(),
                    "invalid actor draw material or nonrendering atomic");
        }
    }
    if(version>=4){
        const auto geometries=Read<std::uint32_t>(stream);Require(actor.render_bindings && geometries==actor.render_bindings->geometry_materials.size(),"actor bounds geometry count mismatch");
        auto& original=actor.bind_vertices.emplace(geometries);std::uint64_t total{};
        for(auto& points:original){const auto count=Read<std::uint32_t>(stream);total+=count;budget+=std::uint64_t(count)*12;
            Require(total<=3000000 && budget<=256ull*1024*1024,"actor bounds vertices exceed budget");points.resize(count);
            for(auto& point:points){Read(stream,point.data(),12);for(float value:point)Finite(value);}
        }
        for(const auto& atomic:actor.render_bindings->atomics)Require(!original[atomic.geometry].empty(),"actor bounds geometry has no vertices");
    }
    Require(stream.peek()==std::char_traits<char>::eof(),"actor resource has trailing bytes");
    return actor;
}
void PrepareActorTextureFiltering(Actor& actor) {
    for(auto& texture:actor.mesh.textures)texture.filter=TextureFilter::linear;
}
void PrepareActorInstanceRoot(Actor& actor) {
    Require(!actor.frames.empty() && actor.frames.front().parent==-1,"actor instance has no clump root");
    actor.frames.front().local={1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};
}
void SampleActorVertices(const Actor& actor,int clip,float seconds,std::vector<Vertex>& output,AnimationTimeMode mode,std::vector<Matrix>* frames) {
    Require(clip>=-1 && (clip==-1 || static_cast<std::size_t>(clip)<actor.clips.size()),"invalid animation selection");
    Require(clip<0 || !actor.clip_loader,"lazy animation requires an owned clip");
    SampleActorClipVertices(actor,clip<0?nullptr:&actor.clips[clip],seconds,output,mode,frames);
}
void ActorClipLease::Reset() noexcept {clip_.reset();owner_=nullptr;index_=-1;}
void ActorClipLease::Sample(const Actor& actor,int clip,float seconds,std::vector<Vertex>& output,AnimationTimeMode mode,std::vector<Matrix>* frames) {
    Require(clip>=-1 && (clip==-1 || static_cast<std::size_t>(clip)<actor.clips.size()),"invalid animation selection");
    if(clip<0 || !actor.clip_loader){SampleActorVertices(actor,clip,seconds,output,mode,frames);Reset();return;}
    auto value=(owner_==&actor && index_==clip)?clip_:actor.clip_loader(actor.clips[clip].name);
    Require(value && value->name==actor.clips[clip].name,"invalid loaded animation");
    SampleActorClipVertices(actor,value.get(),seconds,output,mode,frames);
    clip_=std::move(value);owner_=&actor;index_=clip;
}
void SampleActorClipVertices(const Actor& actor,const AnimationClip* clip,float seconds,std::vector<Vertex>& output,AnimationTimeMode mode,std::vector<Matrix>* frames) {
    Finite(seconds);
    std::vector<Matrix> transforms;
    transforms.reserve(actor.frames.size());
    for (const auto& frame:actor.frames) transforms.push_back(frame.local);
    if (clip) {
        const auto& animation=*clip;Finite(animation.duration);Require(animation.duration>0,"invalid animation duration");
        const float time=mode==AnimationTimeMode::loop?std::fmod(std::max(seconds,0.f),animation.duration):std::clamp(seconds,0.f,animation.duration);
        for (const auto& track:animation.tracks) {
            Require(track.frame<transforms.size() && !track.keys.empty(),"invalid animation track");
            transforms[track.frame]=KeyMatrix(TrackKey(track,time));
        }
    }
    Skin(actor,std::move(transforms),output,frames);
}
void SampleBlendedActorVertices(const Actor& actor,int from,int to,float weight,std::vector<Vertex>& output,std::vector<Matrix>* frames) {
    Require(from>=0 && to>=0 && static_cast<std::size_t>(from)<actor.clips.size() && static_cast<std::size_t>(to)<actor.clips.size(),"invalid animation blend");
    Require(!actor.clip_loader,"lazy animation requires owned clips");
    SampleBlendedActorClipVertices(actor,actor.clips[from],actor.clips[to],weight,output,frames);
}
void SampleBlendedActorClipVertices(const Actor& actor,const AnimationClip& from,const AnimationClip& to,float weight,std::vector<Vertex>& output,std::vector<Matrix>* frames) {
    Finite(weight);Require(weight>=0 && weight<=1,"invalid animation blend");
    // Both temporary RW interpolators are set to their animation's first pose.
    std::vector<AnimationKey> a(actor.frames.size()),b;
    for(std::size_t i=0;i<a.size();++i){
        HMM_Mat4 matrix{};std::memcpy(matrix.Elements,actor.frames[i].local.data(),64);
        const auto q=HMM_M4ToQ_RH(matrix);a[i].rotation={q.X,q.Y,q.Z,q.W};
        for(unsigned axis=0;axis<3;++axis)a[i].translation[axis]=actor.frames[i].local[12+axis];
    }
    b=a;std::vector<bool> animated(a.size());
    for(const auto& track:from.tracks){Require(track.frame<a.size() && !track.keys.empty(),"invalid animation track");a[track.frame]=TrackKey(track,0);animated[track.frame]=true;}
    for(const auto& track:to.tracks){Require(track.frame<b.size() && !track.keys.empty(),"invalid animation track");b[track.frame]=TrackKey(track,0);animated[track.frame]=true;}
    std::vector<Matrix> transforms;transforms.reserve(a.size());
    for(std::size_t i=0;i<a.size();++i)transforms.push_back(animated[i]?KeyMatrix(BlendKey(a[i],b[i],weight)):actor.frames[i].local);
    Skin(actor,std::move(transforms),output,frames);
}
namespace {
void Skin(const Actor& actor,std::vector<Matrix> transforms,std::vector<Vertex>& output,std::vector<Matrix>* frames) {
    for (std::size_t i=0;i<transforms.size();++i)
        if (actor.frames[i].parent>=0) transforms[i]=Multiply(transforms[actor.frames[i].parent],transforms[i]);
    if(frames)*frames=transforms;
    std::vector<Matrix> skin; skin.reserve(actor.palettes.size());
    for (const auto& palette:actor.palettes) skin.push_back(Multiply(transforms[palette.frame],palette.inverse_bind));
    if (output.size()!=actor.mesh.vertices.size()) output=actor.mesh.vertices;
    for (std::size_t i=0;i<output.size();++i) {
        const auto& source=actor.mesh.vertices[i]; const auto& influence=actor.influences[i];
        for (int axis=0;axis<3;++axis) {
            float value{};
            for (int j=0;j<4;++j) {
                if (influence.weights[j]==0) continue;
                const auto& m=skin[influence.bones[j]];
                value+=influence.weights[j]*(m[axis]*source.position[0]+m[4+axis]*source.position[1]+m[8+axis]*source.position[2]+m[12+axis]);
            }
            Finite(value); output[i].position[axis]=value;
        }
    }
}
}
}  // namespace pal4::portable
