#pragma once

#include <cstdint>
#include <string_view>
#include <vector>

namespace pal4 {

class IGameSystemDataRuntime {
public:
    virtual ~IGameSystemDataRuntime() = default;
    virtual void InitializeNamedText(std::uint32_t address,
                                     std::string_view text) = 0;
    virtual void ReleaseNamedText(std::uint32_t address) = 0;
    virtual void ZeroBytes(std::uint32_t address, std::uint32_t count) = 0;
    virtual void FillDwords(std::uint32_t address, std::uint32_t count,
                            std::uint32_t value) = 0;
    virtual void WriteDword(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t ReadDword(std::uint32_t address) = 0;
    virtual void ErasePointerRange(std::uint32_t collection,
                                       std::uint32_t begin,
                                       std::uint32_t end) = 0;
    virtual void InsertPointer(std::uint32_t collection,
                               std::uint32_t address, bool insert_copies) = 0;
    [[nodiscard]] virtual std::vector<std::uint32_t> PointerEntries(
        std::uint32_t collection) = 0;
    virtual void ReleasePointerCollection(std::uint32_t collection) = 0;
    virtual void Log(std::string_view format, std::string_view source,
                     std::uint32_t line) = 0;
    [[nodiscard]] virtual std::uint32_t UiFrameManager() = 0;
    virtual void UpdateTimerFrameText(std::uint32_t manager) = 0;
    virtual void InitializeTimerName(std::string_view name) = 0;
    virtual void RemoveTimer(std::uint32_t queue,
                             std::string_view name) = 0;
    [[nodiscard]] virtual std::uint32_t AllocateTimerCallback(
        std::uint32_t size, std::uint32_t vtable,
        std::uint32_t owner, std::uint32_t method) = 0;
    [[nodiscard]] virtual std::uint32_t ConstructTimerEntry(
        std::string_view name, std::uint32_t duration_bits,
        std::uint32_t callback) = 0;
    virtual void AddTimer(std::uint32_t queue, std::uint32_t entry) = 0;
    virtual void DestroyTimerEntry(std::uint32_t entry) = 0;
    [[nodiscard]] virtual std::int32_t CleanupTimerName() = 0;
    [[nodiscard]] virtual std::uint32_t GlobalDataSubsystem() = 0;
};

class GameSystemData final {
public:
    // 5C297C fills 13 DWORDs from +0x28; GetEntry uses +8+index*4.
    static constexpr std::uint32_t kFirstInitialUiPermissionEntry=8u;
    static constexpr std::uint32_t kInitialUiPermissionEntryCount=13u;
    static constexpr std::uint32_t kInitialUiPermission=1u;
    // 5C2981 writes +0x70; GetEntry26 addresses +8+26*4.
    static constexpr std::uint32_t kMinimapSkillPermissionEntry=26u;
    static constexpr std::uint32_t kInitialMinimapSkillPermission=1u;
    explicit GameSystemData(std::uint32_t address) : address_(address) {}

    // IDA 0x5c2810 / 0x5c2920.
    [[nodiscard]] std::uint32_t InitializeVariables(
        IGameSystemDataRuntime& runtime) const;
    // Construction can establish storage before the UI timer owner exists.
    [[nodiscard]] std::uint32_t InitializeVariableStorage(
        IGameSystemDataRuntime& runtime) const;
    [[nodiscard]] std::int32_t InitializeCoreSystems(
        IGameSystemDataRuntime& runtime) const;
    void InitializeCoreStorage(IGameSystemDataRuntime& runtime) const;
    [[nodiscard]] std::int32_t InitializeUiTimer(
        IGameSystemDataRuntime& runtime) const;
    // IDA 0x5c2f10 / 0x5c2f30.
    [[nodiscard]] std::uint32_t GetEntry(std::int32_t index,
                                         IGameSystemDataRuntime& runtime) const;
    void SetEntry(std::int32_t index, std::uint32_t value,
                  IGameSystemDataRuntime& runtime) const;
    // IDA 0x5c3bd0: read the first dword of the indexed pointer target.
    [[nodiscard]] std::int32_t PointerEntryValue(
        std::int32_t index, IGameSystemDataRuntime& runtime) const;

private:
    std::uint32_t address_;
};

}  // namespace pal4
