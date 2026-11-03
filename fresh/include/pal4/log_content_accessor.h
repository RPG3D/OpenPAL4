#pragma once

#include <cstdint>

namespace pal4 {

class ILogContentRuntime {
public:
    virtual ~ILogContentRuntime() = default;
    virtual void CopyString(std::uint32_t source,
                            std::uint32_t destination) = 0;
};

class LogContentRuntimeStub final : public ILogContentRuntime {
public:
    void CopyString(std::uint32_t, std::uint32_t) override {}
};

class LogContentAccessor final {
public:
    // IDA 0x600e90.
    [[nodiscard]] static std::uint32_t Get(
        std::uint32_t log, std::uint32_t destination,
        ILogContentRuntime& runtime);
};

}  // namespace pal4
