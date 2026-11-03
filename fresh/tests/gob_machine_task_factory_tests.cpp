#include "pal4/gob_machine_task_factory.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IGobMachineTaskFactoryRuntime {
    std::uint32_t address = 0u;
    std::uint32_t vtable = 0u;
    int writes = 0;
    void WriteWord(std::uint32_t target,
                   std::uint32_t value) override {
        address = target;
        vtable = value;
        ++writes;
    }
};

struct CreationRuntime final : pal4::IGobMachineTaskCreationRuntime {
    std::uint32_t size = 0u;
    std::uint32_t allocation = 0x3000u;
    std::uint32_t constructed = 0u;
    pal4::GobMachineTaskKind kind = pal4::GobMachineTaskKind::Script;
    bool throws = false;
    std::vector<std::string> calls;

    std::uint32_t Allocate(std::uint32_t requested) override {
        size = requested;
        calls.emplace_back("allocate");
        return allocation;
    }
    void Free(std::uint32_t address) override {
        Check(address == 0x3000u, "factory freed address");
        calls.emplace_back("free");
    }
    std::uint32_t ConstructTask(pal4::GobMachineTaskKind requested,
                                std::uint32_t address) override {
        kind = requested;
        constructed = address;
        calls.emplace_back("construct");
        if (throws) throw std::runtime_error("construct failed");
        return address + 4u;
    }
};

using Factory = pal4::GobMachineTaskFactory;
using Constructor = std::uint32_t (*)(
    std::uint32_t, pal4::IGobMachineTaskFactoryRuntime&);

struct Case {
    Constructor construct;
    std::uint32_t vtable;
};

constexpr Case cases[] = {
    {Factory::ConstructScript, 0x849018u},
    {Factory::ConstructRollback, 0x849014u},
    {Factory::ConstructGetItem, 0x849010u},
    {Factory::ConstructCamera, 0x84900cu},
    {Factory::ConstructCameraAutoSeek, 0x849008u},
    {Factory::ConstructLockControl, 0x849004u},
    {Factory::ConstructUnlockControl, 0x849000u},
    {Factory::ConstructActiveObject, 0x848ffcu},
    {Factory::ConstructTouchOther, 0x848ff8u},
};

using Create = std::uint32_t (*)(pal4::IGobMachineTaskCreationRuntime&);
struct CreationCase {
    Create create;
    std::uint32_t size;
    pal4::GobMachineTaskKind kind;
};
constexpr CreationCase creation_cases[] = {
    {Factory::CreateScript, 0x5cu, pal4::GobMachineTaskKind::Script},
    {Factory::CreateRollback, 0x44u, pal4::GobMachineTaskKind::Rollback},
    {Factory::CreateGetItem, 0x54u, pal4::GobMachineTaskKind::GetItem},
    {Factory::CreateCamera, 0x64u, pal4::GobMachineTaskKind::Camera},
    {Factory::CreateCameraAutoSeek, 0x48u,
        pal4::GobMachineTaskKind::CameraAutoSeek},
    {Factory::CreateLockControl, 0x48u,
        pal4::GobMachineTaskKind::LockControl},
    {Factory::CreateUnlockControl, 0x48u,
        pal4::GobMachineTaskKind::UnlockControl},
    {Factory::CreateActiveObject, 0x5cu,
        pal4::GobMachineTaskKind::ActiveObject},
    {Factory::CreateTouchOther, 0x5cu,
        pal4::GobMachineTaskKind::TouchOther},
};

}  // namespace

int main() {
    try {
        for (const auto& item : cases) {
            Runtime runtime;
            Check(item.construct(0x2000u, runtime) == 0x2000u &&
                  runtime.address == 0x2000u &&
                  runtime.vtable == item.vtable &&
                  runtime.writes == 1,
                  "factory constructor writes exact vtable once");
        }
        for (const auto& item : creation_cases) {
            CreationRuntime runtime;
            Check(item.create(runtime) == 0x3004u &&
                  runtime.size == item.size &&
                  runtime.kind == item.kind &&
                  runtime.constructed == 0x3000u &&
                  runtime.calls ==
                      (std::vector<std::string>{"allocate", "construct"}),
                  "task creation size, constructor and return");
            CreationRuntime failed;
            failed.allocation = 0u;
            Check(item.create(failed) == 0u &&
                  failed.size == item.size &&
                  failed.calls ==
                      (std::vector<std::string>{"allocate"}),
                  "task creation allocation failure");
        }
        CreationRuntime throwing;
        throwing.throws = true;
        try {
            (void)Factory::CreateScript(throwing);
            throw std::runtime_error("missing constructor exception");
        } catch (const std::runtime_error& error) {
            Check(std::string(error.what()) == "construct failed" &&
                  throwing.calls == (std::vector<std::string>{
                      "allocate", "construct", "free"}),
                  "constructor exception frees allocation");
        }
        std::cout << "gob_machine_task_factory_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
