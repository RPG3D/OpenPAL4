#pragma once

#include "pal4/script_sound_2d.h"
#include "pal4/native_effect_sound.h"

#include <cstdint>
#include <filesystem>
#include <functional>
#include <memory>
#include <map>
#include <string>
#include <string_view>
#include <unordered_map>
#include <array>
#include <vector>

namespace pal4 {

class NativeMilesDigitalDriver;

class NativeMilesSound2dRuntime final : public IScriptSound2dRuntime, public INativeEffectSound {
public:
    using SoundNameResolver =
        std::function<std::string_view(std::uint32_t)>;

    NativeMilesSound2dRuntime(std::filesystem::path game_root,
                              NativeMilesDigitalDriver& driver,
                              SoundNameResolver sound_name = {});
    ~NativeMilesSound2dRuntime() override;
    NativeMilesSound2dRuntime(const NativeMilesSound2dRuntime&) = delete;
    NativeMilesSound2dRuntime& operator=(const NativeMilesSound2dRuntime&) = delete;

    [[nodiscard]] std::string_view SoundName(std::uint32_t source) override;
    [[nodiscard]] std::uint32_t AudioManager() override;
    [[nodiscard]] std::uint32_t PlaySound(
        std::uint32_t audio, std::uint32_t source,
        std::string_view path, std::int32_t mode) override;
    // Native UI callers already have a filename, without an AngelScript string
    // object or script wait. Samples share the same driver and owned lifetime.
    [[nodiscard]] std::uint32_t PlayNamedSound(std::string_view filename,
                                              std::int32_t mode);
    [[nodiscard]] std::uint32_t ScriptExecutor() override;
    [[nodiscard]] std::int32_t InsertSoundId(
        std::uint32_t executor, std::uint32_t handle) override;
    void SetCurrentHandle(std::uint32_t address,
                          std::uint32_t handle) override;
    [[nodiscard]] std::uint32_t CheckActive(
        std::uint32_t handle_address) override;
    [[nodiscard]] std::uint32_t StopCurrent(
        std::uint32_t handle_address) override;
    [[nodiscard]] std::uint32_t RemoveSoundId(
        std::uint32_t executor, std::int32_t id) override;

    // Direct UI/native callers own AudioHandles, not script registry IDs.
    [[nodiscard]] bool StopSample(std::uint32_t handle) noexcept;
    std::uint32_t Play(std::string_view path,bool spatial,bool once,
        const std::array<float,3>&,float max_distance,float min_distance) override;
    bool Exists(std::uint32_t h) override {return samples_.contains(h);}
    bool Playing(std::uint32_t h) override {return IsPlaying(h);}
    bool Stop(std::uint32_t h) override {return StopSample(h);}
    bool SetPosition(std::uint32_t,const std::array<float,3>&) override;
    void Update();
    [[nodiscard]] bool IsPlaying(std::uint32_t handle) const;
    void StopAll() noexcept;
    // IDA6009F0: stop registered detached sounds and reset script ID+56.
    // UI/current samples and script execution contexts have separate owners.
    void ResetScriptData() noexcept;
    [[nodiscard]] std::size_t ScriptSoundCount() const { return script_sounds_.size(); }
    void SetVolume(float value);
    float Volume() const noexcept { return volume_; }
    [[nodiscard]] std::array<float, 2> VolumeLevels(std::uint32_t handle) const;
    [[nodiscard]] std::uint32_t CurrentHandle() const { return current_handle_; }
    [[nodiscard]] std::size_t ActiveCount() const { return samples_.size(); }
    [[nodiscard]] std::vector<std::uint32_t> SampleHandles() const;
    [[nodiscard]] const std::string& LastError() const { return last_error_; }

private:
    struct Sample;
    [[nodiscard]] std::uint32_t Status(std::uint32_t id) const;
    void Release(std::uint32_t id) noexcept;

    std::filesystem::path game_root_;
    NativeMilesDigitalDriver& driver_;
    SoundNameResolver sound_name_;
    std::unordered_map<std::uint32_t, std::unique_ptr<Sample>> samples_;
    std::uint32_t next_id_{1u};
    std::map<std::int32_t, std::uint32_t> script_sounds_;
    std::uint32_t next_script_id_{};
    std::uint32_t current_handle_{};
    std::string last_error_;
    void* allocate_sample_{};
    void* release_sample_{};
    void* init_sample_{};
    void* set_sample_file_{};
    void* set_sample_volume_{};
    void* set_sample_loop_{};
    void* start_sample_{};
    void* sample_status_{};
    void* end_sample_{};
    void* read_sample_volume_{};
    float volume_ = 1.0f;
};

}  // namespace pal4
