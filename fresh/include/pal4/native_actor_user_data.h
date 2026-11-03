#pragma once

#include <cstdint>
#include <string_view>

struct RpAtomic;
struct RpClump;
struct RpMaterial;
struct RwFrame;

namespace pal4 {

class NativeActorUserData final {
public:
    // Original7B4BD0 shared atomic identity callback, also used by EVF records.
    static RpAtomic* IdentityRender(RpAtomic*) noexcept;
    // Requires PAL4's 8451 Atomic plugin to be attached before engine open.
    [[nodiscard]] static bool HideHooks(RpClump* clump);
    [[nodiscard]] static RpAtomic* FindAtomic(RpClump* clump,
                                              std::string_view name);
    [[nodiscard]] static RpMaterial* FindFirstMaterial(
        RpClump* clump, std::string_view name);
    [[nodiscard]] static std::uint32_t HiddenHookCount(RpClump* clump);
    // Reuse PAL4's 8451 attachment traversal; removal also clears its frame view.
    static void AttachFrame(RpClump* clump, std::string_view hook, RwFrame* frame);
    static void RemoveFrame(RpClump* clump, std::string_view hook, RwFrame* frame);
};

}  // namespace pal4
