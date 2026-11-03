#pragma once

#include <cstdint>

namespace pal4 {
struct PalActorControlState;
class NativeGameSaveArchiveReader;
class IActorControlConfigInitRuntime;

class NativeActorControlConfigLoadRuntime final {
public:
    // IDA427160: XML input is native; all scene/input/audio operations are
    // forwarded to the required borrowed service runtime, never default stubs.
    [[nodiscard]] static std::uint8_t Deserialize(
        PalActorControlState& control, NativeGameSaveArchiveReader& archive,
        IActorControlConfigInitRuntime& scene);
};
}  // namespace pal4
