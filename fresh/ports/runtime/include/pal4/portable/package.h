#pragma once
#include "pal4/package_archive.h"
#include <istream>
#include <ostream>
#include <optional>
#include <string>
#include <string_view>

namespace pal4::portable {
struct PackageMember {
    std::uint32_t offset{}, packed_bytes{}, decoded_bytes{}, encryption{};
    std::uint16_t codec{};
};
// Borrows a seekable binary stream. File offsets never contain host pointers.
// Read and Find must be serialized by the owner of the stream.
class PackageReader {
public:
    explicit PackageReader(std::istream& stream);
    std::optional<PackageMember> Find(std::string_view name) const;
    std::vector<std::string> ListFiles();
    std::vector<std::uint8_t> Read(std::string_view name, std::size_t limit=64u*1024u*1024u);
    std::optional<std::vector<std::uint8_t>> ReadIfPresent(std::string_view name, std::size_t limit=64u*1024u*1024u);
    // 仅未压缩/未加密成员的有界随机读取；不跨越成员尾部，EOF返回0。
    std::size_t ReadRaw(std::string_view name,std::uint64_t position,std::span<std::uint8_t> output);
    // 未加密原样成员分块复制；其他格式复用现有解码路径。
    std::size_t CopyTo(std::string_view name,std::ostream& output,std::size_t limit=64u*1024u*1024u);
private:
    friend class PackageMemberStream;
    std::vector<std::uint8_t> ReadMember(const PackageMember& member,std::size_t limit);
    std::size_t ReadRawMember(const PackageMember& member,std::uint64_t position,std::span<std::uint8_t> output);
    std::istream& stream_;
    std::uint64_t file_bytes_{};
    PackageArchiveState state_;
    PackageNameCrc crc_;
};
// 原样成员按需读取；压缩或加密成员解码到内存。借用的包和流须保持存活。
class PackageMemberStream {
public:
    PackageMemberStream(PackageReader& package,std::string member,std::size_t memory_limit=512u*1024u*1024u);
    std::size_t Read(std::span<std::uint8_t> output);
    void Seek(std::uint64_t position);
    std::uint64_t Size() const noexcept{return size_;}
    std::uint64_t Position() const noexcept{return position_;}
    bool InMemory() const noexcept{return in_memory_;}
private:
    PackageReader& package_;PackageMember member_{};std::vector<std::uint8_t> data_;
    std::uint64_t size_{},position_{};bool in_memory_{};
};
} // namespace pal4::portable
