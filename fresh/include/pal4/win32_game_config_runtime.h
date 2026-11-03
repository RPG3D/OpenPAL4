#pragma once

#include "pal4/binary_xml_config.h"
#include "pal4/game_config_initializer.h"

#include <filesystem>

namespace pal4 {

class Win32GameConfigRuntime final : public IGameConfigRuntime {
public:
    Win32GameConfigRuntime(std::filesystem::path config_directory,
                           IGameConfigLogger& logger)
        : config_directory_(std::move(config_directory)), logger_(logger) {}

    void GetTimerCaps(std::uint32_t& minimum_period,
                      std::uint32_t& maximum_period,
                      std::uint32_t byte_count) override;
    void BeginTimePeriod(std::uint32_t period) override;
    [[nodiscard]] std::uint32_t InitializeLogger() override;
    void AppendLog(std::uint32_t logger, std::string_view format,
                   std::string_view source_file,
                   std::uint32_t source_line) override;
    void InitializeDocument(ConfigDocument& document) override;
    void LoadConfig(std::string_view filename,
                    ConfigDocument& document) override;
    [[nodiscard]] std::uint32_t FindValue(
        ConfigDocument& document, std::string_view key) override;
    [[nodiscard]] std::uint32_t ValueToInt(
        std::uint32_t token, std::uint32_t& output) override;
    void DestroyDocument(ConfigDocument& document) override;

    [[nodiscard]] bool Loaded() const { return loaded_; }

private:
    std::filesystem::path config_directory_;
    IGameConfigLogger& logger_;
    BinaryXmlConfig parsed_;
    bool loaded_ = false;
};

}  // namespace pal4
