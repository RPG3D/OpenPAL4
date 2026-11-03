#pragma once
#include <array>
#include <cstdint>
#include <vector>
namespace pal4 {
class NativeSceneEvfAssets;
// Read-only selection boundary over the scene's owned original EVF records.
class NativeSceneEvfSelection final {
public:
    explicit NativeSceneEvfSelection(NativeSceneEvfAssets& assets):assets_(assets){}
    std::vector<std::uint32_t> Records();
    std::array<float,3> PropertyPoint(std::uint32_t record,std::int32_t index);
private:
    NativeSceneEvfAssets& assets_;
};
}
