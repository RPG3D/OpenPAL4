#pragma once

#include <cstdint>
#include <cstdio>
#include <string>
#include <string_view>

namespace pal4 {

class IXmlParserNodeRuntime;
class IXmlDocumentRuntime;
class IXmlLeafRuntime;

class IXmlDeclarationRuntime {
public:
    virtual ~IXmlDeclarationRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::string ReadText(
        std::uint32_t address, std::uint32_t length) = 0;
    [[nodiscard]] virtual std::uint32_t Append(
        std::uint32_t output, std::string_view text) = 0;
};

class XmlDeclarationRuntimeStub final : public IXmlDeclarationRuntime {
public:
    std::uint32_t ReadWord(std::uint32_t) override { return 0u; }
    std::string ReadText(std::uint32_t, std::uint32_t) override {
        return {};
    }
    std::uint32_t Append(std::uint32_t, std::string_view) override {
        return 0u;
    }
};

class XmlDeclaration final {
public:
    static constexpr std::uint32_t kVtable = 0x849ae8u;

    // IDA 0x5f15a0 / 0x5f15f0.
    static void Cleanup(std::uint32_t declaration,
                        IXmlParserNodeRuntime& runtime);
    [[nodiscard]] static int Write(
        std::uint32_t declaration, std::FILE* stream, int depth,
        std::uint32_t output, IXmlDeclarationRuntime& runtime);
    // IDA 0x5f17b0 / 0x5f1800 / 0x5f1810.
    [[nodiscard]] static std::uint32_t CloneInto(
        std::uint32_t source, std::uint32_t destination,
        IXmlParserNodeRuntime& runtime);
    [[nodiscard]] static std::uint32_t AcceptVisitor(
        std::uint32_t declaration, std::uint32_t visitor,
        IXmlLeafRuntime& runtime);
    [[nodiscard]] static std::uint32_t Create(
        std::uint32_t source, IXmlDocumentRuntime& runtime);
};

}  // namespace pal4
