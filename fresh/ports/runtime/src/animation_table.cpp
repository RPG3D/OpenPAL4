#include "pal4/portable/animation_table.h"
#include "pal4/animation_script_loader.h"
#include <algorithm>
#include <stdexcept>
namespace pal4::portable {
namespace {
void Require(bool valid){if(!valid)throw std::runtime_error("invalid original animation table");}
class TableReader final:public pal4::IAnimationScriptLoaderRuntime {
public:
    std::string text;std::size_t cursor{};std::vector<std::pair<std::string,std::string>> records;
    explicit TableReader(std::span<const std::uint8_t> bytes):text(bytes.begin(),bytes.end()){Require(bytes.size()<=1024*1024 && text.find('\0')==std::string::npos);}
    pal4::AnimationScriptRecordState& RecordAt(std::uint32_t) override{throw std::logic_error("animation table has no legacy records");}
    std::uint8_t ReadReference(std::uint32_t) override{throw std::logic_error("animation table has no legacy strings");}
    void WriteReference(std::uint32_t,std::uint8_t) override{throw std::logic_error("animation table has no legacy strings");}
    std::uint32_t Free(std::uint32_t) override{throw std::logic_error("animation table storage belongs to host");}
    std::uint32_t Open(std::string_view,std::string_view mode) override{Require(mode=="rt");return 1;}
    bool EndOfFile(std::uint32_t file) override{Require(file==1);return cursor==text.size();}
    std::string ReadLine(std::uint32_t file,std::size_t capacity) override {
        Require(file==1 && capacity==1024);const auto start=cursor;
        while(cursor<text.size() && cursor-start<capacity-1){if(text[cursor++]=='\n')break;}
        return text.substr(start,cursor-start);
    }
    std::size_t EffectiveLength(std::string_view line) override{return line.size();}
    void Insert(pal4::AnimationScriptRecordArrayState& state,std::uint32_t position,std::uint32_t count,std::string_view alias,std::string_view resource) override {
        Require(position==state.end && count==1 && records.size()<4096 && !alias.empty() && resource.size()>4);
        records.emplace_back(alias,resource);state.end+=32;
    }
    void Close(std::uint32_t file) override{Require(file==1);}
};
}
std::vector<std::pair<std::string,std::string>> ReadAnimationTable(std::span<const std::uint8_t> bytes) {
    TableReader reader(bytes);pal4::AnimationScriptRecordArrayState state;
    Require(pal4::AnimationScriptLoader::Load(state,"original.tab",reader)==1);return std::move(reader.records);
}
}
