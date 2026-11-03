#include "pal4/portable/original_animation.h"
#include "rw_binary.h"
#include <algorithm>
#include <map>
#include <set>
namespace pal4::portable {
namespace {using namespace rw;}
std::vector<std::uint32_t> ReadDffAnimationHierarchy(std::span<const std::uint8_t> bytes){
    Input in(bytes);const auto root=in.ReadChunk(0,in.Size());Need(root.id==0x10 && root.end==in.Size(),"invalid animation DFF clump");const auto frame_list=First(in.Children(root),0x0e);const auto children=in.Children(frame_list);const auto header=First(children,1);Range(header.begin,4,header.end);const auto count=in.U32(header.begin);Need(count && count<=1024,"invalid animation frame count");Range(header.begin+4,std::size_t(count)*56,header.end);
    std::map<std::uint32_t,std::uint32_t> frame_by_id;std::vector<Chunk> hierarchies;
    for(unsigned frame=0;frame<count;++frame){if(frame+1>=children.size() || children[frame+1].id!=3)continue;for(const auto& plugin:in.Children(children[frame+1]))if(plugin.id==0x11e && plugin.end-plugin.begin>=8){frame_by_id[in.U32(plugin.begin+4)]=frame;if(plugin.end-plugin.begin>=20 && in.U32(plugin.begin+8))hierarchies.push_back(plugin);}}
    if(hierarchies.empty())return {};
    const auto hierarchy=hierarchies.front();const auto nodes=in.U32(hierarchy.begin+8);Need(nodes<=count && nodes<=1024,"invalid DFF hierarchy node count");Range(hierarchy.begin+20,std::size_t(nodes)*12,hierarchy.end);std::vector<std::uint32_t> result;std::set<std::uint32_t> used;
    for(unsigned i=0;i<nodes;++i){const auto id=in.U32(hierarchy.begin+20+i*12);const auto found=frame_by_id.find(id);Need(found!=frame_by_id.end() && used.insert(found->second).second,"missing or duplicate DFF hierarchy frame");result.push_back(found->second);}return result;
}
AnimationClip ReadOriginalAnimation(std::span<const std::uint8_t> bytes,std::string_view name,std::span<const std::uint32_t> hierarchy){
    Need(!name.empty() && name.size()<=64 && name.find_first_not_of("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_-")==name.npos,"invalid animation name");
    Input in(bytes);const auto root=in.ReadChunk(0,in.Size());Need(root.id==0x1b,"invalid ANM chunk");Range(root.begin,20,root.end);const auto type=in.U32(root.begin),count=in.U32(root.begin+8);const auto duration=static_cast<float>(in.Float(root.begin+16));Need(type==256 && count>0 && count<=2000000 && duration>0,"unsupported ANM header");Range(root.begin+20,std::size_t(count)*36,root.end);
    AnimationClip clip;clip.name=std::string(name);clip.duration=duration;std::vector<std::size_t> track_ids;track_ids.reserve(count);
    for(unsigned i=0;i<count;++i){const auto p=root.begin+20+std::size_t(i)*36;const auto previous=std::bit_cast<std::int32_t>(in.U32(p+32));std::size_t track=clip.tracks.size();if(previous>=0 && previous%36==0 && std::size_t(previous/36)<track_ids.size())track=track_ids[previous/36];if(track==clip.tracks.size())clip.tracks.push_back({});track_ids.push_back(track);
        AnimationKey key{};key.time=static_cast<float>(in.Float(p));for(unsigned j=0;j<4;++j)key.rotation[j]=static_cast<float>(in.Float(p+4+j*4));for(unsigned j=0;j<3;++j)key.translation[j]=static_cast<float>(in.Float(p+20+j*4));clip.tracks[track].keys.push_back(key);}
    Need(clip.tracks.size()==hierarchy.size() && !hierarchy.empty(),"ANM and DFF hierarchy mismatch");
    for(std::size_t i=0;i<clip.tracks.size();++i){auto& track=clip.tracks[i];track.frame=hierarchy[i];std::stable_sort(track.keys.begin(),track.keys.end(),[](const auto& a,const auto& b){return a.time<b.time;});Need(!track.keys.empty() && track.keys.front().time==0 && std::abs(track.keys.back().time-duration)<=.001f,"incomplete ANM track");float previous=-1;
        for(const auto& key:track.keys){float norm=0;for(float q:key.rotation)norm+=q*q;Need(key.time>previous && key.time>=0 && key.time<=duration+.001f && std::abs(norm-1)<.01f,"invalid ANM key");previous=key.time;}}
    return clip;
}
OriginalAnimationStore::OriginalAnimationStore(std::vector<std::uint32_t> hierarchy,std::vector<std::string> names,
    std::function<std::vector<std::uint8_t>(std::string_view)> read):hierarchy_(std::move(hierarchy)),read_(std::move(read)) {
    Need(!hierarchy_.empty() && hierarchy_.size()<=1024 && bool(read_) && names.size()<=128,"invalid original animation store");
    for(auto& name:names){
        Need(!name.empty() && name.size()<=64 && name.find_first_not_of("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_-")==name.npos,"invalid animation name");
        Need(clips_.emplace(std::move(name),std::weak_ptr<const AnimationClip>{}).second,"duplicate animation name");
    }
}
std::shared_ptr<const AnimationClip> OriginalAnimationStore::Get(std::string_view name){
    const auto found=clips_.find(name);Need(found!=clips_.end(),"unknown original animation");
    if(auto clip=found->second.lock())return clip;
    const auto bytes=read_(found->first);
    auto clip=std::make_shared<const AnimationClip>(ReadOriginalAnimation(bytes,found->first,hierarchy_));
    found->second=clip;return clip;
}
void LoadOriginalAnimations(Actor& actor,std::span<const std::uint8_t> dff,const std::function<std::vector<std::uint8_t>(std::string_view)>& read){
    if(actor.clips.empty())return;
    Need(bool(read),"original ANM reader missing");const auto hierarchy=ReadDffAnimationHierarchy(dff);std::vector<AnimationClip> clips;clips.reserve(actor.clips.size());std::uint64_t key_bytes=0;
    for(const auto& previous:actor.clips){Need(previous.tracks.size()==hierarchy.size(),"actor and DFF hierarchy mismatch");for(std::size_t i=0;i<hierarchy.size();++i)Need(hierarchy[i]<actor.frames.size() && previous.tracks[i].frame==hierarchy[i],"actor and DFF frame mismatch");const auto bytes=read(previous.name);auto clip=ReadOriginalAnimation(bytes,previous.name,hierarchy);for(const auto& track:clip.tracks){key_bytes+=std::uint64_t(track.keys.size())*sizeof(AnimationKey);Need(key_bytes<=256ull*1024*1024,"original ANM memory budget exceeded");}clips.push_back(std::move(clip));}
    actor.clips=std::move(clips);
}
}
