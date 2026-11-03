#include "pal4/d3dx_shader_state_block_update.h"

namespace pal4 {

std::int32_t D3DXShaderStateBlockUpdate::Run(
    const std::uint32_t device,
    const std::uint8_t owner_flags,
    std::uint32_t& saved_state_block,
    ID3DXShaderStateBlockRuntime& runtime,
    ID3DXShaderStateBlockOperation& operation) {
    auto first_state_block = std::uint32_t{0};
    auto second_state_block = std::uint32_t{0};
    auto completed_recordings = std::uint32_t{0};
    auto status = std::int32_t{0};

    if ((owner_flags & 2u) == 0) {
        while (completed_recordings < 2) {
            status = runtime.BeginStateBlock(device);
            if (status < 0) break;

            runtime.SetDebugMute(true);
            status = operation.Submit();
            runtime.SetDebugMute(false);
            if (status < 0) {
                auto& partial_state_block = completed_recordings == 0
                    ? first_state_block : second_state_block;
                (void)runtime.EndStateBlock(device, partial_state_block);
                break;
            }

            auto& recorded_state_block = completed_recordings == 0
                ? first_state_block : second_state_block;
            status = runtime.EndStateBlock(device, recorded_state_block);
            if (status < 0) break;
            ++completed_recordings;
        }
    }

    if ((owner_flags & 2u) != 0 || completed_recordings == 2) {
        status = runtime.CaptureStateBlock(first_state_block);
        if (status >= 0) {
            status = saved_state_block != 0
                ? runtime.ApplyStateBlock(saved_state_block) : 0;
            if (status >= 0) {
                status = runtime.CaptureStateBlock(second_state_block);
                if (status >= 0) {
                    status = runtime.ApplyStateBlock(first_state_block);
                    if (status >= 0) {
                        if (saved_state_block != 0)
                            runtime.ReleaseStateBlock(saved_state_block);
                        saved_state_block = second_state_block;
                        second_state_block = 0;
                    }
                }
            }
        }
    }

    if (first_state_block != 0)
        runtime.ReleaseStateBlock(first_state_block);
    if (second_state_block != 0)
        runtime.ReleaseStateBlock(second_state_block);
    return status;
}

}  // namespace pal4
