#pragma once

#include "pal4/smith_window.h"
#include "pal4/smith_window_factory.h"

namespace pal4 {

class SmithWindowFactoryRuntime final : public ISmithWindowFactoryRuntime {
public:
    SmithWindowFactoryRuntime(ISmithWindowFactoryRuntime& allocation,
                              ISmithWindowRuntime& window)
        : allocation_(allocation), window_(window) {}

    [[nodiscard]] std::uint32_t Allocate(std::uint32_t bytes) override;
    [[nodiscard]] std::uint32_t Construct(
        std::uint32_t object, std::uint32_t layout_name) override;
    void Free(std::uint32_t object) override;

private:
    ISmithWindowFactoryRuntime& allocation_;
    ISmithWindowRuntime& window_;
};

}  // namespace pal4
