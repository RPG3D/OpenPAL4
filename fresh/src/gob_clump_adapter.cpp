#include "pal4/gob_sound.h"
#include "pal4/gob_sound_config.h"
namespace pal4 {
std::uint32_t GobSound::ReturnZero() noexcept {return 0;}
std::uint32_t GobSound::ApplyClumpAdapterProperties(
    const std::uint32_t object, IGobClumpAdapterRuntime& runtime) {
    const auto clump = runtime.ReadWord(object + 0x14u);
    const auto adapter = runtime.MovementAdapter();
    const auto render_property = runtime.RenderProperty(object);
    return runtime.SetClumpProperties(adapter, clump,
        runtime.ReadWord(object + 0x188u), 1u, 0u, render_property);
}

}
