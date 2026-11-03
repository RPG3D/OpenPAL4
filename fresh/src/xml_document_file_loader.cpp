#include "pal4/xml_document_file_loader.h"

#include <limits>
#include <new>
#include <stdexcept>

namespace pal4 {
namespace {

class ParserScope final {
public:
    explicit ParserScope(IXmlDocumentFileRuntime& runtime) : runtime_(runtime) {
        runtime_.ConstructParser();
    }
    ~ParserScope() { runtime_.DestroyParser(); }
    ParserScope(const ParserScope&) = delete;
    ParserScope& operator=(const ParserScope&) = delete;

private:
    IXmlDocumentFileRuntime& runtime_;
};

class BufferScope final {
public:
    BufferScope(IXmlDocumentFileRuntime& runtime, const std::uint32_t size)
        : runtime_(runtime), address_(runtime.Allocate(size)) {
        if (address_ == 0u) throw std::bad_alloc{};
    }
    ~BufferScope() { Release(); }
    BufferScope(const BufferScope&) = delete;
    BufferScope& operator=(const BufferScope&) = delete;
    [[nodiscard]] std::uint32_t Address() const { return address_; }
    void Release() {
        if (address_ != 0u) runtime_.Free(address_);
        address_ = 0u;
    }

private:
    IXmlDocumentFileRuntime& runtime_;
    std::uint32_t address_;
};

}  // namespace

std::uint8_t XmlDocumentFileLoader::Load(
    const std::uint32_t stream, const std::uint32_t destination,
    const bool prefixed, IXmlDocumentFileRuntime& file,
    IXmlDocumentDataSource& source, IXmlNodeDataRuntime& output) {
    if (stream == 0u || destination == 0u) return 0u;

    std::int32_t length = 0;
    if (prefixed) {
        if (!file.ReadLengthWord(stream, length)) return 0u;
    } else {
        file.SeekToStart(stream);
        length = file.GetSize(stream);
    }
    if (length < 0 || length == std::numeric_limits<std::int32_t>::max())
        throw std::length_error("invalid XML document length");

    BufferScope buffer(file, static_cast<std::uint32_t>(length) + 1u);
    file.ReadBytes(stream, buffer.Address(), static_cast<std::uint32_t>(length));
    file.WriteByte(buffer.Address() + static_cast<std::uint32_t>(length), 0u);
    ParserScope parser(file);
    file.Parse(buffer.Address());
    buffer.Release();
    if (file.HasParseError()) return 0u;
    return XmlDocumentDataReader::Read(
        0u, destination, file.FirstParsedChild(), source, output);
}

}  // namespace pal4
