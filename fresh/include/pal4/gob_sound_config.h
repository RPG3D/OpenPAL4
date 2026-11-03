#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace pal4 {

class IGobSoundPlaybackRuntime {
public:
    virtual ~IGobSoundPlaybackRuntime() = default;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual bool IsAudioPlaying(std::uint32_t handle) = 0;
    [[nodiscard]] virtual std::uint8_t StopAudio(std::uint32_t handle) = 0;
    [[nodiscard]] virtual std::uint8_t PauseAudio(std::uint32_t handle) = 0;
};

// Shared 5A6560 binding inherited by the supported GOB classes.
class IGobClumpAdapterRuntime {
public:
    virtual ~IGobClumpAdapterRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t MovementAdapter() = 0;
    [[nodiscard]] virtual std::uint32_t RenderProperty(std::uint32_t object) = 0;
    [[nodiscard]] virtual std::uint32_t SetClumpProperties(
        std::uint32_t adapter, std::uint32_t clump,
        std::uint32_t property, std::uint32_t first,
        std::uint32_t second, std::uint32_t render_property) = 0;
};

class IGobSoundConfigValueRuntime : public IGobSoundPlaybackRuntime {
public:
    virtual void WriteFloat(std::uint32_t address, float value) = 0;
    virtual std::string ReadConfigString(std::uint32_t node,
        std::string_view key, std::string_view fallback) = 0;
    virtual float ReadConfigFloat(std::uint32_t node,
        std::string_view key, float fallback) = 0;
    virtual void AssignNameRecord(std::uint32_t record, std::string_view name) = 0;
    virtual std::uint8_t CheckAudioActive(std::uint32_t handle) = 0;
    virtual std::uint32_t PlayAudioHandle(std::uint32_t handle) = 0;
};

class IGobSoundConfigRuntime : public IGobSoundConfigValueRuntime,
                              public IGobClumpAdapterRuntime {
public:
    ~IGobSoundConfigRuntime() override = default;
    virtual void CleanupBeforeSceneLoad(std::uint32_t object) = 0;
    [[nodiscard]] virtual bool UsesObjectPackage(std::string_view path) = 0;
    [[nodiscard]] virtual bool UsesWeaponPackage(std::string_view path) = 0;
    virtual void LoadObjectPackage() = 0;
    virtual void LoadWeaponPackage() = 0;
    [[nodiscard]] virtual std::uint32_t CurrentScenePackage() = 0;
    virtual void LoadScenePackage(std::uint32_t package) = 0;
    [[nodiscard]] virtual std::uint32_t InitializeBaseModel(
        std::uint32_t object, std::uint32_t first, std::string_view path,
        std::uint32_t fourth, std::uint32_t fifth) = 0;
    virtual void CleanupScriptPackages() = 0;
    [[nodiscard]] virtual std::uint32_t FindXmlChild(
        std::uint32_t parent, std::string_view name) = 0;
    virtual void RemoveXmlChild(std::uint32_t parent,
                                std::uint32_t child) = 0;
    [[nodiscard]] virtual std::uint32_t CreateXmlChild(
        std::uint32_t parent, std::string_view name) = 0;
    [[nodiscard]] virtual std::uint32_t InstallRenderCallback(
        std::uint32_t adapter, std::uint32_t clump) = 0;
    virtual void ApplyBaseParameters(std::uint32_t object) = 0;
};

}  // namespace pal4
