#include "pal4/portable/bsp_scene.h"
#include "rw_binary.h"
#include <algorithm>
#include <limits>
#include <map>
#include <utility>

namespace pal4::portable {
namespace {
using namespace rw;
struct Layout {bool normal,color;unsigned uv;};
Layout SectorLayout(std::uint32_t flags,std::uint32_t vertices,std::uint32_t triangles,std::size_t size){
    unsigned uv=(flags>>16)&255;if(!uv)uv=(flags&0x80)?2:((flags&4)?1:0);
    std::vector<Layout> choices{{bool(flags&0x10),bool(flags&8),uv}};
    for(bool n:{true,false})for(bool c:{true,false})for(unsigned u:{2u,1u,0u})choices.push_back({n,c,u});
    for(const auto& choice:choices){const auto length=44ull+std::uint64_t(vertices)*(12+(choice.normal?4:0)+(choice.color?4:0)+8ull*choice.uv)+std::uint64_t(triangles)*8;if(length==size)return choice;}
    throw std::runtime_error("unsupported BSP sector layout");
}
}
Scene ReadBspScene(std::span<const std::uint8_t> bytes,std::span<const std::uint8_t> config,const std::function<Texture(std::string_view)>& load_texture,const std::function<std::optional<Texture>(std::string_view)>& load_lightmap){
    Input in(bytes),settings(config);Need(config.size()==16,"invalid BSP lightmap configuration");Scene result;for(unsigned i=0;i<3;++i)result.saturation[i]=static_cast<float>(settings.Float(i*4));result.brightness=static_cast<float>(settings.Float(12));
    const auto root=in.ReadChunk(0,in.Size());Need(root.id==0x0b && root.end==in.Size(),"invalid BSP world");const auto children=in.Children(root);const auto header=First(children,1);Range(header.begin,64,header.end);const auto expected_triangles=in.U32(header.begin+16),flags=in.U32(header.begin+36);Need(expected_triangles>0 && expected_triangles<=1000000,"invalid BSP triangle count");
    const auto materials=ReadMaterials(in,First(children,8));Need(!materials.empty(),"BSP material list is empty");
    struct Bind {std::int32_t diffuse,light;std::array<std::uint8_t,4> color;std::array<std::string,8> uv_animations;};std::vector<Bind> bindings;std::map<std::string,std::int32_t> textures;std::uint64_t image_bytes=0;
    auto texture_id=[&](const std::string& name,bool light){if(name.empty())return std::int32_t(-1);Need(bool(load_texture) && name.size()<=255 && name.find_first_of("\\/:")==name.npos && name.find("..") == name.npos,"invalid BSP texture name");std::string key=name;for(auto& c:key)if(c>='A' && c<='Z')c=static_cast<char>(c-'A'+'a');if(auto it=textures.find(key);it!=textures.end())return it->second;std::optional<Texture> image;if(light && load_lightmap)image=load_lightmap(name);else image=load_texture(name);if(!image)return std::int32_t(-1);const auto size=std::uint64_t(image->width)*image->height*4;Need(image->width && image->height && image->width<=4096 && image->height<=4096 && image->rgba.size()==size && size<=64ull*1024*1024 && image_bytes+size<=384ull*1024*1024,"invalid BSP texture");image_bytes+=size;Need(result.textures.size()<4096,"too many BSP textures");const auto id=static_cast<std::int32_t>(result.textures.size());result.textures.push_back(std::move(*image));textures.emplace(key,id);return id;};
    for(const auto& m:materials)bindings.push_back({texture_id(m.texture,false),texture_id(m.lightmap,true),m.color,m.uv_animations});
    result.minimum.fill(std::numeric_limits<float>::infinity());result.maximum.fill(-std::numeric_limits<float>::infinity());std::size_t triangle_total=0;
    std::vector<std::pair<Chunk,unsigned>> pending;for(auto it=children.rbegin();it!=children.rend();++it)if(it->id==9 || it->id==10)pending.emplace_back(*it,0);
    while(!pending.empty()){
        const auto [sector,depth]=pending.back();pending.pop_back();Need(depth<=256,"BSP sector tree exceeds depth limit");const auto parts=in.Children(sector);const auto data=First(parts,1);
        if(sector.id==10){for(auto it=parts.rbegin();it!=parts.rend();++it)if(it->id==9 || it->id==10)pending.emplace_back(*it,depth+1);continue;}
        Range(data.begin,44,data.end);const auto base=in.U32(data.begin),triangles=in.U32(data.begin+4),vertices=in.U32(data.begin+8);Need(vertices<=65535 && triangles<=65535 && triangle_total+triangles<=expected_triangles,"invalid BSP sector count");const auto layout=SectorLayout(flags,vertices,triangles,data.end-data.begin);
        auto position=data.begin+44;auto color=position+std::size_t(vertices)*12+(layout.normal?std::size_t(vertices)*4:0);auto uv=color+(layout.color?std::size_t(vertices)*4:0);auto triangle=uv+std::size_t(vertices)*8*layout.uv;
        struct Group {std::uint32_t material;std::vector<std::uint16_t> indices;};std::vector<Group> groups;
        for(unsigned i=0;i<triangles;++i){const auto p=triangle+i*8;const auto a=in.U16(p),b=in.U16(p+2),c=in.U16(p+4),slot=in.U16(p+6);const auto material=std::uint64_t(base)+slot;Need(a<vertices && b<vertices && c<vertices && material<bindings.size(),"invalid BSP triangle reference");auto it=std::find_if(groups.begin(),groups.end(),[&](const auto& g){return g.material==material;});if(it==groups.end()){groups.push_back({static_cast<std::uint32_t>(material),{}});it=groups.end()-1;}it->indices.insert(it->indices.end(),{a,b,c});}
        triangle_total+=triangles;
        for(const auto& group:groups){const auto& material=bindings[group.material];Need(material.light<0 || layout.uv>=2,"lightmapped BSP sector missing UV1");Need(result.draws.size()<100000 && result.vertices.size()+group.indices.size()<=3000000 && image_bytes+(result.vertices.size()+group.indices.size())*sizeof(Vertex)<=384ull*1024*1024,"BSP scene memory budget exceeded");Draw draw{};draw.first=static_cast<std::uint32_t>(result.vertices.size());draw.count=static_cast<std::uint32_t>(group.indices.size());draw.diffuse=material.diffuse;draw.lightmap=material.light;draw.uv_animations=material.uv_animations;for(unsigned c=0;c<4;++c)draw.color[c]=material.color[c]/255.f;
            for(auto index:group.indices){Vertex v{};for(unsigned k=0;k<3;++k){v.position[k]=static_cast<float>(in.Float(position+std::size_t(index)*12+k*4));result.minimum[k]=std::min(result.minimum[k],v.position[k]);result.maximum[k]=std::max(result.maximum[k],v.position[k]);}if(layout.uv)for(unsigned k=0;k<2;++k)v.uv0[k]=std::bit_cast<float>(in.U32(uv+std::size_t(index)*8+k*4));if(layout.uv>=2)for(unsigned k=0;k<2;++k)v.uv1[k]=static_cast<float>(in.Float(uv+std::size_t(vertices)*8+std::size_t(index)*8+k*4));const auto rgba=layout.color?in.U32(color+std::size_t(index)*4):0xffffffffu;for(unsigned k=0;k<4;++k)v.color[k]=float((rgba>>(8*k))&255)/255.f;result.vertices.push_back(v);}
            result.draws.push_back(draw);}
    }
    Need(triangle_total==expected_triangles && !result.vertices.empty(),"BSP triangle count mismatch");return result;
}
}
