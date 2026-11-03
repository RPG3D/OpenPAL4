#pragma once

#include <cstdint>
#include <string_view>

namespace pal4 {

enum class SoundEvent : std::uint8_t {
    Clicked,
    ClickDenied,
    SelectionChanged,
};

class ISoundPlayerRuntime {
public:
    virtual ~ISoundPlayerRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ImportedBaseVtable() = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    virtual void ConstructTree(std::uint32_t tree) = 0;
    virtual void BeginTemporaryText(std::uint8_t seed) = 0;
    virtual void EndTemporaryText() = 0;
    [[nodiscard]] virtual std::uint32_t CopyCeguiKey(SoundEvent event) = 0;
    virtual void DestroyCeguiKey(std::uint32_t key) = 0;
    [[nodiscard]] virtual std::uint32_t FindOrInsert(
        std::uint32_t tree, std::uint32_t key) = 0;
    [[nodiscard]] virtual std::uint32_t InsertAndResolve(
        std::uint32_t tree, std::uint32_t key) = 0;
    [[nodiscard]] virtual bool AllocateText(
        std::uint32_t record, std::uint32_t length,
        std::uint8_t mode) = 0;
    virtual void CopyText(std::uint32_t destination,
                          std::string_view text) = 0;
    virtual void SetTextSize(std::uint32_t record,
                             std::uint32_t length) = 0;
    [[nodiscard]] virtual std::uint32_t Free(std::uint32_t address) = 0;
    virtual void FreeTreeNodes(std::uint32_t tree,
                               std::uint32_t root) = 0;
    [[nodiscard]] virtual std::uint32_t NextTreeNode(
        std::uint32_t node) = 0;
    virtual void DeleteTreeNode(std::uint32_t tree,
                                std::uint32_t node) = 0;
    virtual void GetFirstTreeNode(std::uint32_t tree) = 0;
    [[nodiscard]] virtual std::uint32_t FindTreeNode(
        std::uint32_t tree, std::uint32_t key) = 0;
    [[nodiscard]] virtual bool CeguiLess(
        std::uint32_t left, std::uint32_t right) = 0;
    [[nodiscard]] virtual std::uint32_t EmptyTextPointer() = 0;
    [[nodiscard]] virtual std::uint8_t PlayMusic(
        std::uint32_t path, std::uint32_t repeat) = 0;
    [[nodiscard]] virtual std::uint32_t InsertTreeNode(
        std::uint32_t tree, std::uint32_t child,
        std::uint32_t parent, std::uint32_t key) = 0;
    [[nodiscard]] virtual std::uint32_t PreviousTreeNode(
        std::uint32_t node) = 0;
    virtual void GrowCeguiString(std::uint32_t string,
                                  std::uint32_t length) = 0;
    virtual void SetCeguiStringLength(std::uint32_t string,
                                      std::uint32_t length) = 0;
    virtual void CopyMemory(std::uint32_t destination,
                             std::uint32_t source,
                             std::uint32_t bytes) = 0;
    [[nodiscard]] virtual std::uint32_t AssignTextRecord(
        std::uint32_t destination, std::uint32_t source,
        std::uint32_t start, std::uint32_t count) = 0;
};

class SoundPlayer final {
public:
    static constexpr std::uint32_t kVtable = 0x844490u;

    // IDA 0x4c3340. seed represents the copied stack byte.
    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t object, std::uint8_t seed,
        ISoundPlayerRuntime& runtime);
    // IDA 0x4c3620: destroy the embedded tree at object +4.
    [[nodiscard]] static std::uint32_t DestroyTree(
        std::uint32_t tree, ISoundPlayerRuntime& runtime);
    // IDA 0x4c36e0: return the observed low EAX byte.
    [[nodiscard]] static std::uint8_t FindAndPlayMusic(
        std::uint32_t object, std::uint32_t key,
        ISoundPlayerRuntime& runtime);
    // IDA 0x4c3750. Writes node and inserted byte into output.
    [[nodiscard]] static std::uint32_t FindOrInsertNode(
        std::uint32_t tree, std::uint32_t output,
        std::uint32_t key, ISoundPlayerRuntime& runtime);

private:
    [[nodiscard]] static std::uint32_t ResolveNode(
        std::uint32_t tree, std::uint8_t seed,
        SoundEvent event, bool insert_variant,
        ISoundPlayerRuntime& runtime);
    static void AssignFile(std::uint32_t node, std::string_view path,
                           bool use_size_helper,
                           ISoundPlayerRuntime& runtime);
};

class SoundEventKeyRecord final {
public:
    // IDA 0x4c3840. The record occupies 168 bytes.
    [[nodiscard]] static std::uint32_t CopyWithProperties(
        std::uint32_t destination, std::uint32_t source_cegui,
        std::uint32_t source_text,
        ISoundPlayerRuntime& runtime);
    // IDA 0x4c40a0. Returns the StringBuffer_Assign result.
    [[nodiscard]] static std::uint32_t CopyFromRecord(
        std::uint32_t destination, std::uint32_t source,
        ISoundPlayerRuntime& runtime);
};

}  // namespace pal4
