#include "pal4/world_map_frame.h"

#include <array>

namespace pal4 {
namespace {

constexpr std::array<std::string_view, WorldMapFrame::kNameCount> kLocations{
    "M05", "Q05", "M15", "M03", "Q13", "M17", "M13",
    "Q09", "Q08", "Q01", "Q06", "M14", "Q03", "Q02",
    "M09", "Q07", "M11", "M02", "Q04", "M19", "Q10"};

}  // namespace

std::uint32_t WorldMapFrame::Construct(
    const std::uint32_t frame, const std::uint32_t name,
    IWorldMapFrameRuntime& runtime) {
    runtime.LoadLayout(frame, name);
    try {
        runtime.ConstructMouse(frame + kMouseOffset);
        try {
            runtime.WriteWord(frame + kMouseOffset, 0x843628u);
            runtime.ConstructNames(frame + kNamesOffset, kNameCount);
            try {
                const auto fly_name = frame + kFlyNameOffset;
                runtime.WriteByte(fly_name, runtime.ReadByte(name));
                runtime.ClearNameFlag(fly_name);
                runtime.AssignText(fly_name, "WorldMapFly");
                try {
                    runtime.WriteWord(frame + 0x268u, kFlySpeedBits);
                    runtime.WriteWord(frame, kFrameVtable);
                    runtime.WriteWord(frame + kMouseOffset, kMouseVtable);
                    runtime.InitializeEventHandlers(frame);
                    for (std::uint32_t i = 0; i < kNameCount; ++i) {
                        const auto address = frame + kNamesOffset +
                                             i * kNameStride;
                        const auto text = kLocations[i];
                        if (i < 5u) {
                            runtime.AssignText(address, text);
                        } else if (runtime.AllocateText(address,
                                                        static_cast<std::uint32_t>(text.size()))) {
                            runtime.CopyText(address, text);
                            runtime.SetTextSize(address,
                                                static_cast<std::uint32_t>(text.size()));
                        }
                    }
                } catch (...) {
                    runtime.DestroyText(fly_name);
                    throw;
                }
            } catch (...) {
                runtime.DestroyNames(frame + kNamesOffset, kNameCount);
                throw;
            }
        } catch (...) {
            runtime.DestroyMouse(frame + kMouseOffset);
            throw;
        }
    } catch (...) {
        runtime.DestroyBase(frame);
        throw;
    }
    return frame;
}

std::uint32_t WorldMapFrame::Destroy(
    const std::uint32_t frame,
    IWorldMapFrameDestructorRuntime& runtime) {
    const auto text = runtime.ReadWord(frame + kFlyNameOffset + 4u);
    if (text != 0u) {
        const auto count_address = text - 1u;
        const auto count = runtime.ReadByte(count_address);
        if (count == 0u || count == 0xffu) {
            runtime.Free(count_address);
        } else {
            runtime.WriteByte(count_address,
                              static_cast<std::uint8_t>(count - 1u));
        }
    }
    for (std::uint32_t offset = 4u; offset <= 12u; offset += 4u) {
        runtime.WriteWord(frame + kFlyNameOffset + offset, 0u);
    }
    runtime.DestroyNames(frame + kNamesOffset, kNameCount);

    const auto mouse = frame + kMouseOffset;
    runtime.Free(runtime.ReadWord(mouse + 8u));
    for (std::uint32_t offset = 8u; offset <= 16u; offset += 4u) {
        runtime.WriteWord(mouse + offset, 0u);
    }
    return runtime.DestroyBase(frame);
}

}  // namespace pal4
