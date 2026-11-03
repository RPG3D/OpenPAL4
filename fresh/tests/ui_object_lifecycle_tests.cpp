#include "pal4/ui_object_lifecycle.h"

#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

struct Runtime final : pal4::UiObjectLifecycleRuntimeStub {
    std::vector<std::string> calls;
    bool throw_member = false;
    bool throw_elements = false;
    std::uint32_t freed = 0u;

    void LoadLayout(pal4::UiObjectState&, std::uint32_t layout) override {
        Check(layout == 0x8800u, "layout argument");
        calls.emplace_back("layout");
    }
    void ConstructMember(pal4::UiObjectState&) override {
        calls.emplace_back("member");
        if (throw_member) throw std::runtime_error("member failure");
    }
    void InitializeElements(pal4::UiObjectState& state) override {
        Check(state.Read(0u) == pal4::UiObjectState::kVtable &&
                  state.Read(0x28u) == pal4::UiObjectState::kMemberVtable &&
                  state.Read(0x54u) == pal4::UiObjectState::kTailVtable,
              "vtables precede element initialization");
        calls.emplace_back("elements");
        if (throw_elements) throw std::runtime_error("element failure");
    }
    void DestroyMember(pal4::UiObjectState&) override {
        calls.emplace_back("destroy-member");
    }
    void DestroyLayout(pal4::UiObjectState&) override {
        calls.emplace_back("destroy-layout");
    }
    void FreeMemberBuffer(std::uint32_t buffer) override {
        freed = buffer;
        calls.emplace_back("free");
    }
};

void ConstructAndCleanup() {
    pal4::UiObjectState state;
    state.Write(0x58u, 0x1234u);
    Runtime runtime;
    Check(&pal4::UiObjectLifecycle::Construct(state, 0x8800u, runtime) ==
              &state &&
              runtime.calls == std::vector<std::string>{
                  "layout", "member", "elements"} &&
              state.Read(0x58u) == 0x1234u,
          "constructor preserves adjacent state and call order");
    state.Write(0x30u, 0x7000u);
    state.Write(0x34u, 0x7100u);
    state.Write(0x38u, 0x7200u);
    pal4::UiObjectLifecycle::CleanupMember(state, runtime);
    Check(runtime.freed == 0x7000u && state.Read(0x30u) == 0u &&
              state.Read(0x34u) == 0u && state.Read(0x38u) == 0u &&
              state.Read(0x58u) == 0x1234u,
          "cleanup frees member buffer and clears its three words");
}

void ConstructorUnwind() {
    pal4::UiObjectState state;
    Runtime member;
    member.throw_member = true;
    try {
        (void)pal4::UiObjectLifecycle::Construct(state, 0x8800u, member);
        Check(false, "member constructor should throw");
    } catch (const std::runtime_error&) {}
    Check(member.calls == std::vector<std::string>{
              "layout", "member", "destroy-layout"},
          "member failure releases layout");

    Runtime elements;
    elements.throw_elements = true;
    try {
        (void)pal4::UiObjectLifecycle::Construct(state, 0x8800u, elements);
        Check(false, "elements constructor should throw");
    } catch (const std::runtime_error&) {}
    Check(elements.calls == std::vector<std::string>{
              "layout", "member", "elements",
              "destroy-member", "destroy-layout"},
          "element failure unwinds member then layout");
}

}  // namespace

int main() {
    try {
        ConstructAndCleanup();
        ConstructorUnwind();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
