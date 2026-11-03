#include "pal4/pal_actor_control_named_member.h"

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(1);
    }
}

struct Runtime final : pal4::IActorControlNamedMemberRuntime {
    std::vector<std::string> calls;
    void ResetBaseVtable(std::uint32_t object) override {
        Check(object == 0x432100u, "named member base reset address");
        calls.emplace_back("reset");
    }
    void Free(std::uint32_t object) override {
        Check(object == 0x432100u, "named member free address");
        calls.emplace_back("free");
    }
    void CallFirstMember(std::uint32_t member) override {
        Check(member == 0x123400u, "first dispatch forwards member pointer");
        calls.emplace_back("first");
    }
    void ResetMemberDisplay(std::uint32_t member) override {
        Check(member == 0x123400u, "second dispatch forwards member pointer");
        calls.emplace_back("second");
    }
};

}  // namespace

int main() {
    const auto* first = pal4::PalActorControlNamedMember::FirstTag();
    const auto* second = pal4::PalActorControlNamedMember::SecondTag();
    Check(std::strcmp(first, "jl") == 0 &&
              std::strcmp(second, "jo") == 0 &&
              pal4::PalActorControlNamedMember::FirstTag() == first &&
              pal4::PalActorControlNamedMember::SecondTag() == second &&
              pal4::PalActorControlNamedMember::kFirstVtable == 0x842660u &&
              pal4::PalActorControlNamedMember::kSecondVtable == 0x842654u,
          "both named-member vtable tags are stable constants");

    Runtime runtime;
    Check(pal4::PalActorControlNamedMember::Delete(
              0x432100u, 2u, runtime) == 0x432100u &&
              runtime.calls == std::vector<std::string>{"reset"},
          "named member destructor ignores non-low flag bits");
    runtime.calls.clear();
    Check(pal4::PalActorControlNamedMember::Delete(
              0x432100u, 3u, runtime) == 0x432100u &&
              runtime.calls == std::vector<std::string>{"reset", "free"},
          "named member destructor resets base before conditional free");

    pal4::PalActorControlNamedMemberPrefix object{
        pal4::PalActorControlNamedMember::kFirstVtable, 0u};
    runtime.calls.clear();
    Check(pal4::PalActorControlNamedMember::DispatchFirst(
              object, 1u, 2u, runtime) == 0u &&
              pal4::PalActorControlNamedMember::DispatchSecond(
                  object, 3u, 4u, runtime) == 0u &&
              runtime.calls.empty(),
          "null member suppresses both vtable handlers");
    object.member = 0x123400u;
    Check(pal4::PalActorControlNamedMember::DispatchFirst(
              object, 0xffffffffu, 0u, runtime) == 1u &&
              pal4::PalActorControlNamedMember::DispatchSecond(
                  object, 0u, 0xffffffffu, runtime) == 1u &&
              runtime.calls == std::vector<std::string>{"first", "second"} &&
              object.member == 0x123400u,
          "non-null member invokes both handlers and ignores event arguments");
}
