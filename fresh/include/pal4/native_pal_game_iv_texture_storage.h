#pragma once

#include "pal4/game_system_data.h"
#include "pal4/native_base_object_memory.h"
#include "pal4/rw_tex_dictionary_tree.h"
#include "pal4/script_ui_timer_save_data.h"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace pal4 {

class DialogManagerState;
class IGameDataPersistenceRuntime;
class NativeGameTimerQueue;

struct NativeGameUiTimerBinding {
    // The queue and manager image must outlive the PALGameIV owner.
    NativeGameTimerQueue* queue{};
    std::uint32_t manager{};
    std::function<void(std::uint32_t)> update_frame_text;
};

// Owns PALGameIV's x86 image, core storage, bone, string and texture tree.
// State registration is owned separately by NativePalGameIVStateList.
class NativePalGameIVTextureStorage final : public IScriptUiTimerSaveDataRuntime {
public:
    explicit NativePalGameIVTextureStorage(
        std::function<void(std::uint32_t)> destroy_dictionary);
    NativePalGameIVTextureStorage(const NativePalGameIVTextureStorage&) = delete;
    NativePalGameIVTextureStorage& operator=(
        const NativePalGameIVTextureStorage&) = delete;
    ~NativePalGameIVTextureStorage();

    [[nodiscard]] std::uint32_t Construct();
    [[nodiscard]] std::uint32_t Construct(
        const NativeGameUiTimerBinding& timer);
    void Destroy();
    // IDA5C2920: reset persistent core/Story storage and replace PalGameTimer.
    void InitializeCoreSystems();
    [[nodiscard]] std::uint32_t Address() const noexcept { return address_; }
    [[nodiscard]] bool Constructed() const noexcept { return address_ != 0u; }
    [[nodiscard]] const RwTexDictionaryTreeState& Dictionary() const;
    // IDA6644A0: decrement a named cache reference, destroying only at zero.
    [[nodiscard]] bool RemoveTextureDictionary(std::string_view name);
    [[nodiscard]] std::uint32_t GlobalAddress(
        std::uint32_t original_address) const;
    [[nodiscard]] std::uint32_t GameDataEntry(std::int32_t index);
    [[nodiscard]] std::uint32_t Game() override;
    [[nodiscard]] std::uint32_t GameDataEntry(
        std::uint32_t game, std::uint32_t index) override;
    void SetGameDataEntry(std::int32_t index, std::uint32_t value);
    // The bound global data image must outlive this owner.
    void BindGlobalData(DialogManagerState& state);
    [[nodiscard]] bool DeserializeGameData(std::uint32_t source, IGameDataPersistenceRuntime& runtime);
    // Borrowed live Story pointers in PALGameIV+154, copied without ownership transfer.
    [[nodiscard]] std::vector<std::uint32_t> StoryObjects();
    // IDA5C3BD0: signed value through the Story pointer table, distinct from GetEntry.
    [[nodiscard]] std::int32_t PointerEntryValue(std::int32_t index);
    void AddStateMonster(std::string_view name);
    //5B6A40: visits the actual retained names in insertion order, without
    // clearing them. Callback views are borrowed; collection mutation/reentry
    // is rejected until traversal unwinds.
    void VisitStateMonsters(const std::function<void(std::string_view)>& visitor);
    void ClearStateCollection();
    [[nodiscard]] std::size_t StateMonsterCount() const;
    [[nodiscard]] std::size_t StateCollectionAllocationCount() const;
    [[nodiscard]] std::size_t CoreAllocationCount() const noexcept {
        return core_.AllocationCount();
    }

private:
    class StateCollectionOwner;
    class CoreRuntime final : public IGameSystemDataRuntime {
    public:
        void Bind(std::uint32_t address) noexcept { address_ = address; }
        void BindGlobalData(DialogManagerState& state) noexcept {
            global_data_ = &state;
        }
        void BindTimer(const NativeGameUiTimerBinding& timer);
        void Unbind() noexcept;
        void ReleaseStorage();
        void ReleaseUiTimer();
        void InitializeMembers();
        void ReleaseMembers();
        [[nodiscard]] std::uint32_t GlobalAddress(
            std::uint32_t original_address) const;
        [[nodiscard]] std::size_t AllocationCount() const noexcept {
            return memory_.OutstandingAllocations();
        }
        void InitializeNamedText(std::uint32_t address,
                                 std::string_view text) override;
        void ReleaseNamedText(std::uint32_t address) override;
        void ZeroBytes(std::uint32_t address, std::uint32_t count) override;
        void FillDwords(std::uint32_t address, std::uint32_t count,
                        std::uint32_t value) override;
        void WriteDword(std::uint32_t address, std::uint32_t value) override;
        [[nodiscard]] std::uint32_t ReadDword(std::uint32_t address) override;
        void ErasePointerRange(std::uint32_t collection,
                                   std::uint32_t begin,
                                   std::uint32_t end) override;
        void InsertPointer(std::uint32_t collection,
                           std::uint32_t address, bool insert_copies) override;
        [[nodiscard]] std::vector<std::uint32_t> PointerEntries(
            std::uint32_t collection) override;
        void ReleasePointerCollection(std::uint32_t collection) override;
        void Log(std::string_view format, std::string_view source,
                 std::uint32_t line) override;
        [[nodiscard]] std::uint32_t UiFrameManager() override;
        void UpdateTimerFrameText(std::uint32_t manager) override;
        void InitializeTimerName(std::string_view name) override;
        void RemoveTimer(std::uint32_t queue,
                         std::string_view name) override;
        [[nodiscard]] std::uint32_t AllocateTimerCallback(
            std::uint32_t size, std::uint32_t vtable,
            std::uint32_t owner, std::uint32_t method) override;
        [[nodiscard]] std::uint32_t ConstructTimerEntry(
            std::string_view name, std::uint32_t duration_bits,
            std::uint32_t callback) override;
        void AddTimer(std::uint32_t queue, std::uint32_t entry) override;
        void DestroyTimerEntry(std::uint32_t entry) override;
        [[nodiscard]] std::int32_t CleanupTimerName() override;
        [[nodiscard]] std::uint32_t GlobalDataSubsystem() override;

    private:
        void CheckCollection(std::uint32_t collection) const;
        void SyncCollection();
        NativeBaseObjectMemory memory_;
        std::vector<std::uint32_t> pointers_;
        std::uint32_t address_{};
        DialogManagerState* global_data_{};
        NativeGameUiTimerBinding timer_{};
        std::string timer_name_;
    };

    class Runtime final : public IRwTexDictionaryTreeRuntime {
    public:
        explicit Runtime(std::function<void(std::uint32_t)> destroy_dictionary)
            : destroy_dictionary_(std::move(destroy_dictionary)) {}
        [[nodiscard]] std::uint32_t Allocate(std::uint32_t bytes) override;
        [[nodiscard]] std::uint32_t Free(std::uint32_t address) override;
        [[nodiscard]] std::uint32_t ReadWord(std::uint32_t address) override;
        void WriteWord(std::uint32_t address, std::uint32_t value) override;
        [[nodiscard]] std::uint8_t ReadByte(std::uint32_t address) override;
        void WriteByte(std::uint32_t address, std::uint8_t value) override;
        [[nodiscard]] std::uint32_t SharedNil() override;
        void SetSharedNil(std::uint32_t address) override;
        [[nodiscard]] std::uint32_t Users() override;
        void SetUsers(std::uint32_t count) override;
        void FirstElement(RwTexDictionaryTreeState& state) override;
        void DestroyDictionary(std::uint32_t dictionary) override;
        void CommitConstruction() noexcept { allocated_.clear(); }
        void RollbackConstruction(std::uint32_t nil, std::uint32_t users);

    private:
        std::function<void(std::uint32_t)> destroy_dictionary_;
        std::vector<std::uint32_t> allocated_;
    };

    CoreRuntime core_;
    Runtime runtime_;
    std::unique_ptr<StateCollectionOwner> state_collection_;
    std::uint32_t address_{};
};

}  // namespace pal4
