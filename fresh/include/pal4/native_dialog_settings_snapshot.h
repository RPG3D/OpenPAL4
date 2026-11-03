#pragma once

#include "pal4/native_dialog_settings_file_reader.h"
#include "pal4/native_dialog_settings_file_writer.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <unordered_map>
#include <vector>

namespace pal4 {

// Owns settings data used by the reconstructed pal4.set reader and writer.
// Tree node IDs are local to this adapter; game tree consumers still need a
// native node owner when the full DialogManager is integrated.
class NativeDialogSettingsSnapshot final :
    public INativeDialogSettingsStateSink,
    public INativeDialogSettingsStateSource {
public:
    NativeDialogSettingsSnapshot() = default;
    NativeDialogSettingsSnapshot(const NativeDialogSettingsSnapshot&) = delete;
    NativeDialogSettingsSnapshot& operator=(
        const NativeDialogSettingsSnapshot&) = delete;
    NativeDialogSettingsSnapshot(NativeDialogSettingsSnapshot&&) = delete;
    NativeDialogSettingsSnapshot& operator=(
        NativeDialogSettingsSnapshot&&) = delete;

    [[nodiscard]] DialogManagerState& State() noexcept { return state_; }
    [[nodiscard]] const DialogManagerState& State() const noexcept {
        return state_;
    }
    //5C3B10: defeated Monster keys in the existing pal4.set tree132. Native
    //signed ordering/uniqueness uses STL and the same storage as Load/Save.
    void RecordMonsterProgress(std::int32_t key);
    bool HasMonsterProgress(std::int32_t key) const;
    void RecordEquipmentProgress(std::int32_t key);
    bool HasEquipmentProgress(std::int32_t key) const;
    // IDA5CE300: opened-object CRC keys use unsigned ordering in tree116.
    void RecordEventKey(std::uint32_t key);
    bool HasEventKey(std::uint32_t key) const;

    void Reset(DialogManagerState& state) override;
    void ClearTree(DialogManagerState& state, std::size_t offset) override;
    void InsertTreeValue(DialogManagerState& state, std::size_t offset,
                         std::uint32_t value) override;
    void WriteStoryWord(std::uint32_t address, std::uint32_t value) override;
    void SetAudioLevel(std::uint32_t index, float value) override;
    [[nodiscard]] std::size_t AudioStreamCount(
        GlobalDataAudioCollection collection) override;
    [[nodiscard]] std::uint32_t AudioStreamAt(
        GlobalDataAudioCollection collection, std::size_t index) override;
    [[nodiscard]] std::uint32_t AudioStreamKind(
        std::uint32_t stream) override;
    void SetStreamVolume(std::uint32_t stream, float value) override;
    void RefreshStream(std::uint32_t stream) override;
    void SetAtomicPluginFloatData(float value) override;
    [[nodiscard]] bool HasEffectSystem() override;
    [[nodiscard]] std::int32_t SetEffectSystemSquaredValue(
        float value) override;

    void SynchronizeStoryData(DialogManagerState& state) override;
    [[nodiscard]] float AudioLevel(std::uint32_t index) override;
    [[nodiscard]] std::uint32_t FirstNode(
        const DialogManagerState& state, std::size_t offset) override;
    [[nodiscard]] std::uint32_t NextNode(
        std::uint32_t node, bool primary_tree) override;
    [[nodiscard]] std::uint32_t NodeValue(std::uint32_t node) override;
    [[nodiscard]] std::uint32_t StoryWord(std::uint32_t address) override;

private:
    void InsertProgressKey(DialogManagerState&,std::size_t offset,std::int32_t key);
    bool HasProgressKey(std::size_t offset,std::int32_t key) const;
    [[nodiscard]] static std::uint32_t NodeId(
        std::size_t offset, std::size_t index);
    [[nodiscard]] std::size_t StoryIndex(std::uint32_t address) const;

    DialogManagerState state_{};
    std::unordered_map<std::size_t, std::vector<std::uint32_t>> trees_;
    std::array<std::uint32_t, 38> stories_{};
    std::array<float, 3> audio_{};
    float atomic_scale_{};
    float effect_squared_{};
};

}  // namespace pal4
