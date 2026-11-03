#pragma once

#include <utility>

namespace pal4 {

class IRenderWareVector2 {
public:
    virtual ~IRenderWareVector2() = default;
    [[nodiscard]] virtual double Length(float x, float y) = 0;
    [[nodiscard]] virtual std::pair<float, float> Normalize(
        float x, float y) = 0;
};

}  // namespace pal4
