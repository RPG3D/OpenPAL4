#include "pal4/audio_3d_sample.h"

namespace pal4 {

Audio3DSampleState& Audio3DSample::Initialize(
    Audio3DSampleState& sample, const std::uint32_t parent,
    IAudio3DSampleRuntime& runtime) {
    sample.parent = parent;
    sample.vtable = kBaseVtable;
    sample.volume_bits = 0x3f800000u;
    sample.audio_file = 0u;
    sample.sample = 0u;
    sample.loop_count = 1u;
    const auto defaults = runtime.DefaultVector();
    sample.vector24 = defaults;
    sample.vector36 = defaults;
    sample.range60_bits = 1142292480u;
    sample.range64_bits = 1133903872u;
    runtime.InitializePath(sample);
    sample.vtable = k3DVtable;
    sample.vector48 = defaults;
    sample.vector48[2] = 0x3f800000u;
    return sample;
}

std::uint32_t Audio3DSample::DestroyAt(
    Audio3DSampleState& sample, const std::uint32_t address,
    const std::uint8_t flags, IAudio3DSampleRuntime& runtime) {
    (void)Cleanup(sample, runtime);
    if ((flags & 1u) != 0u)
        (void)runtime.FreeObject(address);
    return address;
}

std::uint32_t Audio3DSample::Cleanup(
    Audio3DSampleState& sample, IAudio3DSampleRuntime& runtime) {
    sample.vtable = k3DVtable;
    runtime.Stop(sample);
    if (sample.audio_file != 0u) {
        const auto crc = runtime.FileCrc(sample.audio_file);
        runtime.ReleasePoolFile(crc);
        sample.audio_file = 0u;
    }
    const auto result = runtime.CleanupPath(sample);
    sample.vtable = kBaseVtable;
    return result;
}

}  // namespace pal4
