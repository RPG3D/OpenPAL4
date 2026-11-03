#pragma once

#include <cstdint>
#include <string_view>

namespace pal4 {

class IMusicUiRuntime {
public:
    virtual ~IMusicUiRuntime() = default;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t UIFrameManager() = 0;
    virtual void SetUIFlag(std::uint32_t manager, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t Console() = 0;
    [[nodiscard]] virtual bool AllocateTrack(std::string_view track) = 0;
    virtual void ReleaseTrack() = 0;
    [[nodiscard]] virtual std::uint32_t MusicPlayer() = 0;
    virtual void FadeInMusic(std::uint32_t player,
                             std::string_view track,
                             double seconds, std::uint32_t flags) = 0;
    virtual void UpdateUI(std::uint32_t manager, double delta) = 0;
    [[nodiscard]] virtual std::uint32_t ScriptExecutor() = 0;
    [[nodiscard]] virtual std::uint32_t ProcessLog(
        std::uint32_t logger, double delta) = 0;
};

class MusicUiState final {
public:
    static constexpr std::uint32_t kVtable = 0x848664u;

    // IDA 0x5905e0/0x5905f0/0x5906e0/0x590710.
    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t object, IMusicUiRuntime& runtime);
    [[nodiscard]] static std::uint32_t Enter(
        std::uint32_t unused1, std::uint32_t unused2,
        IMusicUiRuntime& runtime);
    [[nodiscard]] static std::uint32_t Update(
        float delta, std::uint32_t unused1,
        std::uint32_t unused2, IMusicUiRuntime& runtime);
    [[nodiscard]] static std::uint32_t GetTypeCode();
};

}  // namespace pal4
