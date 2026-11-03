#pragma once

#include "pal4/magic_window_stunt_image_updater.h"

#include <cstdint>

namespace pal4 {

class IMagicWindowCostRuntime {
public:
    virtual ~IMagicWindowCostRuntime() = default;
    virtual void AdjustCounter(std::int32_t delta) = 0;
    virtual void RemoveItem(std::uint32_t item_id, std::int32_t count) = 0;
};

class MagicWindowCostRuntimeStub : public IMagicWindowCostRuntime {
public:
    void AdjustCounter(std::int32_t) override {}
    void RemoveItem(std::uint32_t, std::int32_t) override {}
};

class MagicWindowCostProcessor final {
public:
    static void Apply(const MagicDbStuntRecord& record,
                      IMagicWindowCostRuntime& runtime);
};

}  // namespace pal4
