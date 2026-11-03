#pragma once

#include <cstdint>
#include <string_view>

namespace pal4 {

// Position/rotation create already-closed nonblocking handlers. They still
// participate in the creating context's ordered wait dispatch (IDA 7DF4B0).
class IScriptCompletedHandlerQueue {
public:
    virtual ~IScriptCompletedHandlerQueue() = default;
    virtual void AppendCompletedHandler(std::uint32_t vtable,
        std::int32_t marker, std::string_view name) = 0;
};

}  // namespace pal4
