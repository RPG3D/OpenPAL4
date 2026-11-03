#pragma once
#include <cstdint>
#include <memory>
namespace pal4 {
class NativeBaseObjectMemory;
class NativeScriptObjectRuntime;
struct NativeGobMachineRewardServices;
struct GobMachineTaskDefinition;
// Task7 property /8 equipment /9 money share original GetItemTask and Hint wait.
class NativeGobMachineRewardTasks final {
public:
    NativeGobMachineRewardTasks(NativeBaseObjectMemory&,NativeScriptObjectRuntime&,
        NativeGobMachineRewardServices&);
    ~NativeGobMachineRewardTasks();
    std::uint32_t Construct();
    void Initialize(std::uint32_t,std::uint32_t owner,std::uint32_t context,
                    const GobMachineTaskDefinition&);
    void Destruct(std::uint32_t);
    void Begin(std::uint32_t);
    void Tick(std::uint32_t);
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};
}
