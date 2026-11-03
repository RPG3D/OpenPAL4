#pragma once
#include <array>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <string_view>
namespace pal4 {
class NativeMilesDigitalDriver;
class NativeMiles3dListener;
struct NativeMiles3dSampleObservation {
    std::uint32_t status{},loop_count{};
    std::array<float,3> miles_position{};
    std::array<float,2> distances{};
    float volume{};
};
// Owns each sample and its WAV bytes. Borrowed listener/provider and driver
// must remain open until StopAll/destruction has retired every voice.
class NativeMilesSound3dRuntime final {
public:
    NativeMilesSound3dRuntime(std::filesystem::path,NativeMilesDigitalDriver&,NativeMiles3dListener&);
    ~NativeMilesSound3dRuntime();
    std::uint32_t PlayNamedSound(std::string_view,const std::array<float,3>&,
        std::uint32_t loop_count,float max_distance,float min_distance);
    void Update();
    bool Stop(std::uint32_t);
    bool Exists(std::uint32_t)const;
    bool IsPlaying(std::uint32_t)const;
    bool Suspend(std::uint32_t);
    bool Resume(std::uint32_t);
    void StopAll();
    bool SetPosition(std::uint32_t,const std::array<float,3>&);
    void SetVolume(float);
    std::size_t ActiveCount()const noexcept;
    NativeMiles3dSampleObservation Inspect(std::uint32_t)const;
private:
    class Impl;std::unique_ptr<Impl> impl_;
};
}
