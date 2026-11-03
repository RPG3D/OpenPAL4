#pragma once
#include "pal4/portable/actor.h"
#include <span>
#include <functional>
#include <map>
#include <memory>
#include <string_view>
namespace pal4::portable {
// Node order matches the DFF HAnim hierarchy; values index the DFF frame list.
std::vector<std::uint32_t> ReadDffAnimationHierarchy(std::span<const std::uint8_t> dff);
AnimationClip ReadOriginalAnimation(std::span<const std::uint8_t> anm,std::string_view name,
                                    std::span<const std::uint32_t> hierarchy);
// 调用方负责读取器依赖的生命周期；本对象只弱引用已解析动作，供同一线程使用。
class OriginalAnimationStore {
public:
    OriginalAnimationStore(std::vector<std::uint32_t> hierarchy,std::vector<std::string> names,
                           std::function<std::vector<std::uint8_t>(std::string_view)> read);
    std::shared_ptr<const AnimationClip> Get(std::string_view name);
private:
    std::vector<std::uint32_t> hierarchy_;
    std::map<std::string,std::weak_ptr<const AnimationClip>,std::less<>> clips_;
    std::function<std::vector<std::uint8_t>(std::string_view)> read_;
};
// Replaces the existing clip set atomically, retaining its names and frame contract.
void LoadOriginalAnimations(Actor& actor,std::span<const std::uint8_t> dff,
                            const std::function<std::vector<std::uint8_t>(std::string_view)>& read);
}
