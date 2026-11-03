#pragma once

#include <cstdint>
#include <string_view>

namespace pal4 {

class ICombatEndSoundRuntime {
public:
    virtual ~ICombatEndSoundRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t CombatSystemInstance() = 0;
    [[nodiscard]] virtual bool IsCombatSystemInactive(
        std::uint32_t system) = 0;
    [[nodiscard]] virtual std::uint32_t BackgroundMusicPlayer() = 0;
    virtual void FadeInSpecialMusic(std::uint32_t player,
                                    std::uint32_t text_record,
                                    float seconds, std::uint32_t mode) = 0;
    virtual void RestoreNormalMusic(std::uint32_t player,
                                    std::uint32_t music) = 0;
    [[nodiscard]] virtual std::uint32_t CreateText(
        std::string_view literal) = 0;
    virtual void AppendText(std::uint32_t destination,
                            std::uint32_t source) = 0;
    [[nodiscard]] virtual std::uint32_t ReleaseText(
        std::uint32_t record) = 0;
    [[nodiscard]] virtual std::uint32_t AudioManagerInstance() = 0;
    [[nodiscard]] virtual std::uint32_t TemporaryOutputSlot() = 0;
    virtual void PlayAudio(std::uint32_t manager, std::uint32_t output,
                           std::uint32_t path_record,
                           std::uint32_t mode) = 0;
};

class CombatEndSoundPlayback final {
public:
    // IDA 0x579bd0.
    [[nodiscard]] static std::uint32_t OnBattleEnd(
        std::uint32_t event, ICombatEndSoundRuntime& runtime);
    // IDA 0x579d00.
    [[nodiscard]] static std::uint8_t OnMusicCue(
        std::uint32_t event, ICombatEndSoundRuntime& runtime);
    // IDA 0x579ed0.
    [[nodiscard]] static std::uint32_t PlayEffect(
        std::uint32_t output, std::uint32_t text_record,
        std::uint32_t mode, ICombatEndSoundRuntime& runtime);
};

}  // namespace pal4
