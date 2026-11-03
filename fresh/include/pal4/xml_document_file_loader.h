#pragma once

#include <cstdint>

#include "pal4/xml_document_data_reader.h"

namespace pal4 {

class IXmlDocumentFileRuntime {
public:
    virtual ~IXmlDocumentFileRuntime() = default;
    [[nodiscard]] virtual bool ReadLengthWord(
        std::uint32_t stream, std::int32_t& length) = 0;
    virtual void SeekToStart(std::uint32_t stream) = 0;
    [[nodiscard]] virtual std::int32_t GetSize(std::uint32_t stream) = 0;
    virtual void ReadBytes(std::uint32_t stream, std::uint32_t destination,
                           std::uint32_t length) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t size) = 0;
    virtual void Free(std::uint32_t address) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    virtual void ConstructParser() = 0;
    virtual void Parse(std::uint32_t source) = 0;
    [[nodiscard]] virtual bool HasParseError() = 0;
    [[nodiscard]] virtual std::uint32_t FirstParsedChild() = 0;
    virtual void DestroyParser() = 0;
};

class XmlDocumentFileLoader final {
public:
    // IDA 0x5f8110. A nonzero prefixed flag reads a leading length word.
    [[nodiscard]] static std::uint8_t Load(
        std::uint32_t stream, std::uint32_t destination, bool prefixed,
        IXmlDocumentFileRuntime& file, IXmlDocumentDataSource& source,
        IXmlNodeDataRuntime& output);
};

}  // namespace pal4
