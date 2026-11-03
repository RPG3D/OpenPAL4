#pragma once
#include "pal4/gom_task_base.h"
#include <cstdint>
namespace pal4 {
// Borrowed by task adapters; the Machine owns registration and both containers.
class INativeGobMachineTaskSpawns {
public:
    virtual ~INativeGobMachineTaskSpawns()=default;
    virtual void AppendReverse(std::uint32_t task,std::int32_t kind,
        std::uint32_t context,std::uint32_t timing)=0;
    virtual std::uint32_t ConstructHelper(std::uint32_t memory)=0;
    virtual void AppendHelper(std::uint32_t context,std::uint32_t task,
        const GomTaskTimingConfig& timing)=0;
    virtual void SetHelperOwner(std::uint32_t task,std::uint32_t owner)=0;
    virtual void InsertOwnerTask(std::uint32_t collection,std::uint32_t task)=0;
    virtual void Reverse(std::uint32_t task)=0;
    virtual void BeginContext(std::uint32_t context)=0;
};
}
