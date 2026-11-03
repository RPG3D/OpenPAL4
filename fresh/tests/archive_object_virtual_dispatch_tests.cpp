#include "pal4/archive_object_virtual_dispatch.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <utility>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

class Runtime final : public pal4::IArchiveObjectVirtualRuntime {
public:
    std::int32_t result = -31;
    std::vector<std::pair<std::uint32_t, std::uint32_t>> calls;
    std::int32_t Invoke(std::uint32_t object,
                        std::uint32_t slot_offset) override {
        calls.emplace_back(object, slot_offset);
        return result;
    }
};

void Cases() {
    Runtime runtime;
    pal4::ArchiveObjectVirtualDispatch dispatch(runtime);
    Check(dispatch.ReleaseSlot12(0x1234u) == -31 &&
              runtime.calls == std::vector<std::pair<std::uint32_t,
                                                     std::uint32_t>>{{0x1234u, 12u}},
          "first wrapper forwards object and vtable slot twelve");
    runtime.result = 0x76543210;
    Check(dispatch.ReleaseSlot8(0x5678u) == 0x76543210 &&
              runtime.calls.back() == std::make_pair(0x5678u, 8u),
          "second wrapper forwards object, slot eight and full return");
    pal4::ArchiveObjectVirtualRuntimeStub stub;
    Check(stub.Invoke(0u, 0u) == 0, "virtual runtime stub links");
}

}  // namespace

int main() {
    try {
        Cases();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
