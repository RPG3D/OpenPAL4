#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace pal4 {

class IGameSaveImagesetRuntime {
public:
    virtual ~IGameSaveImagesetRuntime() = default;
    [[nodiscard]] virtual std::uint32_t PackedFileManager() = 0;
    virtual void ProcessPackedFile3(std::uint32_t manager) = 0;
    virtual void ProcessPackedFile4(std::uint32_t manager) = 0;
    [[nodiscard]] virtual std::string ConvertToUtf8(
        std::string_view text) = 0;
    [[nodiscard]] virtual std::uint32_t ImagesetManager() = 0;
    [[nodiscard]] virtual bool IsImagesetPresent(
        std::uint32_t manager, std::string_view name) = 0;
    virtual void DestroyImageset(std::uint32_t manager,
                                  std::string_view name) = 0;
    virtual void CreateImagesetFromImageFile(
        std::uint32_t manager, std::string_view name,
        std::string_view image_file,
        std::string_view resource_group) = 0;
    [[nodiscard]] virtual std::uint32_t GetImageset(
        std::uint32_t manager, std::string_view name) = 0;
    virtual void SetAutoScalingEnabled(std::uint32_t imageset,
                                       bool enabled) = 0;
    [[nodiscard]] virtual std::uint32_t GetImage(
        std::uint32_t imageset, std::string_view name) = 0;
};

class GameSaveImagesetLoader final {
public:
    // IDA 0x5cc740.
    [[nodiscard]] static std::uint32_t Load(
        std::int32_t slot, IGameSaveImagesetRuntime& runtime);
};

}  // namespace pal4
