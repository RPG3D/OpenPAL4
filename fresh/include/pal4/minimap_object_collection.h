#pragma once

#include <array>
#include <cstdint>
#include <string_view>

namespace pal4 {

class MinimapIconState;

class MinimapObjectCollectionState final {
public:
    static constexpr std::uint32_t kBytes = 20u;
    [[nodiscard]] std::array<std::uint8_t, kBytes>& Bytes() noexcept {
        return bytes_;
    }
    [[nodiscard]] const std::array<std::uint8_t, kBytes>& Bytes()
        const noexcept { return bytes_; }
    [[nodiscard]] std::uint32_t ReadDword(std::uint32_t offset) const;
    void WriteDword(std::uint32_t offset, std::uint32_t value);

private:
    std::array<std::uint8_t, kBytes> bytes_{};
};

class IMinimapObjectCollectionRuntime {
public:
    virtual ~IMinimapObjectCollectionRuntime() = default;
    [[nodiscard]] virtual MinimapObjectCollectionState& Access(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t ReadDword(
        std::uint32_t address) = 0;
    virtual void WriteDword(std::uint32_t address,
                            std::uint32_t value) = 0;
    virtual void FreeRecordBuffer(std::uint32_t address) = 0;
    virtual void FreeRecordArray(std::uint32_t address) = 0;
};

struct MinimapObjectGroupRecord {
    std::uint32_t id_pointer = 0u;
    std::uint8_t name_pointer_low_byte = 0u;
    std::uint32_t begin = 0u;
    std::uint32_t end = 0u;
    std::uint32_t capacity = 0u;
};

class IMinimapObjectAddRuntime {
public:
    virtual ~IMinimapObjectAddRuntime() = default;
    [[nodiscard]] virtual std::uint32_t FindGroup(
        std::uint32_t collection, std::uint32_t id_pointer) = 0;
    [[nodiscard]] virtual std::int32_t CollectionSize(
        const MinimapObjectGroupRecord& record) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(
        std::uint32_t bytes) = 0;
    [[nodiscard]] virtual std::uint32_t CopyDwordArray(
        std::uint32_t begin, std::uint32_t end,
        std::uint32_t destination) = 0;
    virtual void InsertGroup(std::uint32_t collection,
                             std::uint32_t insertion_address,
                             const MinimapObjectGroupRecord& record) = 0;
    virtual void Free(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t ReadDword(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual bool FindIconByName(
        std::uint32_t group_collection,
        std::string_view name) = 0;
    [[nodiscard]] virtual std::uint32_t ConstructIcon(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual bool LoadIconTexture(
        std::uint32_t icon, std::string_view filename,
        std::uint32_t id_pointer) = 0;
    virtual void DestroyIcon(std::uint32_t icon) = 0;
    virtual void SetIconName(std::uint32_t icon_string,
                             std::string_view name) = 0;
    virtual void SetIconCenter(std::uint32_t icon,
                               std::int32_t x, std::int32_t y) = 0;
    virtual void InsertIcon(std::uint32_t group_collection,
                            std::uint32_t insertion_address,
                            std::uint32_t icon) = 0;
};

class IMinimapObjectRemoveRuntime {
public:
    virtual ~IMinimapObjectRemoveRuntime() = default;
    [[nodiscard]] virtual std::uint32_t FindGroup(
        std::uint32_t collection, std::uint32_t id_pointer) = 0;
    [[nodiscard]] virtual std::uint32_t ReadDword(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual bool IconNameEquals(
        std::uint32_t icon, std::string_view name) = 0;
    virtual void DestroyIcon(std::uint32_t icon) = 0;
    virtual void WriteDword(std::uint32_t address,
                            std::uint32_t value) = 0;
};

class IMinimapObjectClearRuntime {
public:
    virtual ~IMinimapObjectClearRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadDword(
        std::uint32_t address) = 0;
    virtual void DestroyIcon(std::uint32_t icon) = 0;
    [[nodiscard]] virtual int CleanupGroup(std::uint32_t group) = 0;
};

class IMinimapObjectSectionRuntime : public IMinimapObjectClearRuntime {
public:
    [[nodiscard]] virtual std::uint32_t FindGroup(
        std::uint32_t collection, std::uint32_t id_pointer) = 0;
    virtual void WriteDword(std::uint32_t address,
                            std::uint32_t value) = 0;
};

class IMinimapObjectDrawRuntime {
public:
    virtual ~IMinimapObjectDrawRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadDword(
        std::uint32_t address) = 0;
    virtual void DrawIcon(std::uint32_t icon, std::uint32_t context,
                          std::int32_t x, std::int32_t y,
                          std::uint32_t bounds_address,
                          bool bypass_intersection,
                          std::uint32_t alpha) = 0;
};

class IMinimapObjectLookupRuntime {
public:
    virtual ~IMinimapObjectLookupRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadDword(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(
        std::uint32_t address) = 0;
};

class IMinimapObjectActiveRuntime : public IMinimapObjectLookupRuntime {
public:
    virtual void WriteByte(std::uint32_t address,
                           std::uint8_t value) = 0;
};

class IMinimapObjectHitRuntime : public IMinimapObjectLookupRuntime {
public:
    [[nodiscard]] virtual const MinimapIconState& IconState(
        std::uint32_t icon) = 0;
};

class IMinimapObjectInsertRuntime {
public:
    virtual ~IMinimapObjectInsertRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Allocate(
        std::uint32_t bytes) = 0;
    virtual void CopyObject(std::uint32_t destination,
                            std::uint32_t source) = 0;
    virtual void AssignNestedArray(std::uint32_t destination,
                                   std::uint32_t source) = 0;
    virtual void CleanupGroup(std::uint32_t address) = 0;
    virtual void Free(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t ReadDword(
        std::uint32_t address) = 0;
    virtual void WriteDword(std::uint32_t address,
                            std::uint32_t value) = 0;
};

class IMinimapObjectGroupRuntime {
public:
    virtual ~IMinimapObjectGroupRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadDword(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(
        std::uint32_t address) = 0;
    virtual void WriteDword(std::uint32_t address,
                            std::uint32_t value) = 0;
    virtual void WriteByte(std::uint32_t address,
                           std::uint8_t value) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(
        std::uint32_t bytes) = 0;
};

class IMinimapObjectArrayRuntime {
public:
    virtual ~IMinimapObjectArrayRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadDword(
        std::uint32_t address) = 0;
    virtual void WriteDword(std::uint32_t address,
                            std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(
        std::uint32_t bytes) = 0;
    virtual void Free(std::uint32_t address) = 0;
};

class MinimapObjectCollection final {
public:
    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t address, std::uint8_t incoming_stack_byte,
        IMinimapObjectCollectionRuntime& runtime);
    [[nodiscard]] static int Cleanup(
        std::uint32_t address, IMinimapObjectCollectionRuntime& runtime);
    [[nodiscard]] static bool AddIcon(
        std::uint32_t collection, std::string_view name,
        std::uint8_t name_pointer_low_byte,
        std::uint32_t id_pointer, std::string_view filename,
        std::int32_t x, std::int32_t y,
        IMinimapObjectAddRuntime& runtime);
    [[nodiscard]] static std::uint32_t RemoveIcon(
        std::uint32_t collection, std::string_view name,
        std::uint32_t id_pointer,
        IMinimapObjectRemoveRuntime& runtime);
    [[nodiscard]] static int ClearIcons(
        MinimapObjectCollectionState& state,
        IMinimapObjectClearRuntime& runtime);
    [[nodiscard]] static std::uint32_t ClearSection(
        std::uint32_t collection, std::uint32_t id_pointer,
        IMinimapObjectSectionRuntime& runtime);
    [[nodiscard]] static std::uint32_t DrawIcons(
        const MinimapObjectCollectionState& state,
        std::uint32_t context, std::int32_t x, std::int32_t y,
        std::uint32_t bounds_address,
        IMinimapObjectDrawRuntime& runtime);
    [[nodiscard]] static std::uint32_t FindIconByName(
        std::uint32_t group_collection, std::string_view name,
        IMinimapObjectLookupRuntime& runtime);
    [[nodiscard]] static bool SetActiveIcon(
        MinimapObjectCollectionState& state, std::string_view name,
        std::uint32_t id_pointer,
        IMinimapObjectActiveRuntime& runtime);
    [[nodiscard]] static std::uint32_t FindGroupById(
        const MinimapObjectCollectionState& state,
        std::uint32_t id_pointer,
        IMinimapObjectLookupRuntime& runtime);
    [[nodiscard]] static std::uint32_t FindIconAtPoint(
        const MinimapObjectCollectionState& state,
        std::int32_t x, std::int32_t y,
        std::uint32_t id_pointer,
        IMinimapObjectHitRuntime& runtime);
    [[nodiscard]] static std::uint32_t InsertGroups(
        std::uint32_t collection_address,
        MinimapObjectCollectionState& state,
        std::uint32_t insertion_address,
        std::uint32_t count,
        std::uint32_t source_address,
        IMinimapObjectInsertRuntime& runtime);
    static void CopyGroup(std::uint32_t destination,
                          std::uint32_t source,
                          IMinimapObjectGroupRuntime& runtime);
    [[nodiscard]] static std::uint32_t AssignNestedArray(
        std::uint32_t destination,
        std::uint32_t source,
        IMinimapObjectArrayRuntime& runtime);
};

}  // namespace pal4
