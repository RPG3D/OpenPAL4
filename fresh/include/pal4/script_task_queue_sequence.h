#pragma once
#include <cstdint>
namespace pal4 {
// Opaque sequence handles. Native hosts use standard containers; the legacy
// memory adapter retains the original ABI for existing game callers/tests.
class IScriptTaskQueueSequence {
public:
    virtual ~IScriptTaskQueueSequence()=default;
    virtual std::uint32_t End()=0;
    virtual std::uint32_t First()=0;
    virtual std::uint32_t Next(std::uint32_t entry)=0;
    virtual std::uint32_t Task(std::uint32_t entry)=0;
    virtual bool AppendOwned(std::uint32_t task)=0;
    virtual std::uint32_t EraseOwned(std::uint32_t entry)=0;
};
}
