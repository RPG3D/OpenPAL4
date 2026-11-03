#include "pal4/game_system_data.h"

#include <array>

namespace pal4 {
namespace {

constexpr std::string_view kSource =
    R"(D:\VcProject\Pal4\project\Sourcecode\PAL4\PAL4Game\palGameData.cpp)";
constexpr std::string_view kBeginLog =
    "FILE:  [%s]\nLINE:  [%d]\nINFO:  palGameData initializing begin...\n\n\n";
constexpr std::string_view kEndLog =
    "FILE:  [%s]\nLINE:  [%d]\nINFO:  palGameData initialize ok\n\n\n";
constexpr std::string_view kTimerName = "PalGameTimer";
constexpr std::array<std::string_view, 3> kNames{
    "ElapsedTime", "MVar", "MovieFlag"};
constexpr std::uint32_t kPointerCollectionOffset = 0x154u;

}  // namespace

std::uint32_t GameSystemData::InitializeVariables(
    IGameSystemDataRuntime& runtime) const {
    (void)InitializeVariableStorage(runtime);
    try {
        (void)InitializeUiTimer(runtime);
    } catch (...) {
        runtime.ReleasePointerCollection(address_ + kPointerCollectionOffset);
        for (std::uint32_t i = static_cast<std::uint32_t>(kNames.size());
             i != 0u; --i)
            runtime.ReleaseNamedText(address_ + 0x124u + (i - 1u) * 0x10u);
        throw;
    }
    return address_;
}

std::uint32_t GameSystemData::InitializeVariableStorage(
    IGameSystemDataRuntime& runtime) const {
    std::uint32_t initialized = 0u;
    bool collection_initialized = false;
    try {
        for (std::uint32_t i = 0u; i < kNames.size(); ++i) {
            runtime.InitializeNamedText(address_ + 0x124u + i * 0x10u,
                                        kNames[i]);
            ++initialized;
        }
        // The native constructor reads an uninitialized first byte in these
        // records; zeroing the whole record gives defined equivalent state.
        runtime.ZeroBytes(address_ + kPointerCollectionOffset, 0x20u);
        collection_initialized = true;
        InitializeCoreStorage(runtime);
    } catch (...) {
        if (collection_initialized)
            runtime.ReleasePointerCollection(address_ + kPointerCollectionOffset);
        while (initialized != 0u) {
            --initialized;
            runtime.ReleaseNamedText(address_ + 0x124u + initialized * 0x10u);
        }
        throw;
    }
    return address_;
}

std::int32_t GameSystemData::InitializeCoreSystems(
    IGameSystemDataRuntime& runtime) const {
    InitializeCoreStorage(runtime);
    return InitializeUiTimer(runtime);
}

void GameSystemData::InitializeCoreStorage(
    IGameSystemDataRuntime& runtime) const {
    runtime.Log(kBeginLog, kSource, 40u);
    runtime.ZeroBytes(address_, 0x124u);
    runtime.FillDwords(address_ + 8u + kFirstInitialUiPermissionEntry*4u,
                       kInitialUiPermissionEntryCount, kInitialUiPermission);
    runtime.WriteDword(address_ + 8u + kMinimapSkillPermissionEntry*4u, kInitialMinimapSkillPermission);

    const auto collection = address_ + kPointerCollectionOffset;
    const auto begin = runtime.ReadDword(collection + 4u);
    const auto end = runtime.ReadDword(collection + 8u);
    runtime.ErasePointerRange(collection, begin, end);
    runtime.InsertPointer(collection, 0x90dec0u, false);
    for (std::uint32_t address = 0x90deecu; address <= 0x90df28u;
         address += 4u) {
        runtime.InsertPointer(collection, address, false);
    }
    for (std::uint32_t address = 0x90df2cu; address <= 0x90df7cu;
         address += 4u) {
        runtime.InsertPointer(collection, address, true);
    }
    for (const auto address : runtime.PointerEntries(collection))
        runtime.WriteDword(address, 0u);
    for (std::uint32_t address = 0x90dec4u; address <= 0x90dee8u;
         address += 4u) {
        runtime.WriteDword(address, 0u);
    }
}

std::int32_t GameSystemData::InitializeUiTimer(
    IGameSystemDataRuntime& runtime) const {
    runtime.UpdateTimerFrameText(runtime.UiFrameManager());
    runtime.InitializeTimerName(kTimerName);
    try {
        const auto remove_manager = runtime.UiFrameManager();
        runtime.RemoveTimer(runtime.ReadDword(remove_manager + 0x1a0u),
                            kTimerName);
        const auto add_manager = runtime.UiFrameManager();
        const auto queue = runtime.ReadDword(add_manager + 0x1a0u);
        const auto callback = runtime.AllocateTimerCallback(
            12u, 0x8494b0u, address_, 0x5c3c10u);
        const auto entry = runtime.ConstructTimerEntry(
            kTimerName, 0x3f800000u, callback);
        try {
            runtime.AddTimer(queue, entry);
        } catch (...) {
            runtime.DestroyTimerEntry(entry);
            throw;
        }
        runtime.DestroyTimerEntry(entry);
        for (std::uint32_t offset = 0x164u; offset <= 0x170u;
             offset += 4u) {
            runtime.WriteDword(address_ + offset, 0u);
        }
        runtime.Log(kEndLog, kSource, 122u);
    } catch (...) {
        (void)runtime.CleanupTimerName();
        throw;
    }
    return runtime.CleanupTimerName();
}

std::uint32_t GameSystemData::GetEntry(
    std::int32_t index, IGameSystemDataRuntime& runtime) const {
    const auto offset = static_cast<std::uint32_t>(index) * 4u;
    if (index > 7) return runtime.ReadDword(address_ + offset + 8u);
    return runtime.ReadDword(runtime.GlobalDataSubsystem() + offset + 0x34u);
}

void GameSystemData::SetEntry(
    std::int32_t index, std::uint32_t value,
    IGameSystemDataRuntime& runtime) const {
    const auto offset = static_cast<std::uint32_t>(index) * 4u;
    if (index > 7) {
        runtime.WriteDword(address_ + offset + 8u, value);
    } else {
        runtime.WriteDword(runtime.GlobalDataSubsystem() + offset + 0x34u,
                           value);
    }
}

std::int32_t GameSystemData::PointerEntryValue(
    const std::int32_t index, IGameSystemDataRuntime& runtime) const {
    if (index < 0) return -1;
    const auto begin = runtime.ReadDword(address_ + 0x158u);
    if (begin == 0u) return -1;
    const auto end = runtime.ReadDword(address_ + 0x15cu);
    if (static_cast<std::uint32_t>(index) >= (end - begin) / 4u)
        return -1;
    const auto pointer = runtime.ReadDword(begin + 4u *
                                         static_cast<std::uint32_t>(index));
    return static_cast<std::int32_t>(runtime.ReadDword(pointer));
}

}  // namespace pal4
