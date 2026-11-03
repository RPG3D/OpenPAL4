#include "pal4/portable/package.h"
#include "pal4/package_archive_index.h"
#include "pal4/package_tea.h"
#include <minilzo.h>
#include <algorithm>
#include <stdexcept>

namespace pal4::portable {
namespace {
void ReadAt(std::istream& input,std::uint64_t offset,std::span<std::uint8_t> output) {
    input.clear();input.seekg(static_cast<std::streamoff>(offset));
    if(!input || !input.read(reinterpret_cast<char*>(output.data()),static_cast<std::streamsize>(output.size())))
        throw std::runtime_error("truncated package read");
}
std::uint32_t Word(std::span<const std::uint8_t> bytes,std::size_t offset) {
    return std::uint32_t(bytes[offset]) | (std::uint32_t(bytes[offset+1])<<8) |
        (std::uint32_t(bytes[offset+2])<<16) | (std::uint32_t(bytes[offset+3])<<24);
}
}
PackageReader::PackageReader(std::istream& stream):stream_(stream) {
    stream_.clear();stream_.seekg(0,std::ios::end);
    const auto end=stream_.tellg();
    if(end<128)throw std::runtime_error("truncated package header");
    file_bytes_=static_cast<std::uint64_t>(end);
    std::array<std::uint8_t,128> header{};ReadAt(stream_,0,header);
    if(Word(header,0)!=0x1a545352u)throw std::runtime_error("invalid RST package magic");
    const auto capacity=Word(header,16),count=Word(header,32);
    if(capacity>32768 || count>capacity)throw std::runtime_error("invalid package table count");
    const auto bytes=std::size_t(capacity)*32;
    if(bytes>file_bytes_-128)throw std::runtime_error("truncated package table");
    // Original Package_Open decrypts the full zero-filled 1 MiB table,
    // even when fewer table records are stored in the file (IDA 0x793747).
    std::vector<std::uint8_t> table(0x100000);
    ReadAt(stream_,128,std::span(table).first(bytes));
    (void)PackageTea::DecryptArchiveTable(table);
    for(std::uint32_t i=0;i<header.size();++i)state_.WriteByte(8+i,header[i]);
    for(std::uint32_t i=0;i<table.size();++i)state_.WriteByte(136+i,table[i]);
    crc_.InitializeTable();
    std::uint32_t previous=0;
    for(std::uint32_t i=0;i<count;++i) {
        const auto hash=state_.ReadWord(136+i*32);
        if(i && hash<previous)throw std::runtime_error("unsorted package index");
        previous=hash;
    }
}
std::optional<PackageMember> PackageReader::Find(std::string_view name) const {
    if(name.find('\0')!=std::string_view::npos)throw std::invalid_argument("NUL in package member");
    const auto index=PackageArchiveIndex::FindByName(state_,std::string(name).c_str(),crc_);
    if(index<0)return {};
    const auto record=136+static_cast<std::uint32_t>(index)*32;
    if(state_.ReadWord(record+4)&2)throw std::runtime_error("package member is a directory");
    PackageMember result{state_.ReadWord(record+12),state_.ReadWord(record+16),
        state_.ReadWord(record+20),state_.ReadWord(record+28),
        static_cast<std::uint16_t>(state_.ReadWord(record+4)>>16)};
    if(result.offset<128+std::uint64_t(state_.ReadWord(24))*32 || result.offset>file_bytes_ ||
       result.packed_bytes>file_bytes_-result.offset)throw std::runtime_error("package member range outside file");
    return result;
}
std::vector<std::string> PackageReader::ListFiles() {
    std::vector<std::string> result;
    for(std::uint32_t i=0;i<state_.ReadWord(40);++i) {
        const auto r=136+i*32,flags=state_.ReadWord(r+4);
        if(!(flags&1) || (flags&0x12))continue;
        const auto offset=std::uint64_t(state_.ReadWord(r+12))+state_.ReadWord(r+16);
        const auto size=state_.ReadWord(r+24);
        if(!size || size>=324 || offset>file_bytes_ || size>file_bytes_-offset)
            throw std::runtime_error("invalid package filename range");
        std::vector<std::uint8_t> bytes(size);ReadAt(stream_,offset,bytes);
        const auto end=std::find(bytes.begin(),bytes.end(),0);
        result.emplace_back(bytes.begin(),end);
    }
    return result;
}
std::vector<std::uint8_t> PackageReader::Read(std::string_view name,std::size_t limit) {
    const auto member=Find(name);
    if(!member)throw std::runtime_error("package member missing: "+std::string(name));
    return ReadMember(*member,limit);
}
std::optional<std::vector<std::uint8_t>> PackageReader::ReadIfPresent(std::string_view name,std::size_t limit) {
    const auto member=Find(name);
    if(!member)return {};
    return ReadMember(*member,limit);
}
std::vector<std::uint8_t> PackageReader::ReadMember(const PackageMember& member,std::size_t limit) {
    if(member.decoded_bytes>limit)throw std::length_error("package member exceeds read limit");
    if(member.codec!=1 && member.codec!=2)throw std::runtime_error("unsupported package codec");
    std::vector<std::uint8_t> data(member.decoded_bytes);
    if(member.codec==1) {
        if(member.decoded_bytes>member.packed_bytes)throw std::runtime_error("invalid raw package member size");
        ReadAt(stream_,member.offset,data);
    } else {
        if(member.packed_bytes>limit)throw std::length_error("packed member exceeds read limit");
        std::vector<std::uint8_t> packed(member.packed_bytes);ReadAt(stream_,member.offset,packed);
        lzo_uint size=static_cast<lzo_uint>(data.size());
        if(lzo1x_decompress_safe(packed.data(),static_cast<lzo_uint>(packed.size()),data.data(),&size,nullptr)!=LZO_E_OK || size!=data.size())
            throw std::runtime_error("invalid LZO package member");
    }
    if(member.encryption) {
        if(PackageTea::DecryptBytes(data,PackageTea::PackageKeyWords())!=0)
            throw std::runtime_error("invalid encrypted package member");
    }
    return data;
}
std::size_t PackageReader::ReadRaw(std::string_view name,std::uint64_t position,std::span<std::uint8_t> output){
    const auto member=Find(name);if(!member)throw std::runtime_error("package member missing: "+std::string(name));
    return ReadRawMember(*member,position,output);
}
std::size_t PackageReader::ReadRawMember(const PackageMember& member,std::uint64_t position,std::span<std::uint8_t> output){
    if(member.codec!=1 || member.encryption)throw std::runtime_error("package member is not raw plaintext");
    if(member.decoded_bytes>member.packed_bytes || position>member.decoded_bytes)throw std::runtime_error("invalid raw member read range");
    const auto count=std::min(output.size(),std::size_t(member.decoded_bytes-position));
    if(count){ReadAt(stream_,std::uint64_t(member.offset)+position,output.first(count));}
    return count;
}
std::size_t PackageReader::CopyTo(std::string_view name,std::ostream& output,std::size_t limit){
    const auto member=Find(name);
    if(!member)throw std::runtime_error("package member missing: "+std::string(name));
    if(member->decoded_bytes>limit)throw std::length_error("package member exceeds copy limit");
    if(member->codec==1 && !member->encryption){
        if(member->decoded_bytes>member->packed_bytes)throw std::runtime_error("invalid raw package member size");
        std::array<std::uint8_t,65536> buffer{};std::size_t copied=0;
        while(copied<member->decoded_bytes){
            const auto count=std::min(buffer.size(),std::size_t(member->decoded_bytes)-copied);
            ReadAt(stream_,std::uint64_t(member->offset)+copied,std::span(buffer).first(count));
            output.write(reinterpret_cast<const char*>(buffer.data()),static_cast<std::streamsize>(count));
            if(!output)throw std::runtime_error("package copy output failed");
            copied+=count;
        }
    }else{
        const auto data=Read(name,limit);
        output.write(reinterpret_cast<const char*>(data.data()),static_cast<std::streamsize>(data.size()));
        if(!output)throw std::runtime_error("package copy output failed");
    }
    return member->decoded_bytes;
}
PackageMemberStream::PackageMemberStream(PackageReader& package,std::string member,std::size_t memory_limit):package_(package){
    const auto info=package_.Find(member);if(!info)throw std::runtime_error("package member missing: "+member);
    member_=*info;
    size_=info->decoded_bytes;in_memory_=info->codec!=1 || info->encryption;
    if(in_memory_)data_=package_.Read(member,memory_limit);
    else if(info->decoded_bytes>info->packed_bytes)throw std::runtime_error("invalid raw package member size");
}
std::size_t PackageMemberStream::Read(std::span<std::uint8_t> output){
    const auto count=std::min(output.size(),static_cast<std::size_t>(size_-position_));
    if(in_memory_)std::copy_n(data_.begin()+static_cast<std::size_t>(position_),count,output.begin());
    else if(count)package_.ReadRawMember(member_,position_,output.first(count));
    position_+=count;return count;
}
void PackageMemberStream::Seek(std::uint64_t position){if(position>size_)throw std::out_of_range("package stream seek outside member");position_=position;}
} // namespace pal4::portable
