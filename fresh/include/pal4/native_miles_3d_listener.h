#pragma once

#include "pal4/audio_manager.h"
#include <array>
#include <functional>
#include <memory>
#include <string>
#include <string_view>

namespace pal4 {
class NativeMilesDigitalDriver;

// Borrows the opened original Miles driver/DLL. Owns its provider/listener;
// close before driver shutdown. Samples/music remain owned by their existing
// services. Uses original public DLL exports, not private middleware code.
class NativeMiles3dListener final {
public:
    explicit NativeMiles3dListener(NativeMilesDigitalDriver&,
        std::function<void(std::uint32_t)> logger = {});
    ~NativeMiles3dListener();
    NativeMiles3dListener(const NativeMiles3dListener&) = delete;
    NativeMiles3dListener& operator=(const NativeMiles3dListener&) = delete;
    bool Open(std::string_view preferred = AudioDriverConfig::kDefaultProvider);
    void SetPosition(const std::array<float,3>&);
    [[nodiscard]] std::array<float,3> Position() const;
    void Close();
    [[nodiscard]] bool Opened() const;
    [[nodiscard]] std::uint32_t Provider() const;
    [[nodiscard]] std::string ProviderName() const;
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};
}  // namespace pal4
