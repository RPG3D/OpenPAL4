#include "pal4/portable/scene_data.h"
#include "pal4/world_path_planner.h"
#include "scene_data_reader.h"

namespace pal4::portable {
namespace {
using detail::Reader;
using detail::Require;
// Single original record loader, with checked numeric handles for temporary
// points/grid slots. No private STL layout or host address crosses the boundary.
class PathReader final:public IWorldPathRuntime {
    Reader& input_;ScenePath& record_;
    std::vector<std::array<float,3>> temporary_;
    std::uint32_t header_word_{},grid_{};bool allocated_points_{};
    static constexpr std::uint32_t base=0x100,grid=0x200,points=0x10000;
    static void Stream(std::uint32_t s){Require(s==1,"invalid pathway stream handle");}
    [[noreturn]] static void Unused(){throw std::logic_error("unused pathway record adapter operation");}
    std::pair<std::size_t,std::size_t> PointWord(std::uint32_t address) const {
        Require(address>=points && (address-points)%4==0,"invalid pathway point handle");
        const auto word=(address-points)/4;
        Require(word/3<temporary_.size(),"pathway point handle out of range");return {word/3,word%3};
    }
public:
    PathReader(Reader& in,ScenePath& out):input_(in),record_(out){}
    std::uint32_t ReadWord(std::uint32_t a) override {
        Require(a==base+12,"invalid pathway grid field");return grid_;
    }
    void WriteWord(std::uint32_t a,std::uint32_t v) override {
        if(a==base+12){Require(v==0 || v==grid,"invalid pathway grid handle");grid_=v;return;}
        const auto [i,axis]=PointWord(a);temporary_[i][axis]=std::bit_cast<float>(v);
    }
    std::uint8_t ReadByte(std::uint32_t) override {Unused();}
    void WriteByte(std::uint32_t,std::uint8_t) override {Unused();}
    std::uint32_t Allocate(std::uint32_t size) override {
        if(!allocated_points_){
            Require(size==temporary_.size()*12,"invalid pathway point allocation");
            allocated_points_=true;return size?points:0;
        }
        Require(size==0x58,"invalid pathway grid allocation");return grid;
    }
    std::uint32_t Free(std::uint32_t handle) override {
        Require(handle==0 || handle==points || handle==grid,"invalid pathway release handle");return 0;
    }
    std::uint32_t ReadStreamWord(std::uint32_t stream) override {
        Stream(stream);const auto word=input_.Word();
        switch(header_word_++){
        case 0:record_.grid_marker=word;break;
        case 1:record_.radius_bits=word;Require(std::isfinite(std::bit_cast<float>(word)),"non-finite pathway radius");break;
        case 2:Require(word<=100000 && std::uint64_t(word)*12<=input_.Remaining(),"invalid pathway point count");temporary_.resize(word);break;
        default:Unused();
        }
        return word;
    }
    std::uint8_t ReadStreamByte(std::uint32_t stream) override {
        Stream(stream);Require(header_word_==2,"invalid pathway loop order");record_.loop_byte=input_.Byte();return record_.loop_byte;
    }
    void ReadStreamRecord(std::uint32_t stream,std::uint32_t destination) override {
        Stream(stream);const auto [index,axis]=PointWord(destination);Require(axis==0,"misaligned pathway point record");input_.Point(temporary_[index]);
    }
    std::uint32_t OpenPathwayStream(std::string_view,std::string_view) override {Unused();}
    void ClosePathwayStream(std::uint32_t) override {Unused();}
    std::uint32_t RegisterSingleton(std::string_view,std::uint32_t,std::uint32_t,std::uint32_t) override {Unused();}
    std::uint32_t Call(WorldPathCall operation,std::initializer_list<std::uint32_t> args) override {
        const auto* a=args.begin();
        switch(operation){
        case WorldPathCall::ReadString:
            Require(args.size()==2 && a[0]==1 && a[1]==base,"invalid pathway text field");record_.name=input_.Text();return 0;
        case WorldPathCall::ConstructGrid:
            Require(args.size()==2 && a[0]==grid && a[1]==1,"invalid pathway construction");return grid;
        case WorldPathCall::InitializeGrid:
            Require(args.size()==5 && a[0]==grid && a[1]==temporary_.size() && a[2]==(temporary_.empty()?0:points)
                && a[3]==record_.radius_bits && a[4]==record_.loop_byte,"invalid pathway initialization");
            record_.points=temporary_;return 1;
        case WorldPathCall::DestroyGrid:
            Require(args.size()==2 && a[0]==grid && a[1]==1,"invalid pathway destruction");return 0;
        default:Unused();
        }
    }
    void Read(){(void)PathwayRecord::Load(base,1,*this);}
};
}
std::vector<ScenePath> ReadScenePaths(std::istream& input) {
    Reader r(input);const auto count=r.Count();std::vector<ScenePath> result;result.reserve(count);
    for(std::uint32_t i=0;i<count;++i){ScenePath path;PathReader reader(r,path);reader.Read();result.push_back(std::move(path));}
    r.End();return result;
}
}
