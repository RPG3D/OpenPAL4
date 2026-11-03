#pragma once

#include <cstdint>
#include <string_view>

namespace pal4 {

class IMinimapCollectionRuntime {
public:
    virtual ~IMinimapCollectionRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    virtual void Free(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t ReadDword(
        std::uint32_t address) = 0;
    virtual void WriteDword(std::uint32_t address,
                            std::uint32_t value) = 0;
    virtual void WriteByte(std::uint32_t address,
                           std::uint8_t value) = 0;
};

class IMinimapOutputRuntime {
public:
    virtual ~IMinimapOutputRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadDword(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t WriteValue(
        std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t WriteBytes(
        std::uint32_t source, std::uint32_t bytes) = 0;
};

class IMinimapInputRuntime : public IMinimapCollectionRuntime {
public:
    [[nodiscard]] virtual std::uint32_t ReadValue() = 0;
    virtual void ReadBytes(std::uint32_t destination,
                           std::uint32_t bytes) = 0;
    virtual void ResizeInner(std::uint32_t element,
                             std::uint32_t count,
                             std::uint32_t fill) = 0;
    virtual void InsertElement(std::uint32_t collection,
                               std::uint32_t element) = 0;
};

class MinimapCollection final {
public:
    static constexpr std::uint32_t kSize = 24u;
    static constexpr std::uint32_t kVtableAddress = 0x841e2cu;
    static constexpr std::uint32_t kBaseVtableAddress = 0x841e40u;
    static constexpr std::uint32_t kLabelAddress = 0x8a595cu;

    [[nodiscard]] static std::uint32_t AllocateAndConstruct(
        std::uint8_t stack_byte, IMinimapCollectionRuntime& runtime);
    [[nodiscard]] static std::uint32_t ConstructAt(
        std::uint32_t address, std::uint8_t stack_byte,
        IMinimapCollectionRuntime& runtime);
    [[nodiscard]] static std::uint32_t CleanupElements(
        std::uint32_t address, IMinimapCollectionRuntime& runtime);
    static void Cleanup(std::uint32_t address,
                        IMinimapCollectionRuntime& runtime);
    static void SetBaseVtable(std::uint32_t address,
                              IMinimapCollectionRuntime& runtime);
    [[nodiscard]] static std::uint32_t DestroyBase(
        std::uint32_t address, std::uint8_t flags,
        IMinimapCollectionRuntime& runtime);
    [[nodiscard]] static std::uint32_t Destroy(
        std::uint32_t address, std::uint8_t flags,
        IMinimapCollectionRuntime& runtime);
    [[nodiscard]] static std::uint32_t Serialize(
        std::uint32_t address, IMinimapOutputRuntime& runtime);
    [[nodiscard]] static std::uint32_t Deserialize(
        std::uint32_t address, std::uint32_t stream_address,
        IMinimapInputRuntime& runtime);
    [[nodiscard]] static constexpr std::uint32_t LabelAddress() {
        return kLabelAddress;
    }
    [[nodiscard]] static constexpr std::string_view Label() {
        return "Minimap";
    }
};

}  // namespace pal4
