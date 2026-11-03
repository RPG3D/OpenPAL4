#pragma once
#include <cstdint>
#include <memory>
namespace pal4 {
class NativeGobObjectImage;
class NativeBaseObjectMemory;
class NativeScriptObjectRuntime;
struct NativeGobMachineServices;
struct GobMachineTaskDefinition;
// Script2, Action3, ActiveObject15, TouchOther17, TimeWait19 and CameraQuake20/Terminate21 reuse the
// shared task state machines and borrow existing scene/camera services.
// The Machine owner registers these with its existing task container/lifecycle.
class NativeGobMachineAuxTasks final {
public:
    NativeGobMachineAuxTasks(NativeGobObjectImage&,NativeBaseObjectMemory&,
        NativeScriptObjectRuntime&,NativeGobMachineServices&);
    ~NativeGobMachineAuxTasks();
    std::uint32_t Construct(std::int32_t type);
    void Initialize(std::uint32_t,std::uint32_t,const GobMachineTaskDefinition&);
    void Destruct(std::uint32_t,std::int32_t);
    void Begin(std::uint32_t,std::int32_t);
    void Tick(std::uint32_t,std::int32_t,float elapsed);
    void Reverse(std::uint32_t,std::int32_t);
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};
}
