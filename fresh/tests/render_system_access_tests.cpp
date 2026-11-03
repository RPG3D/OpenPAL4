#include "pal4/render_system_access.h"

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

class Runtime final : public pal4::IRenderSystemAccessRuntime {
public:
    std::uint32_t GetSingletonManager() override {
        events.emplace_back("manager");
        return 0x1234u;
    }
    std::uint32_t Register(
        const std::uint32_t manager, const std::string_view type_name,
        const std::uint32_t id, const std::uint32_t factory,
        const std::uint32_t destructor) override {
        events.emplace_back("register");
        Check(manager == 0x1234u && type_name == "rwcWindows" &&
                  id == 0x14u && factory == 0x409950u &&
                  destructor == 0x4099b0u,
              "rwcWindows singleton registration arguments");
        return result;
    }
    std::uint32_t result = 0x9000u;
    std::vector<std::string> events;
};

}  // namespace

int main() {
    Check(pal4::RenderSystemAccess::kGuardAddress == 0x8e3de0u &&
              pal4::RenderSystemAccess::kInstanceAddress == 0x8e3ddcu,
          "observed guard and cached-instance globals");
    pal4::RenderSystemAccessState state{0x80u, 0u};
    Runtime runtime;
    Check(pal4::RenderSystemAccess::Get(state, runtime) == 0x9000u &&
              state.guard == 0x81u && state.instance == 0x9000u &&
              runtime.events ==
                  std::vector<std::string>{"manager", "register"},
          "first access sets bit and registers exactly once");
    runtime.events.clear();
    Check(pal4::RenderSystemAccess::Get(state, runtime) == 0x9000u &&
              runtime.events.empty(),
          "guarded access returns cached instance without runtime calls");
    state = {};
    runtime.result = 0u;
    Check(pal4::RenderSystemAccess::Get(state, runtime) == 0u &&
              state.guard == 1u,
          "null registration still sets guard");
    runtime.events.clear();
    runtime.result = 0xaaaau;
    Check(pal4::RenderSystemAccess::Get(state, runtime) == 0u &&
              runtime.events.empty(),
          "failed registration is not retried implicitly");
    std::cout << "4 render system access paths passed\n";
}
