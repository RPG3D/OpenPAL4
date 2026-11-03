#pragma once

#include <cstdint>
#include <optional>
#include <string_view>

namespace pal4 {

class IUiFrameRegistryRuntime {
public:
    virtual ~IUiFrameRegistryRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::string_view FrameName(std::uint32_t frame) = 0;
    [[nodiscard]] virtual std::uint32_t SingletonManager() = 0;
    [[nodiscard]] virtual std::uint32_t RegisterFactorySingleton(
        std::uint32_t manager, const char* type_name, std::uint32_t id,
        std::uint32_t constructor, std::uint32_t type_info) = 0;
    [[nodiscard]] virtual std::uint32_t FindFactoryNode(
        std::uint32_t factory, const char* name) = 0;
    [[nodiscard]] virtual std::uint32_t FactoryEndNode(
        std::uint32_t factory) = 0;
    [[nodiscard]] virtual std::uint32_t CreateFrame(
        std::uint32_t node, const char* name) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    virtual void Free(std::uint32_t address) = 0;
};

class IUiFrameFactoryTreeRuntime : public IUiFrameRegistryRuntime {
public:
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t SharedNil() = 0;
    [[nodiscard]] virtual std::uint32_t Predecessor(
        std::uint32_t node) = 0;
    [[nodiscard]] virtual std::uint32_t InsertFactoryNode(
        std::uint32_t tree, std::uint32_t nil, std::uint32_t parent,
        std::uint32_t key, std::uint32_t creator) = 0;
};

class IUiFrameFactoryTreeStorageRuntime {
public:
    virtual ~IUiFrameFactoryTreeStorageRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    virtual void Free(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    [[nodiscard]] virtual std::uint32_t SharedNil() = 0;
    virtual void SetSharedNil(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t SharedNilUsers() = 0;
    virtual void SetSharedNilUsers(std::uint32_t count) = 0;
    virtual void GetFirstElement(std::uint32_t tree) = 0;
    [[nodiscard]] virtual std::uint32_t NextNode(std::uint32_t node) = 0;
    virtual void DeleteNode(std::uint32_t tree, std::uint32_t node) = 0;
    [[nodiscard]] virtual std::uint32_t InvokeDeletingDestructor(
        std::uint32_t object, std::uint32_t flag) = 0;
};

class UiFrameRegistry final {
public:
    // IDA 0x4bed40. The factory guard and cache model globals 0x978af0/0x978af4.
    [[nodiscard]] static std::uint32_t Register(
        std::uint32_t manager, const char* name,
        std::uint8_t& factory_guard, std::uint32_t& factory_singleton,
        IUiFrameRegistryRuntime& runtime);

    // IDA 0x4bef50. Exact byte-length and case-sensitive frame-name match.
    [[nodiscard]] static std::uint32_t Find(
        std::uint32_t manager, const char* name,
        IUiFrameRegistryRuntime& runtime);
};

class UiFrameFactoryTree final {
public:
    // IDA 0x4c18d0. Returns the output slot address; a miss stores the head.
    [[nodiscard]] static std::uint32_t FindNode(
        std::uint32_t tree, std::uint32_t output, std::uint32_t key,
        IUiFrameFactoryTreeRuntime& runtime);

    // IDA 0x4c1940. child is the insertion hint; value points to key/data.
    [[nodiscard]] static std::uint32_t InsertNode(
        std::uint32_t tree, std::uint32_t output, std::uint32_t child,
        std::uint32_t parent, std::uint32_t value,
        IUiFrameFactoryTreeRuntime& runtime);

    // IDA 0x4beff0. Key is the original 32-bit factory-name address.
    [[nodiscard]] static std::uint32_t InsertRegistration(
        std::uint32_t key, std::uint32_t creator,
        std::uint8_t& factory_guard, std::uint32_t& factory_singleton,
        IUiFrameFactoryTreeRuntime& runtime);
};

class UiFrameFactoryTreeStorage final {
public:
    // IDA 0x4c2210. The null branch has no defined return value.
    [[nodiscard]] static std::optional<std::uint32_t> DeleteIfPresent(
        std::uint32_t object, IUiFrameFactoryTreeStorageRuntime& runtime);

    // IDA 0x4c2180. Frees right subtrees before walking left links.
    [[nodiscard]] static std::uint32_t FreeNodes(
        std::uint32_t node, IUiFrameFactoryTreeStorageRuntime& runtime);

    // IDA 0x4c1c10. The two seed bytes originate from one stack byte.
    [[nodiscard]] static std::uint32_t Construct(
        std::uint8_t seed, IUiFrameFactoryTreeStorageRuntime& runtime);

    // IDA 0x4c1ce0. Releases nodes, head and possibly the last shared nil.
    static void Destroy(std::uint32_t tree,
                        IUiFrameFactoryTreeStorageRuntime& runtime);
};

}  // namespace pal4
