#pragma once
#include <cstdint>
#include <functional>
#include <string_view>
namespace pal4 {
class NativePrescriptionRecordCatalog;
class NativeInventorySession;
// Original5912A0 primitive. The Smith UI supplies selection/material gates.
// Uses the public host CRT srand/rand stream, reseeded from time per attempt.
class NativeCraftingStrengthening final {
public:
    using SoundPlayer=std::function<void(std::string_view,std::uint32_t)>;
    using Clock=std::function<std::uint32_t()>;
    NativeCraftingStrengthening(const NativePrescriptionRecordCatalog&,NativeInventorySession&,SoundPlayer,Clock={});
    std::int32_t Apply(std::uint32_t recipe,std::uint32_t target_sequence,bool opened_from_system);
private:
    const NativePrescriptionRecordCatalog& recipes_;
    NativeInventorySession& inventory_;
    SoundPlayer sound_;
    Clock clock_;
};
}
