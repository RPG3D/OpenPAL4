#include "pal4/gom_task_container.h"
#include "pal4/native_base_object_memory.h"
#include "pal4/native_script_object_runtime.h"
#include "pal4/script_object.h"

#include <cstdint>
#include <cstdio>
#include <cstdlib>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::fprintf(stderr, "FAIL: %s\n", message);
        std::exit(EXIT_FAILURE);
    }
}

std::uint32_t AddressOf(const void* pointer) {
    return static_cast<std::uint32_t>(
        reinterpret_cast<std::uintptr_t>(pointer));
}

}  // namespace

int main() {
    pal4::NativeBaseObjectMemory memory;
    pal4::NativeScriptObjectRuntime runtime(memory);
    pal4::GomTaskContainerState first{};
    pal4::GomTaskContainerState second{};
    const auto first_address = AddressOf(&first);
    const auto second_address = AddressOf(&second);
    Check(pal4::ScriptObject::Initialize(first_address, runtime) ==
              first_address &&
              first.sentinel != 0u && first.count == 0u &&
              first.ready == 1u &&
              memory.ReadDword(first.sentinel) == first.sentinel &&
              memory.ReadDword(first.sentinel + 4u) == first.sentinel &&
              memory.OutstandingAllocations() == 1u,
          "native NPC script object owns an empty task list");
    Check(pal4::ScriptObject::Initialize(second_address, runtime) ==
              second_address &&
              second.sentinel != first.sentinel &&
              memory.OutstandingAllocations() == 2u,
          "second script object owns a distinct sentinel");
    (void)pal4::ScriptObject::Destroy(first_address, runtime);
    Check(first.sentinel == 0u && first.count == 0u &&
              memory.OutstandingAllocations() == 1u,
          "first script object releases only its task list");
    (void)pal4::ScriptObject::Destroy(second_address, runtime);
    Check(second.sentinel == 0u && second.count == 0u &&
              memory.OutstandingAllocations() == 0u,
          "script object teardown releases all native allocations");
    return EXIT_SUCCESS;
}
