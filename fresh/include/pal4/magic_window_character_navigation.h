#pragma once

#include "pal4/magic_window.h"

#include <cstdint>

namespace pal4 {

class IMagicWindowCharacterNavigationRuntime {
public:
    virtual ~IMagicWindowCharacterNavigationRuntime() = default;
    virtual std::int32_t PreviousIndex(std::int32_t current) = 0;
    virtual std::int32_t NextIndex(std::int32_t current) = 0;
    virtual bool HasActivePlayer(std::int32_t index) = 0;
    virtual void UpdateCharacterDisplay(std::uint32_t object,
                                        std::int32_t index) = 0;
};

class MagicWindowCharacterNavigationRuntimeStub :
    public IMagicWindowCharacterNavigationRuntime {
public:
    std::int32_t PreviousIndex(std::int32_t current) override { return current; }
    std::int32_t NextIndex(std::int32_t current) override { return current; }
    bool HasActivePlayer(std::int32_t) override { return false; }
    void UpdateCharacterDisplay(std::uint32_t, std::int32_t) override {}
};

class MagicWindowCharacterNavigation final {
public:
    static std::int32_t NextCharacterIndex(std::int32_t current) noexcept;
    static bool Previous(std::uint32_t object, MagicWindowState& state,
                         IMagicWindowCharacterNavigationRuntime& runtime);
    static bool Next(std::uint32_t object, MagicWindowState& state,
                     IMagicWindowCharacterNavigationRuntime& runtime);
};

}  // namespace pal4
