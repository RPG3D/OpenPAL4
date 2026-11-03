#pragma once
#include <cstdint>

namespace pal4 {
struct PalActorControlState;
class NativeGameSaveArchiveBuffer;

class NativeActorControlConfigSaveRuntime final {
public:
    // IDA427010: append the original final Actor-Controller document.
    // State is borrowed read-only; the caller retains the continuous archive.
    [[nodiscard]] static std::uint8_t Serialize(
        const PalActorControlState&, NativeGameSaveArchiveBuffer&);
};
} // namespace pal4
