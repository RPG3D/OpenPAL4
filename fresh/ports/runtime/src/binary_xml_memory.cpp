#include "pal4/portable/binary_xml_memory.h"
#include "pal4/xml_attribute_record.h"
#include "pal4/xml_float_to_int.h"
#include <algorithm>
#include <bit>
#include <cstdio>
#include <map>
#include <stdexcept>
#include <utility>

namespace pal4::portable {
namespace {
void Require(bool value,const char* message){if(!value)throw std::invalid_argument(message);}
// Bounded scratch address space for one attribute, never a host pointer.
// Strings/arrays use modern owned buffers; original setters choose conversions.
class Value final:public IXmlAttributeRecordRuntime {
    std::map<std::uint32_t,std::vector<std::uint8_t>> blocks_;std::uint32_t next_=0x1000;
    std::uint8_t& Byte(std::uint32_t address){auto it=blocks_.upper_bound(address);Require(it!=blocks_.begin(),"unknown XML value address");--it;Require(address-it->first<it->second.size(),"XML value address out of range");return it->second[address-it->first];}
    [[noreturn]]static void Unsupported(){throw std::logic_error("numeric XML setter requested an unrelated service");}
public:
    std::uint32_t record{},source{};
    explicit Value(const BinaryXmlAttribute& attribute){
        Require(attribute.type<=4,"unsupported XML memory attribute type");record=AllocateMemory(28);source=AllocateMemory(4);WriteWord(record+16,attribute.type);
        switch(attribute.type){
        case 0:Require(std::holds_alternative<std::int8_t>(attribute.value),"XML char type/value mismatch");WriteByte(record+24,std::bit_cast<std::uint8_t>(std::get<std::int8_t>(attribute.value)));break;
        case 1:Require(std::holds_alternative<std::int32_t>(attribute.value),"XML int type/value mismatch");WriteWord(record+24,std::bit_cast<std::uint32_t>(std::get<std::int32_t>(attribute.value)));break;
        case 2:Require(std::holds_alternative<float>(attribute.value),"XML float type/value mismatch");WriteWord(record+24,std::bit_cast<std::uint32_t>(std::get<float>(attribute.value)));break;
        case 3:{
            Require(std::holds_alternative<std::string>(attribute.value),"XML string type/value mismatch");const auto& text=std::get<std::string>(attribute.value);
            Require(text.size()<=1048576,"XML memory text exceeds limit");const auto child=AllocateMemory(16);WriteWord(record+24,child);
            if(!text.empty()){Reallocate(child,static_cast<std::uint32_t>(text.size()));const auto data=ReadWord(child+4);for(std::uint32_t i=0;i<text.size();++i)WriteByte(data+i,static_cast<std::uint8_t>(text[i]));WriteWord(child+8,static_cast<std::uint32_t>(text.size()));}
            break;
        }
        case 4:{
            Require(std::holds_alternative<std::vector<std::uint8_t>>(attribute.value),"XML binary type/value mismatch");const auto& bytes=std::get<std::vector<std::uint8_t>>(attribute.value);
            Require(bytes.size()<=1048576,"XML memory binary exceeds limit");WriteWord(record+20,static_cast<std::uint32_t>(bytes.size()));
            if(!bytes.empty()){const auto data=AllocateRaw(static_cast<std::uint32_t>(bytes.size()));WriteWord(record+24,data);for(std::uint32_t i=0;i<bytes.size();++i)WriteByte(data+i,bytes[i]);}break;
        }
        }
    }
    BinaryXmlAttribute::Value Result(){
        switch(ReadWord(record+16)){
        case 0:return std::bit_cast<std::int8_t>(ReadByte(record+24));
        case 1:return std::bit_cast<std::int32_t>(ReadWord(record+24));
        case 2:return std::bit_cast<float>(ReadWord(record+24));
        case 3:{const auto child=ReadWord(record+24),data=ReadWord(child+4),size=ReadWord(child+8);std::string result;result.reserve(size);for(std::uint32_t i=0;i<size;++i)result.push_back(static_cast<char>(ReadByte(data+i)));return result;}
        default:{const auto data=ReadWord(record+24),size=ReadWord(record+20);std::vector<std::uint8_t> result(size);for(std::uint32_t i=0;i<size;++i)result[i]=ReadByte(data+i);return result;}
        }
    }
    std::uint32_t AllocateMemory(std::uint32_t bytes) override {
        Require(bytes && bytes<=1048578 && next_<0x1000000,"XML scratch allocation limit");const auto token=next_;next_+=((bytes+15)/16)*16+16;blocks_.emplace(token,std::vector<std::uint8_t>(bytes));return token;
    }
    std::uint32_t AllocateRaw(std::uint32_t bytes) override {return AllocateMemory(bytes);}
    std::uint32_t FreeBlock(std::uint32_t token) override {Require(blocks_.erase(token)==1,"unknown XML scratch release");return 1;}
    std::uint32_t FreeRaw(std::uint32_t token) override {return FreeBlock(token);}
    void WriteWord(std::uint32_t address,std::uint32_t value) override {for(unsigned i=0;i<4;++i)Byte(address+i)=static_cast<std::uint8_t>(value>>(i*8));}
    std::uint32_t ReadWord(std::uint32_t address) override {std::uint32_t value{};for(unsigned i=0;i<4;++i)value|=std::uint32_t(Byte(address+i))<<(i*8);return value;}
    std::uint8_t ReadByte(std::uint32_t address) override {return Byte(address);}
    void WriteByte(std::uint32_t address,std::uint8_t value) override {Byte(address)=value;}
    void SetErrorFlag(std::uint32_t child,std::uint8_t flag) override {
        const auto data=ReadWord(child+4);if(data)(void)FreeBlock(data-1);WriteByte(child,flag);WriteWord(child+4,0);WriteWord(child+8,0);WriteWord(child+12,0);
    }
    void Reallocate(std::uint32_t child,std::uint32_t length) override {
        Require(length<=1048576,"XML scratch text exceeds limit");const auto old=ReadWord(child+4);if(old)(void)FreeBlock(old-1);
        const auto data=AllocateMemory(length+2);WriteByte(data,0);WriteWord(child+4,data+1);WriteWord(child+8,0);WriteWord(child+12,length);
    }
    bool AllocateText(std::uint32_t,std::uint32_t,bool) override {Unsupported();}
    void AssignName(std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t) override {Unsupported();}
    std::int32_t FloatToInt(std::uint32_t bits) override {return XmlFloatToInt(bits);}
    std::string FormatFloat(std::uint32_t bits) override {
        char text[64]{};const auto n=std::snprintf(text,sizeof(text),"%f",static_cast<double>(std::bit_cast<float>(bits)));
        Require(n>=0 && static_cast<std::size_t>(n)<sizeof(text),"XML float formatting failed");return text;
    }
    std::uint8_t ScanChar(std::uint32_t,std::uint8_t) override {Unsupported();}
    std::int32_t ScanInt(std::uint32_t,std::int32_t) override {Unsupported();}
    std::uint32_t ScanFloatBits(std::uint32_t,std::uint32_t) override {Unsupported();}
    std::int32_t ParseInt(std::uint32_t) override {Unsupported();}
    std::uint32_t ParseFloatBits(std::uint32_t) override {Unsupported();}
};
void Write(BinaryXmlNode& node,std::string_view key,std::uint32_t bits,bool integer){
    Require(!key.empty() && key.size()<=4096 && key.find('\0')==std::string_view::npos,"invalid XML memory key");
    const auto found=std::find_if(node.attributes.begin(),node.attributes.end(),[&](const auto& a){return a.name==key;});
    BinaryXmlAttribute value=found==node.attributes.end()?BinaryXmlAttribute{std::string(key),integer?1u:2u,integer?BinaryXmlAttribute::Value{std::int32_t{}}:BinaryXmlAttribute::Value{float{}}}:*found;
    Value runtime(value);runtime.WriteWord(runtime.source,bits);
    const auto success=integer?XmlAttributeRecord::SetInt(runtime.record,runtime.source,runtime):XmlAttributeRecord::SetFloat(runtime.record,runtime.source,runtime);
    Require(success!=0,"XML memory setter rejected value");value.value=runtime.Result();
    if(found==node.attributes.end())node.attributes.push_back(std::move(value));else *found=std::move(value);
}
}
void WriteBinaryXmlInt(BinaryXmlNode& node,std::string_view key,std::int32_t value){Write(node,key,std::bit_cast<std::uint32_t>(value),true);}
void WriteBinaryXmlFloat(BinaryXmlNode& node,std::string_view key,float value){Write(node,key,std::bit_cast<std::uint32_t>(value),false);}
}
