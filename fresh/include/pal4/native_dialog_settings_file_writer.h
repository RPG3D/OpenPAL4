#pragma once

#include "pal4/binary_xml_config.h"
#include "pal4/dialog_manager_settings.h"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>

namespace pal4 {

class INativeDialogSettingsStateSource {
public:
    virtual ~INativeDialogSettingsStateSource() = default;
    virtual void SynchronizeStoryData(DialogManagerState& state) = 0;
    [[nodiscard]] virtual float AudioLevel(std::uint32_t index) = 0;
    [[nodiscard]] virtual std::uint32_t FirstNode(
        const DialogManagerState& state, std::size_t tree_offset) = 0;
    [[nodiscard]] virtual std::uint32_t NextNode(
        std::uint32_t node, bool primary_tree) = 0;
    [[nodiscard]] virtual std::uint32_t NodeValue(std::uint32_t node) = 0;
    [[nodiscard]] virtual std::uint32_t StoryWord(std::uint32_t address) = 0;
};

// Writes pal4.set through the reconstructed DialogManagerSettings::Save path.
// The caller retains ownership of audio and live game data.
class NativeDialogSettingsFileWriter final :
    public IDialogManagerSettingsWriteRuntime {
public:
    NativeDialogSettingsFileWriter(std::filesystem::path game_root,
                                   INativeDialogSettingsStateSource& source);
    ~NativeDialogSettingsFileWriter() override;

    void ClearData(DialogManagerState& state) override;
    [[nodiscard]] std::uint32_t OpenFile(
        const char* path, const char* mode) override;
    void InitializeFileStream(std::uint32_t file) override;
    [[nodiscard]] std::uint32_t CreateOutputArchive(
        std::uint32_t file) override;
    void WriteBytes(std::uint32_t archive, const void* data,
                    std::size_t length) override;
    [[nodiscard]] std::uint32_t AudioManager() override;
    [[nodiscard]] float AudioLevel(std::uint32_t manager,
                                    std::uint32_t index) override;
    void WriteFlagString(std::uint32_t archive,
                         const std::string& flags) override;
    [[nodiscard]] std::uint32_t FirstNode(
        const DialogManagerState& state, std::size_t tree_offset) override;
    [[nodiscard]] std::uint32_t NextNode(
        std::uint32_t node, bool primary_tree) override;
    [[nodiscard]] std::uint32_t NodeValue(std::uint32_t node) override;
    [[nodiscard]] std::uint32_t ReadArrayWord(
        std::uint32_t address) override;
    [[nodiscard]] std::uint32_t CreateXml(const char* root) override;
    [[nodiscard]] std::uint32_t FindAttribute(
        std::uint32_t xml, const std::string& name) override;
    void SetAttribute(std::uint32_t attribute,
                      std::uint32_t value) override;
    void AddAttribute(std::uint32_t xml, const std::string& name,
                      std::uint32_t value) override;
    void WriteConfig(std::uint32_t xml, std::uint32_t archive) override;
    void CloseFile(std::uint32_t file) override;
    void DestroyXml(std::uint32_t xml) override;
    void DestroyArchive(std::uint32_t archive) override;

private:
    void RequireFile(std::uint32_t handle) const;
    void RequireXml(std::uint32_t handle) const;

    std::filesystem::path target_path_;
    std::filesystem::path temporary_path_;
    INativeDialogSettingsStateSource& source_;
    std::ofstream output_;
    BinaryXmlNode xml_;
    bool file_open_{};
    bool xml_open_{};
    bool complete_{};
};

}  // namespace pal4
