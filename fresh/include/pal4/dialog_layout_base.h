#pragma once

#include <cstdint>
#include <string>

namespace pal4 {

class IDialogLayoutRuntime {
public:
    virtual ~IDialogLayoutRuntime() = default;
    [[nodiscard]] virtual std::uint8_t IndeterminateBufferByte() = 0;
    virtual void WriteByte(std::uint32_t address,
                           std::uint8_t value) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    virtual void SetErrorFlag(std::uint32_t record,
                              std::uint32_t value) = 0;
    [[nodiscard]] virtual bool AllocateBuffer(
        std::uint32_t record, std::uint32_t length,
        bool copy_mode) = 0;
    virtual void CopyBytes(std::uint32_t record,
                           const std::string& value) = 0;
    virtual void SetBufferSize(std::uint32_t record,
                               std::uint32_t length) = 0;
    virtual void AppendBuffer(std::uint32_t record,
                              const std::string& suffix) = 0;
    [[nodiscard]] virtual std::string ReadBuffer(
        std::uint32_t record) = 0;
    virtual void ProcessPkScripts() = 0;
    virtual void CleanupPkScripts() = 0;
    [[nodiscard]] virtual std::string ConvertToUtf8(
        const std::string& value) = 0;
    [[nodiscard]] virtual std::uint32_t LoadWindowLayout(
        const std::string& path,
        const std::string& utf8_prefix,
        const std::string& error_prefix) = 0;
    virtual void CleanupBuffer(std::uint32_t record) = 0;
};

// Parent ownership is also used without the legacy allocation/destructor path.
class IDialogLayoutParentRuntime {
public:
    virtual ~IDialogLayoutParentRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void BuildUtf8Buffer(std::uint32_t string) = 0;
    [[nodiscard]] virtual std::uint32_t DesktopWindow() = 0;
    [[nodiscard]] virtual bool IsChild(std::uint32_t parent, std::uint32_t child) = 0;
    virtual void RemoveChild(std::uint32_t parent, std::uint32_t child) = 0;
    virtual void AddChild(std::uint32_t parent, std::uint32_t child) = 0;
};

class IDialogLayoutLifecycleRuntime : public IDialogLayoutParentRuntime {
public:
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    virtual void DestroyWindow(std::uint32_t window) = 0;
    virtual void Free(std::uint32_t address) = 0;
};

class IDialogLayoutLookupRuntime {
public:
    virtual ~IDialogLayoutLookupRuntime() = default;
    [[nodiscard]] virtual std::string ConvertToUtf8(
        const std::string& source) = 0;
    [[nodiscard]] virtual std::uint32_t GetWindow(
        const std::u32string& name) = 0;
};

class IDialogLayoutVirtualRuntime {
public:
    virtual ~IDialogLayoutVirtualRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Invoke(
        std::uint32_t object, std::uint32_t vtable_offset) = 0;
};

class DialogLayoutBase final {
public:
    static constexpr std::uint32_t kVtable = 0x84438cu;

    // IDA 0x4baec0.
    [[nodiscard]] static std::uint32_t LoadLayout(
        std::uint32_t object, const std::string& layout,
        const std::string* prefix,
        const std::string* name_override,
        IDialogLayoutRuntime& runtime);

    // IDA 0x4bb230 / 0x4bb250.
    [[nodiscard]] static std::uint32_t DeletingDestruct(
        std::uint32_t object, std::uint8_t flags,
        IDialogLayoutLifecycleRuntime& runtime);
    static void Destruct(std::uint32_t object,
                         IDialogLayoutLifecycleRuntime& runtime);

    // IDA 0x4bb300 / 0x4bb380.
    static void SetParent(std::uint32_t object, bool attach,
                          std::uint32_t parent,
                          IDialogLayoutParentRuntime& runtime);
    [[nodiscard]] static bool IsVisible(
        std::uint32_t object,
        IDialogLayoutParentRuntime& runtime);

    // IDA 0x4bb390 / 0x4bb3b0.
    [[nodiscard]] static std::uint32_t GetWindow(
        const std::string& source,
        IDialogLayoutLookupRuntime& runtime);
    [[nodiscard]] static std::uint32_t GetWindowByName(
        const std::string& utf8,
        IDialogLayoutLookupRuntime& runtime);

    // IDA 0x4bb620 / 0x4bb630 / 0x4bb640.
    [[nodiscard]] static std::uint32_t InvokeVirtual12(
        std::uint32_t object, IDialogLayoutVirtualRuntime& runtime);
    [[nodiscard]] static std::uint32_t InvokeVirtual16(
        std::uint32_t object, IDialogLayoutVirtualRuntime& runtime);
    [[nodiscard]] static std::uint32_t InvokeVirtual20(
        std::uint32_t object, IDialogLayoutVirtualRuntime& runtime);
};

}  // namespace pal4
