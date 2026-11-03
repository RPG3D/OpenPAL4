#pragma once
#include "pal4/portable/scene.h"
#include <memory>
#include <span>

namespace pal4::portable {
// CPU-only library adapter. Owns decoded animation data, never an extracted file.
class UvAnimationDictionary {
public:
    explicit UvAnimationDictionary(std::span<const std::uint8_t> bytes);
    ~UvAnimationDictionary();
    UvAnimationDictionary(const UvAnimationDictionary&)=delete;
    UvAnimationDictionary& operator=(const UvAnimationDictionary&)=delete;
    // Return one appearance per original draw, preserving material colors.
    // Missing bound animations fail explicitly instead of substituting a dummy.
    std::vector<DrawAppearance> Evaluate(const Scene& scene,float seconds) const;
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
}
