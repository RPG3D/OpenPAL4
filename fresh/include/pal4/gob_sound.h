#pragma once

#include "pal4/gob_resource_cleanup.h"

#include <cstdint>
#include <string_view>

namespace pal4 {

class IGobSoundTickRuntime;
class IGobSoundConfigRuntime;
class IGobSoundConfigValueRuntime;
class IGobClumpAdapterRuntime;
class IGobSoundPlaybackRuntime;
class IGobSoundSnapshotCaptureRuntime;

class IGobSoundConstructRuntime {
public:
    virtual ~IGobSoundConstructRuntime() = default;
    virtual void ConstructBase(std::uint32_t object) = 0;
    virtual void DestructBase(std::uint32_t object) = 0;
    virtual void InitializeNameRecord(std::uint32_t record) = 0;
    virtual void ReleaseNameRecord(std::uint32_t record) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
};

class IGobSoundRuntime : public IGobSoundConstructRuntime, public IGobSoundCleanupRuntime {
public:
    ~IGobSoundRuntime() override = default;
    virtual void WriteWord(std::uint32_t,std::uint32_t) override = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t size) = 0;
    virtual void Free(std::uint32_t address) = 0;
    virtual void CleanupBaseResources(std::uint32_t object) override = 0;
    [[nodiscard]] virtual std::uint32_t StopAndRemoveAudioHandle(
        std::uint32_t handle) override = 0;
    [[nodiscard]] virtual std::uint8_t SetBaseVisible(
        std::uint32_t object, std::uint32_t visible) = 0;
};

class GobSound final {
public:
    static constexpr std::uint32_t kSize = 0x208u;
    static constexpr std::uint32_t kVtable = 0x849260u;
    static constexpr std::uint32_t kAudioHandleOffset = 0x1e0u;
    static constexpr std::uint32_t kNameRecordOffset = 0x1e4u;

    [[nodiscard]] static std::uint32_t Create(IGobSoundRuntime& runtime);
    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t object, IGobSoundConstructRuntime& runtime);
    static void NoOp(std::uint32_t value) noexcept;
    [[nodiscard]] static std::uint32_t ReturnZero() noexcept;
    [[nodiscard]] static std::uint32_t TypeKey() noexcept;
    static void Destruct(std::uint32_t object,
                         IGobSoundConstructRuntime& runtime);
    [[nodiscard]] static std::uint32_t CleanupResources(
        std::uint32_t object, IGobSoundCleanupRuntime& runtime);
    [[nodiscard]] static std::uint32_t TickAndMaybePlay3D(
        std::uint32_t object, float elapsed,
        IGobSoundTickRuntime& runtime);
    [[nodiscard]] static std::uint32_t InitializeSceneAndConfig(
        std::uint32_t object, std::uint32_t first, std::string_view path,
        std::uint32_t fourth, std::uint32_t fifth,
        IGobSoundConfigRuntime& runtime);
    [[nodiscard]] static std::uint32_t ApplyClumpAdapterProperties(
        std::uint32_t object, IGobClumpAdapterRuntime& runtime);
    [[nodiscard]] static std::uint32_t SetClumpRenderCallback(
        std::uint32_t object, IGobSoundConfigRuntime& runtime);
    [[nodiscard]] static std::uint8_t StopPlaybackAndSuppress(
        std::uint32_t object, IGobSoundPlaybackRuntime& runtime);
    [[nodiscard]] static std::uint8_t ResumePlaybackAndUnsuppress(
        std::uint32_t object, IGobSoundPlaybackRuntime& runtime);
    [[nodiscard]] static std::uint32_t GetConfigNode(
        std::uint32_t object, IGobSoundConfigRuntime& runtime);
    [[nodiscard]] static std::uint32_t ReadConfigAndResetAudio(
        std::uint32_t object, IGobSoundConfigRuntime& runtime);
    [[nodiscard]] static std::uint32_t ApplyConfigValues(std::uint32_t object,
        std::uint32_t node, IGobSoundConfigValueRuntime& runtime);
    [[nodiscard]] static std::uint8_t SetVisible(
        std::uint32_t object, std::uint32_t visible,
        IGobSoundRuntime& runtime);
    [[nodiscard]] static std::uint32_t CreateStateSnapshot(
        std::uint32_t object, IGobSoundSnapshotCaptureRuntime& runtime);
};

}  // namespace pal4
