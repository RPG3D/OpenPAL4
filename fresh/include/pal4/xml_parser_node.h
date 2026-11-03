#pragma once

#include <cstdint>

namespace pal4 {

class IXmlParserNodeRuntime {
public:
    virtual ~IXmlParserNodeRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    [[nodiscard]] virtual std::uint32_t Free(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t DeleteNode(
        std::uint32_t node, std::uint32_t flag,
        std::uint32_t vtable_offset) = 0;
    [[nodiscard]] virtual std::uint32_t StringLength(
        std::uint32_t text) = 0;
    [[nodiscard]] virtual std::uint32_t AssignString(
        std::uint32_t destination, std::uint32_t source,
        std::uint32_t length) = 0;
    [[nodiscard]] virtual std::int32_t CompareText(
        std::uint32_t left, std::uint32_t right) = 0;
    [[nodiscard]] virtual std::uint32_t CallVirtual(
        std::uint32_t node, std::uint32_t vtable_offset) = 0;
    [[nodiscard]] virtual std::uint32_t AttributeSetup(
        std::uint32_t node, std::uint32_t kind,
        std::uint32_t first, std::uint32_t second,
        std::uint32_t third) = 0;
};

class XmlParserNodeRuntimeStub final : public IXmlParserNodeRuntime {
public:
    std::uint32_t ReadWord(std::uint32_t address) override;
    void WriteWord(std::uint32_t address, std::uint32_t value) override;
    std::uint8_t ReadByte(std::uint32_t address) override;
    void WriteByte(std::uint32_t address, std::uint8_t value) override;
    std::uint32_t Free(std::uint32_t address) override;
    std::uint32_t DeleteNode(std::uint32_t node, std::uint32_t flag,
                             std::uint32_t vtable_offset) override;
    std::uint32_t StringLength(std::uint32_t text) override;
    std::uint32_t AssignString(std::uint32_t destination,
                               std::uint32_t source,
                               std::uint32_t length) override;
    std::int32_t CompareText(std::uint32_t left,
                             std::uint32_t right) override;
    std::uint32_t CallVirtual(std::uint32_t node,
                              std::uint32_t vtable_offset) override;
    std::uint32_t AttributeSetup(std::uint32_t node,
                                 std::uint32_t kind,
                                 std::uint32_t first,
                                 std::uint32_t second,
                                 std::uint32_t third) override;
};

class IXmlVirtualDispatchRuntime {
public:
    virtual ~IXmlVirtualDispatchRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Call(
        std::uint32_t node, std::uint32_t vtable_offset,
        std::uint32_t first, std::uint32_t second,
        std::uint32_t third) = 0;
};

class XmlParserLifecycle final {
public:
    // IDA 0x5f0520 / 0x5f0580 / 0x5f0560.
    [[nodiscard]] static std::uint32_t Initialize(
        std::uint32_t parser, std::uint32_t source,
        IXmlParserNodeRuntime& runtime);
    static void Destroy(std::uint32_t parser,
                        IXmlParserNodeRuntime& runtime);
    [[nodiscard]] static std::uint32_t Delete(
        std::uint32_t parser, std::uint32_t flags,
        IXmlParserNodeRuntime& runtime);
};

class XmlNode final {
public:
    // IDA 0x5f05f0 / 0x5f0620 / 0x5f0650.
    static void Initialize(std::uint32_t node,
                           IXmlParserNodeRuntime& runtime);
    [[nodiscard]] static std::uint32_t SetAttribute(
        std::uint32_t source, std::uint32_t destination,
        IXmlParserNodeRuntime& runtime);
    // IDA 0x5f18a0 thunk.
    [[nodiscard]] static std::uint32_t SetAttributeThunk(
        std::uint32_t source, std::uint32_t destination,
        IXmlParserNodeRuntime& runtime);
    // IDA 0x5f1450.
    [[nodiscard]] static std::uint8_t SetAttributeWithFlag(
        std::uint32_t source, std::uint32_t destination,
        IXmlParserNodeRuntime& runtime);
    [[nodiscard]] static std::uint32_t Cleanup(
        std::uint32_t node, IXmlParserNodeRuntime& runtime);
    // IDA 0x5f0680..0x5f0880.
    [[nodiscard]] static std::uint32_t AddChild(
        std::uint32_t parent, std::uint32_t child,
        IXmlParserNodeRuntime& runtime);
    [[nodiscard]] static std::uint32_t FindChildByName(
        std::uint32_t node, std::uint32_t name,
        IXmlParserNodeRuntime& runtime);
    [[nodiscard]] static std::uint32_t FindFollowingSiblingByName(
        std::uint32_t node, std::uint32_t name,
        IXmlParserNodeRuntime& runtime);
    [[nodiscard]] static std::uint32_t GetFirstChild(
        std::uint32_t node, IXmlParserNodeRuntime& runtime);
    [[nodiscard]] static std::uint32_t GetChildByName(
        std::uint32_t node, std::uint32_t name,
        IXmlParserNodeRuntime& runtime);
    [[nodiscard]] static std::uint32_t GetChildByNameFromNext(
        std::uint32_t node, std::uint32_t name,
        IXmlParserNodeRuntime& runtime);
    [[nodiscard]] static std::uint32_t GetParent(
        std::uint32_t node, IXmlParserNodeRuntime& runtime);
    // IDA 0x5f1090.
    [[nodiscard]] static std::uint32_t GetNextSibling(
        std::uint32_t node, IXmlParserNodeRuntime& runtime);
    // IDA 0x5f1560.
    [[nodiscard]] static std::uint32_t CallVirtual68(
        std::uint32_t node, std::uint32_t first,
        std::uint32_t second, IXmlVirtualDispatchRuntime& runtime);
};

}  // namespace pal4
