#pragma once

#include "pal4/binary_xml_config.h"
#include "pal4/dialog_manager_settings.h"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <optional>
#include <string>
#include <utility>

namespace pal4 {

class INativeDialogSettingsStateSink {
public:
    virtual ~INativeDialogSettingsStateSink() = default;
    virtual void Reset(DialogManagerState& state) = 0;
    virtual void ClearTree(DialogManagerState& state,
                           std::size_t tree_offset) = 0;
    virtual void InsertTreeValue(DialogManagerState& state,
                                 std::size_t tree_offset,
                                 std::uint32_t value) = 0;
    virtual void WriteStoryWord(std::uint32_t address,
                                std::uint32_t value) = 0;
    virtual void SetAudioLevel(std::uint32_t index, float value) = 0;
    [[nodiscard]] virtual std::size_t AudioStreamCount(
        GlobalDataAudioCollection collection) = 0;
    [[nodiscard]] virtual std::uint32_t AudioStreamAt(
        GlobalDataAudioCollection collection, std::size_t index) = 0;
    [[nodiscard]] virtual std::uint32_t AudioStreamKind(
        std::uint32_t stream) = 0;
    virtual void SetStreamVolume(std::uint32_t stream, float value) = 0;
    virtual void RefreshStream(std::uint32_t stream) = 0;
    virtual void SetAtomicPluginFloatData(float value) = 0;
    [[nodiscard]] virtual bool HasEffectSystem() = 0;
    [[nodiscard]] virtual std::int32_t SetEffectSystemSquaredValue(
        float value) = 0;
};

// Reads pal4.set through the reconstructed DialogManagerSettings::Load path.
class NativeDialogSettingsFileReader final :
    public IDialogManagerSettingsReadRuntime {
public:
    NativeDialogSettingsFileReader(std::filesystem::path game_root,
                                   INativeDialogSettingsStateSink& sink)
        : path_(std::move(game_root) / "pal4.set"), sink_(sink) {}

    void Reset(DialogManagerState& state) override;
    [[nodiscard]] std::uint32_t OpenFile(
        const char* path, const char* mode) override;
    void InitializeFileStream(std::uint32_t file) override;
    [[nodiscard]] std::uint32_t CreateInputArchive(
        std::uint32_t file) override;
    void ReadBytes(std::uint32_t archive, void* data,
                   std::size_t length) override;
    void WriteBytes(std::uint32_t archive, const void* data,
                    std::size_t length) override;
    [[nodiscard]] std::uint32_t AudioManager() override;
    [[nodiscard]] float AudioLevel(std::uint32_t manager,
                                    std::uint32_t index) override;
    void SetAudioLevel(std::uint32_t manager,
                       std::uint32_t index, float value) override;
    [[nodiscard]] std::size_t AudioStreamCount(
        std::uint32_t manager, GlobalDataAudioCollection collection) override;
    [[nodiscard]] std::uint32_t AudioStreamAt(
        std::uint32_t manager, GlobalDataAudioCollection collection,
        std::size_t index) override;
    [[nodiscard]] std::uint32_t AudioStreamKind(
        std::uint32_t stream) override;
    void SetStreamVolume(std::uint32_t stream, float value) override;
    void RefreshStream(std::uint32_t stream) override;
    void SetAtomicPluginFloatData(float value) override;
    [[nodiscard]] bool HasEffectSystem() override;
    [[nodiscard]] std::int32_t SetEffectSystemSquaredValue(
        float value) override;
    [[nodiscard]] std::string ReadFlagString(
        std::uint32_t archive) override;
    void ClearTree(DialogManagerState& state,
                   std::size_t tree_offset) override;
    void InsertTreeValue(DialogManagerState& state,
                         std::size_t tree_offset, std::uint32_t value) override;
    void WriteArrayWord(std::uint32_t address,
                        std::uint32_t value) override;
    [[nodiscard]] std::uint32_t CreateXml(const char* root) override;
    [[nodiscard]] bool XmlVersionMatches(std::uint32_t xml,
                                          std::uint32_t archive) override;
    [[nodiscard]] std::optional<std::uint32_t> XmlInteger(
        std::uint32_t xml, const std::string& name) override;
    void CloseFile(std::uint32_t file) override;
    void DestroyXml(std::uint32_t xml) override;
    void DestroyArchive(std::uint32_t archive) override;

private:
    void RequireFile(std::uint32_t handle) const;
    void RequireAudioManager(std::uint32_t handle) const;
    void RequireXml(std::uint32_t handle) const;

    std::filesystem::path path_;
    INativeDialogSettingsStateSink& sink_;
    std::ifstream input_;
    BinaryXmlConfig xml_;
    bool file_open_{};
    bool xml_open_{};
};

}  // namespace pal4
