#include "pal4/portable/scene_pathways.h"
#include "pal4/pathway_grid.h"
#include <bit>
#include <cmath>
#include <limits>
#include <map>
#include <stdexcept>

namespace pal4::portable {
namespace {
void Require(bool ok,const char* message){if(!ok)throw std::invalid_argument(message);}
void Finite(const std::array<float,3>& p){for(float value:p)Require(std::isfinite(value),"non-finite pathway query");}
class Memory final:public IPathwayGridRuntime {
    std::uint32_t next_=0x1000;
    std::map<std::uint32_t,std::vector<std::uint32_t>> blocks_;
public:
    std::uint32_t Allocate(std::uint32_t bytes) override {
        Require(bytes && bytes%4==0 && bytes<=16u*1024*1024,"invalid pathway grid array size");
        Require(next_<std::numeric_limits<std::uint32_t>::max()-bytes-16,"pathway numeric handle exhausted");
        const auto address=next_;next_+=bytes+16;
        blocks_.emplace(address,std::vector<std::uint32_t>(bytes/4));return address;
    }
    void Free(std::uint32_t address) override {
        Require(blocks_.erase(address)==1,"unknown pathway array release");
    }
    std::uint32_t& Word(std::uint32_t address) {
        auto found=blocks_.upper_bound(address);Require(found!=blocks_.begin(),"unknown pathway numeric address");--found;
        const auto offset=address-found->first;
        Require(offset%4==0 && offset/4<found->second.size(),"pathway array address out of bounds");return found->second[offset/4];
    }
    void WriteWord(std::uint32_t address,std::uint32_t value) override {Word(address)=value;}
    std::uint32_t ReadWord(std::uint32_t address) override {return Word(address);}
};
}
struct ScenePathways::Impl {
    struct Entry {
        ScenePath source;Memory memory;PathwayGridState grid{};
        explicit Entry(const ScenePath& record):source(record) {
            (void)PathwayGrid::Construct(grid,true);
            Require(source.name.size()<4096 && source.name.find('\0')==std::string::npos,"invalid pathway name");
            if(!source.grid_marker){Require(source.points.empty(),"grid-less pathway has points");return;}
            Require(!source.points.empty() && source.points.size()<=100000,"active pathway has invalid point count");
            Require(std::isfinite(std::bit_cast<float>(source.radius_bits)),"non-finite pathway radius");
            std::vector<PathwayPoint> points;points.reserve(source.points.size());
            for(const auto& p:source.points){Finite(p);points.push_back({p});}
            Require(PathwayGrid::Initialize(grid,points,source.radius_bits,source.loop_byte!=0,memory),"cannot initialize pathway grid");
            grid.loop_word=(grid.loop_word&0xffffff00u)|source.loop_byte;
            Require(std::isfinite(std::bit_cast<float>(grid.total_length_bits)),"pathway length overflow");
        }
        ~Entry(){(void)PathwayGrid::Destroy(grid,memory);}
    };
    std::uint64_t generation;
    std::vector<std::unique_ptr<Entry>> paths;
    explicit Impl(std::uint64_t g):generation(g){}
    Entry& Get(ScenePathHandle h) const {
        Require(h.generation==generation && h.index<paths.size(),"stale or invalid pathway handle");return *paths[h.index];
    }
    Entry& Grid(ScenePathHandle h) const {
        auto& e=Get(h);Require(e.source.grid_marker!=0,"pathway has no grid");return e;
    }
};
ScenePathways::ScenePathways(std::uint64_t generation,std::span<const ScenePath> records):impl_(std::make_unique<Impl>(generation)) {
    Require(generation && records.size()<=10000,"invalid pathway owner parameters");
    for(const auto& path:records)impl_->paths.push_back(std::make_unique<Impl::Entry>(path));
}
ScenePathways::~ScenePathways()=default;
std::uint64_t ScenePathways::Generation() const{return impl_->generation;}
std::size_t ScenePathways::Size() const{return impl_->paths.size();}
ScenePathHandle ScenePathways::Handle(std::size_t i) const {Require(i<Size(),"pathway index out of bounds");return {impl_->generation,static_cast<std::uint32_t>(i)};}
std::optional<ScenePathHandle> ScenePathways::Find(std::string_view name) const {
    for(std::size_t i=0;i<Size();++i)if(impl_->paths[i]->source.name==name)return Handle(i);
    return {};
}
const ScenePath& ScenePathways::Source(ScenePathHandle h) const{return impl_->Get(h).source;}
bool ScenePathways::HasGrid(ScenePathHandle h) const{return impl_->Get(h).source.grid_marker!=0;}
std::uint32_t ScenePathways::PointCount(ScenePathHandle h) const{return impl_->Grid(h).grid.point_count;}
std::array<float,3> ScenePathways::Point(ScenePathHandle h,std::size_t i) const {
    auto& e=impl_->Grid(h);Require(i<e.grid.point_count,"pathway point index out of bounds");std::array<float,3> result{};
    for(std::uint32_t axis=0;axis<3;++axis)result[axis]=std::bit_cast<float>(e.memory.ReadWord(e.grid.positions+static_cast<std::uint32_t>(i)*12+axis*4));
    return result;
}
float ScenePathways::Length(ScenePathHandle h) const{return std::bit_cast<float>(impl_->Grid(h).grid.total_length_bits);}
float ScenePathways::ClosestPosition(ScenePathHandle h,const std::array<float,3>& point) {
    Finite(point);auto& e=impl_->Grid(h);return PathwayGrid::FindClosestPosition(e.grid,{point},e.memory);
}
std::array<float,3> ScenePathways::CoordinatesAt(ScenePathHandle h,float distance) {
    Require(std::isfinite(distance),"non-finite pathway distance");auto& e=impl_->Grid(h);
    return PathwayGrid::CoordinatesAt(e.grid,distance,e.memory).coordinates;
}
ScenePathProjection ScenePathways::ProjectToTube(ScenePathHandle h,const std::array<float,3>& point,const std::array<float,3>& initial_direction) {
    Finite(point);Finite(initial_direction);auto& e=impl_->Grid(h);ScenePathProjection result;PathwayPoint direction{initial_direction};
    result.point=PathwayGrid::ProjectToTube(e.grid,{point},direction,result.outside,e.memory).coordinates;
    result.direction=direction.coordinates;return result;
}
}
