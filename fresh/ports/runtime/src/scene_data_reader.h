#pragma once
#include "pal4/binary_xml_config.h"
#include <array>
#include <bit>
#include <cmath>
#include <istream>
#include <stdexcept>

namespace pal4::portable::detail {
inline void Require(bool valid,const char* message) {if(!valid)throw std::runtime_error(message);}
inline void ValidateXml(const BinaryXmlNode& node) {
    for(const auto& attribute:node.attributes) {
        Require(attribute.type<=4,"unsupported scene XML attribute type");
        if(const auto* value=std::get_if<float>(&attribute.value))Require(std::isfinite(*value),"non-finite scene XML value");
    }
    for(const auto& child:node.children)ValidateXml(child);
}
class Reader {
    std::vector<std::uint8_t> bytes_;
    std::size_t offset_{};
public:
    explicit Reader(std::istream& input) {
        char chunk[65536];
        while(input.read(chunk,sizeof chunk) || input.gcount()) {
            Require(bytes_.size()+static_cast<std::size_t>(input.gcount())<=32u*1024*1024,"scene data exceeds limit");
            bytes_.insert(bytes_.end(),chunk,chunk+input.gcount());
        }
        Require(input.eof() && !input.bad(),"cannot read scene data");
    }
    std::size_t Remaining() const {return bytes_.size()-offset_;}
    std::uint32_t Word() {
        Require(Remaining()>=4,"truncated scene data");
        const auto* b=bytes_.data()+offset_;offset_+=4;
        return b[0] | (std::uint32_t(b[1])<<8) | (std::uint32_t(b[2])<<16) | (std::uint32_t(b[3])<<24);
    }
    std::uint8_t Byte() {Require(Remaining()>=1,"truncated scene byte");return bytes_[offset_++];}
    std::uint32_t Peek() {const auto value=Word();offset_-=4;return value;}
    std::uint32_t Count(std::uint32_t limit=10000) {const auto count=Word();Require(count<=limit,"invalid scene record count");return count;}
    std::string Text() {
        const auto length=Word();Require(length>0 && length<=4096 && length<=Remaining(),"invalid scene string length");
        Require(bytes_[offset_+length-1]==0,"unterminated scene string");
        std::string result(reinterpret_cast<const char*>(bytes_.data()+offset_),length-1);offset_+=length;
        Require(result.find('\0')==std::string::npos,"embedded NUL in scene string");return result;
    }
    void Point(std::array<float,3>& point) {for(auto& x:point){x=std::bit_cast<float>(Word());Require(std::isfinite(x),"non-finite scene transform");}}
    BinaryXmlConfig Xml() {
        BinaryXmlConfig result;std::size_t consumed{};
        Require(result.ParsePrefix(std::span<const std::uint8_t>(bytes_).subspan(offset_),consumed) && consumed>0,"invalid scene XML");
        ValidateXml(result.Root());offset_+=consumed;return result;
    }
    std::vector<std::uint8_t> OptionalClump(std::vector<std::uint8_t>& normalized) {
        normalized.clear();
        std::vector<std::pair<std::size_t,std::uint32_t>> patches;
        if(Remaining()<12 || Peek()!=16)return {};
        const auto start=offset_;(void)Word();const auto size=Word();const auto version=Word();
        const auto body=offset_;
        const auto skip=[&](std::uint32_t expected) {
            Require(Word()==expected,"unexpected event clump child");const auto length=Word();(void)Word();
            Require(length<=Remaining(),"truncated event clump child");offset_+=length;return length;
        };
        Require(Word()==1,"missing event clump struct");const auto structure=Word();(void)Word();
        Require(structure>=12 && structure<=Remaining(),"invalid event clump struct");
        const auto atomics=Count();const auto lights=Word(),cameras=Word();
        Require(!lights && !cameras,"unsupported event clump lights/cameras");offset_+=structure-12;
        skip(14);skip(26);
        std::size_t omitted{};
        for(std::uint32_t i=0;i<atomics;++i) {
            Require(Word()==20,"missing event atomic");const auto atomic_size=Word();(void)Word();const auto atomic_body=offset_;
            skip(1);
            Require(Word()==3,"missing event atomic extension");const auto extension_size=Word();const auto extension_version=Word();
            // 此解包资源含四个无载荷游戏插件头，但祖先长度仍计入12+12+8+12。
            // 只接受完整的已观察序列；不做任意字节扫描或固定减44的跳转。
            bool empty_headers=extension_size==92 && version==0x1c020065 && extension_version==version && Remaining()>=48;
            if(empty_headers) {
                const auto saved=offset_;
                for(const auto length:{12u,12u,8u,12u}) {
                    const auto n=static_cast<std::uint32_t>((offset_-saved)/12);
                    const auto id=Word(),declared=Word(),v=Word();
                    if(id!=0x2101u+n || declared!=length || v!=version)empty_headers=false;
                }
                offset_=saved;
            }
            const auto physical_size=empty_headers?48u:extension_size;
            Require(physical_size<=Remaining(),"truncated event atomic extension");offset_+=physical_size;
            const auto missing=empty_headers?44u:0u;
            if(empty_headers) {
                patches.emplace_back(atomic_body-8,atomic_size-missing);
                const auto extension_body=offset_-physical_size;
                patches.emplace_back(extension_body-8,physical_size);
                for(unsigned plugin=0;plugin<4;++plugin)patches.emplace_back(extension_body+plugin*12+4,0);
            }
            Require(offset_-atomic_body+missing==atomic_size,"event atomic size mismatch");omitted+=missing;
        }
        skip(3);
        Require(offset_-body+omitted==size,"event clump size mismatch");
        normalized.assign(bytes_.begin()+start,bytes_.begin()+offset_);
        patches.emplace_back(start+4,size-static_cast<std::uint32_t>(omitted));
        for(const auto& [absolute,value]:patches) {
            Require(absolute>=start && absolute-start+4<=normalized.size(),"invalid event normalization offset");
            for(unsigned i=0;i<4;++i)normalized[absolute-start+i]=static_cast<std::uint8_t>(value>>(i*8));
        }
        return {bytes_.begin()+start,bytes_.begin()+offset_};
    }
    void End() {Require(!Remaining(),"trailing scene data bytes");}
};
}
