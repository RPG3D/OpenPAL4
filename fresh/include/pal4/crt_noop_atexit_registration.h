#pragma once

#include <cstdint>

namespace pal4 {

class ICrtNoopAtExitRuntime {
public:
    virtual ~ICrtNoopAtExitRuntime() = default;
    [[nodiscard]] virtual int RegisterAtExit(
        std::uint32_t callback) = 0;
};

class CrtNoopAtExitRuntime : public ICrtNoopAtExitRuntime {
public:
    int RegisterAtExit(std::uint32_t callback) override;
};

using CrtNoopAtExitRuntimeStub = CrtNoopAtExitRuntime;

class CrtNoopAtExitRegistration final {
public:
    static constexpr std::uint32_t kCallback = 0x5e0560u;

    [[nodiscard]] static int Run(std::uint8_t& flag,
                                 ICrtNoopAtExitRuntime& runtime);
};

}  // namespace pal4
