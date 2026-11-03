#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace pal4 {

class IMissionWindowQuestImageRuntime {
public:
    virtual ~IMissionWindowQuestImageRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ImageFlag(
        std::uint32_t quest, std::uint32_t offset) = 0;
    [[nodiscard]] virtual std::string ImageKey(
        std::uint32_t quest, std::uint32_t offset) = 0;
    virtual void PreparePkFileManager() = 0;
    virtual void CleanupPkFileManager() = 0;
    [[nodiscard]] virtual std::string ConvertToUtf8(
        std::string_view text) = 0;
    [[nodiscard]] virtual bool ImagesetPresent(
        std::string_view name) = 0;
    virtual void CreateImagesetFromImageFile(
        std::string_view name, std::string_view path,
        std::string_view resource_group) = 0;
    [[nodiscard]] virtual std::uint32_t GetImageset(
        std::string_view name) = 0;
    virtual void SetAutoScalingEnabled(std::uint32_t imageset,
                                       bool enabled) = 0;
    [[nodiscard]] virtual std::uint32_t GetImage(
        std::uint32_t imageset, std::string_view name) = 0;
};

class MissionWindowQuestImage final {
public:
    [[nodiscard]] static std::uint32_t Load(
        std::uint32_t quest, IMissionWindowQuestImageRuntime& runtime);
};

}  // namespace pal4
