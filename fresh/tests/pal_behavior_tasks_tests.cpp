#include "pal4/pal_behavior_tasks.h"

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IPalBehaviorTaskRuntime {
    pal4::PalBehaviorTaskState state;
    std::vector<std::string> calls;
    std::uint32_t allocation = 0x4000u;
    std::uint32_t bound = 0x1004u;
    std::uint32_t constructed = 0x5000u;
    std::uint32_t expected_type = 0u;
    std::uint32_t expected_bytes = 0u;
    std::uint8_t flag = 1u;
    bool throw_base = false;
    bool throw_object = false;
    bool duration_complete = true;

    void ConstructBase(pal4::PalBehaviorTaskState& target) override {
        calls.emplace_back("base_construct");
        if (throw_base) throw std::runtime_error("base");
        Check(target.words[0] == 0xaaaaaaaau,
              "base construction precedes derived vtable write");
        target.words[1] = 0x12345678u;
    }
    void DestructBase(pal4::PalBehaviorTaskState& target) override {
        calls.emplace_back("base_destruct");
        Check(target.words[0] == pal4::PalBehaviorTaskBase::kVtable &&
                  target.words[15] == 0u && target.words[16] == 0u,
              "base destructor sees cleared fields and base vtable");
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        calls.emplace_back("read_flag");
        Check(address == 0x50b8u, "owned flag is at object plus 0xb8");
        return flag;
    }
    void WriteByte(std::uint32_t, std::uint8_t) override {
        Check(false, "class zero and four never write helper flag");
    }
    void ReleaseOwned(std::uint32_t owner, std::uint32_t object) override {
        calls.emplace_back("release");
        Check(owner == 0x1000u && object == 0x5000u,
              "owner callback receives stored object");
    }
    void Free(std::uint32_t address) override {
        calls.emplace_back("free");
        Check(address == 0x4000u || address == 0x2000u,
              "only allocated or deleted address is freed");
    }
    std::uint32_t Bind(std::uint32_t binder,
                       std::uint32_t& type_id) override {
        calls.emplace_back("bind");
        Check(binder == 0x6000u && type_id == expected_type,
              "binding receives class-specific type ID");
        type_id = 9u;
        return bound;
    }
    std::uint32_t Allocate(std::uint32_t bytes) override {
        calls.emplace_back("allocate");
        Check(bytes == expected_bytes,
              "allocation uses factory or class-specific owned size");
        return allocation;
    }
    std::uint32_t ConstructVectorObject(std::uint32_t address) override {
        calls.emplace_back("vector_construct");
        Check(expected_type == 0u && address == allocation,
              "class zero uses vector object constructor");
        if (throw_object) throw std::runtime_error("object");
        return constructed;
    }
    std::uint32_t ConstructExtendedObject(std::uint32_t address) override {
        calls.emplace_back("extended_construct");
        Check(expected_type == 4u && address == allocation,
              "class four uses extended object constructor");
        if (throw_object) throw std::runtime_error("object");
        return constructed;
    }
    std::uint32_t ConstructSmallObject(std::uint32_t) override {
        Check(false, "class zero and four never construct small helper");
        return 0u;
    }
    std::uint32_t ConstructRandomObject(std::uint32_t) override {
        Check(false, "class zero and four never construct random helper");
        return 0u;
    }
    void OnCreated(std::uint32_t owner, std::uint32_t object) override {
        calls.emplace_back("created");
        Check(owner == 0x1000u && state.words[16] == object,
              "owner callback runs after object storage");
    }
    void UpdateOwned(std::uint32_t, std::uint32_t) override {
        Check(false, "old class tests do not exercise shared run method");
    }
    std::uint32_t IsOwnedComplete(std::uint32_t,
                                   std::uint32_t) override {
        Check(false, "class zero and four use duration completion");
        return 0u;
    }
    pal4::PalBehaviorTaskState& Access(std::uint32_t address) override {
        calls.emplace_back("access");
        Check(address == allocation, "factory accesses its allocation");
        return state;
    }
    bool IsDurationComplete(pal4::PalBehaviorTaskState& target) override {
        calls.emplace_back("duration");
        Check(&target == &state, "duration thunk forwards this object");
        return duration_complete;
    }
};

template <typename Behavior>
void TestClass(std::uint32_t type_id, std::uint32_t owned_bytes,
               const char* constructor_call) {
    Runtime runtime;
    runtime.expected_type = type_id;
    runtime.expected_bytes = 0x44u;
    runtime.state.words.fill(0xaaaaaaaau);
    auto& result = Behavior::Construct(runtime.state, runtime);
    Check(&result == &runtime.state &&
              runtime.calls == std::vector<std::string>{"base_construct"} &&
              runtime.state.words[0] == Behavior::kVtable &&
              runtime.state.words[1] == 0x12345678u &&
              runtime.state.words[14] == 0xaaaaaaaau &&
              runtime.state.words[15] == 0u && runtime.state.words[16] == 0u,
          "constructor writes only derived vtable and tail fields");

    runtime.calls.clear();
    Check(Behavior::BindOwner(runtime.state, 0x6000u, runtime) == 0x1000u &&
              runtime.state.words[15] == 0x1000u &&
              runtime.calls == std::vector<std::string>{"bind"},
          "binding stores returned pointer minus four");
    runtime.bound = 0u;
    Check(Behavior::BindOwner(runtime.state, 0x6000u, runtime) == 0u &&
              runtime.state.words[15] == 0u,
          "null binding clears owner");
    runtime.calls.clear();
    Check(Behavior::CreateOwned(runtime.state, runtime) == 0u &&
              runtime.calls.empty(), "unbound task does not create object");

    runtime.state.words[15] = 0x1000u;
    runtime.expected_bytes = owned_bytes;
    Check(Behavior::CreateOwned(runtime.state, runtime) == 1u &&
              runtime.state.words[16] == 0x5000u &&
              runtime.calls == std::vector<std::string>{
                  "allocate", constructor_call, "created"},
          "owned object follows class-specific constructor path");
    runtime.calls.clear();
    Check(pal4::PalBehaviorTaskBase::Finish(runtime.state, runtime) &&
              runtime.state.words[16] == 0u &&
              runtime.calls == std::vector<std::string>{"release"},
          "finish releases helper and clears only object field");
    runtime.calls.clear();
    Check(!pal4::PalBehaviorTaskBase::Finish(runtime.state, runtime) &&
              runtime.calls.empty(), "finish without helper is inert");
    runtime.state.words[16] = 0x5000u;
    runtime.state.words[15] = 0u;
    Check(!pal4::PalBehaviorTaskBase::Finish(runtime.state, runtime) &&
              runtime.state.words[16] == 0x5000u,
          "finish without owner leaves helper stored");

    runtime.calls.clear();
    runtime.state.words[15] = 0x1000u;
    Check(Behavior::Delete(runtime.state, 0x2000u, 1u, runtime) == 0x2000u &&
              runtime.calls == std::vector<std::string>{
                  "read_flag", "release", "base_destruct", "free"} &&
              runtime.state.words[0] == pal4::PalBehaviorTaskBase::kVtable,
          "deleting destructor releases helper then frees task");
    runtime.calls.clear();
    runtime.state.words[16] = 0x5000u;
    runtime.flag = 0u;
    Check(Behavior::Delete(runtime.state, 0x2000u, 0u, runtime) == 0x2000u &&
              runtime.calls == std::vector<std::string>{
                  "read_flag", "base_destruct"},
          "unflagged helper and zero delete flag suppress releases");
    runtime.calls.clear();
    runtime.state.words[16] = 0u;
    Behavior::Destruct(runtime.state, runtime);
    Check(runtime.calls == std::vector<std::string>{"base_destruct"},
          "null helper skips flag read");

    runtime.calls.clear();
    runtime.allocation = 0u;
    runtime.expected_bytes = 0x44u;
    Check(Behavior::Create(runtime) == 0u &&
              runtime.calls == std::vector<std::string>{"allocate"},
          "factory skips construction when allocation fails");
    runtime.calls.clear();
    runtime.allocation = 0x4000u;
    runtime.state.words.fill(0xaaaaaaaau);
    Check(Behavior::Create(runtime) == 0x4000u &&
              runtime.calls == std::vector<std::string>{
                  "allocate", "access", "base_construct"} &&
              runtime.state.words[0] == Behavior::kVtable,
          "factory builds object with class-specific vtable");
    runtime.calls.clear();
    runtime.state.words.fill(0xaaaaaaaau);
    runtime.throw_base = true;
    bool threw = false;
    try { static_cast<void>(Behavior::Create(runtime)); }
    catch (const std::runtime_error&) { threw = true; }
    Check(threw && runtime.calls == std::vector<std::string>{
              "allocate", "access", "base_construct", "free"},
          "factory frees task allocation on constructor exception");

    runtime.calls.clear();
    runtime.throw_base = false;
    runtime.throw_object = true;
    runtime.expected_bytes = owned_bytes;
    runtime.state.words[15] = 0x1000u;
    threw = false;
    try { static_cast<void>(Behavior::CreateOwned(runtime.state, runtime)); }
    catch (const std::runtime_error&) { threw = true; }
    Check(threw && runtime.calls == std::vector<std::string>{
              "allocate", constructor_call, "free"},
          "owned allocation is freed on constructor exception");
    runtime.calls.clear();
    runtime.throw_object = false;
    runtime.allocation = 0u;
    Check(Behavior::CreateOwned(runtime.state, runtime) == 1u &&
              runtime.state.words[16] == 0u &&
              runtime.calls == std::vector<std::string>{"allocate", "created"},
          "allocation failure still calls owner with null helper");
}

void TestCommonMethods() {
    Runtime runtime;
    pal4::PalBehaviorTaskBase::Destruct(runtime.state, runtime);
    Check(runtime.calls == std::vector<std::string>{"base_destruct"},
          "base cleanup delegates after vtable write");
    runtime.calls.clear();
    Check(pal4::PalBehaviorTaskBase::IsDurationComplete(runtime.state, runtime) &&
              runtime.calls == std::vector<std::string>{"duration"},
          "duration thunk forwards true result");
    runtime.duration_complete = false;
    Check(!pal4::PalBehaviorTaskBase::IsDurationComplete(runtime.state, runtime),
          "duration thunk forwards false result");
    pal4::PalBehaviorTaskRuntimeStub stub;
    Check(pal4::PalBehaviorClassZero::Create(stub) == 0u &&
              pal4::PalBehaviorClassFour::Create(stub) == 0u,
          "both classes link without external services");
}

}  // namespace

int main() {
    TestClass<pal4::PalBehaviorClassZero>(0u, 0xe0u, "vector_construct");
    TestClass<pal4::PalBehaviorClassFour>(4u, 0xc0u, "extended_construct");
    TestCommonMethods();
}
