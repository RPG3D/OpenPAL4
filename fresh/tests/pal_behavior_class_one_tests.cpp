#include "pal4/pal_behavior_class_one.h"

#include <cstdint>
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

struct Runtime final : pal4::IPalBehaviorClassOneRuntime {
    pal4::PalBehaviorClassOneState state;
    std::vector<std::string> calls;
    std::vector<std::uint32_t> freed;
    std::uint32_t allocation_result = 0x4000u;
    std::uint32_t bind_result = 0u;
    std::uint32_t vector_result = 0x5000u;
    std::uint8_t owned_flag = 0u;
    bool throw_on_base = false;
    bool throw_on_vector = false;

    void ConstructBase(pal4::PalBehaviorClassOneState& target) override {
        calls.emplace_back("base_construct");
        if (throw_on_base) throw std::runtime_error("base constructor");
        Check(target.words[0] == 0xaaaaaaaau &&
                  target.words[15] == 0xaaaaaaaau,
              "base constructor precedes derived writes");
        target.words[1] = 0x12345678u;
    }
    void DestructBase(pal4::PalBehaviorClassOneState& target) override {
        calls.emplace_back("base_destruct");
        Check(target.words[0] == 0x8427f4u &&
                  target.words[15] == 0u && target.words[16] == 0u,
              "base destructor follows field clear and vtable reset");
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        calls.emplace_back("read_flag");
        Check(address == 0x50b8u,
              "owned-object flag is read at object plus 0xb8");
        return owned_flag;
    }
    void ReleaseOwned(std::uint32_t owner,
                      std::uint32_t object) override {
        calls.emplace_back("release");
        Check(owner == 0x1000u && object == 0x5000u &&
                  state.words[0] == 0x8427d4u &&
                  state.words[15] == owner && state.words[16] == object,
              "release precedes clearing owned fields");
    }
    void Free(std::uint32_t address) override {
        calls.emplace_back("free");
        freed.push_back(address);
    }
    std::uint32_t Bind(std::uint32_t binder,
                       std::uint32_t& initial_value) override {
        calls.emplace_back("bind");
        Check(binder == 0x6000u && initial_value == 1u,
              "binding forwards the confirmed constant-one local");
        initial_value = 17u;
        return bind_result;
    }
    std::uint32_t Allocate(std::uint32_t bytes) override {
        calls.emplace_back("allocate");
        Check(bytes == 0xe0u || bytes == 0x44u,
              "factory and owned object use their observed allocation sizes");
        return allocation_result;
    }
    std::uint32_t InitializeVectorObject(
        std::uint32_t address) override {
        calls.emplace_back("vector_init");
        Check(address == allocation_result,
              "vector object initializes the allocated address");
        if (throw_on_vector) throw std::runtime_error("vector constructor");
        return vector_result;
    }
    void OnCreated(std::uint32_t owner,
                   std::uint32_t object) override {
        calls.emplace_back("created");
        Check(owner == 0x1000u && state.words[16] == object,
              "owner callback follows storage of created object");
    }
    pal4::PalBehaviorClassOneState& Access(
        std::uint32_t address) override {
        calls.emplace_back("access");
        Check(address == allocation_result,
              "factory accesses the newly allocated object");
        return state;
    }
};

void TestConstructAndBind() {
    Runtime runtime;
    runtime.state.words.fill(0xaaaaaaaau);
    auto& result = pal4::PalBehaviorClassOne::Construct(
        runtime.state, runtime);
    Check(&result == &runtime.state &&
              runtime.calls ==
                  std::vector<std::string>{"base_construct"} &&
              runtime.state.words[0] == 0x8427d4u &&
              runtime.state.words[1] == 0x12345678u &&
              runtime.state.words[15] == 0u &&
              runtime.state.words[16] == 0u &&
              runtime.state.words[14] == 0xaaaaaaaau,
          "constructor installs class-one vtable and zeros two tail fields");
    runtime.calls.clear();
    Check(pal4::PalBehaviorClassOne::BindOwner(
              runtime.state, 0x6000u, runtime) == 0u &&
              runtime.state.words[15] == 0u &&
              runtime.calls == std::vector<std::string>{"bind"},
          "null binding result clears owner field");
    runtime.bind_result = 0x1004u;
    Check(pal4::PalBehaviorClassOne::BindOwner(
              runtime.state, 0x6000u, runtime) == 0x1000u &&
              runtime.state.words[15] == 0x1000u,
          "non-null binding result stores pointer minus four");
}

void TestOwnedCreation() {
    Runtime runtime;
    Check(pal4::PalBehaviorClassOne::CreateOwned(
              runtime.state, runtime) == 0u && runtime.calls.empty(),
          "no bound owner skips allocation and callback");
    runtime.state.words[15] = 0x1000u;
    Check(pal4::PalBehaviorClassOne::CreateOwned(
              runtime.state, runtime) == 1u &&
              runtime.calls == std::vector<std::string>{
                  "allocate", "vector_init", "created"} &&
              runtime.state.words[16] == 0x5000u,
          "owned object is allocated, initialized, stored and published");
    runtime.calls.clear();
    runtime.allocation_result = 0u;
    Check(pal4::PalBehaviorClassOne::CreateOwned(
              runtime.state, runtime) == 1u &&
              runtime.calls ==
                  std::vector<std::string>{"allocate", "created"} &&
              runtime.state.words[16] == 0u,
          "allocation failure still invokes owner callback with null");
    runtime.calls.clear();
    runtime.allocation_result = 0x4000u;
    runtime.throw_on_vector = true;
    bool threw = false;
    try {
        static_cast<void>(pal4::PalBehaviorClassOne::CreateOwned(
            runtime.state, runtime));
    } catch (const std::runtime_error&) {
        threw = true;
    }
    Check(threw && runtime.calls == std::vector<std::string>{
              "allocate", "vector_init", "free"} &&
              runtime.freed == std::vector<std::uint32_t>{0x4000u},
          "failed owned-object constructor frees its allocation");
}

void TestDestruction() {
    Runtime runtime;
    runtime.state.words[0] = 0x8427d4u;
    runtime.state.words[15] = 0x1000u;
    runtime.state.words[16] = 0x5000u;
    runtime.owned_flag = 1u;
    Check(pal4::PalBehaviorClassOne::Delete(
              runtime.state, 0x2000u, 0u, runtime) == 0x2000u &&
              runtime.calls == std::vector<std::string>{
                  "read_flag", "release", "base_destruct"} &&
              runtime.freed.empty() &&
              runtime.state.words[0] == 0x8427f4u,
          "destructor conditionally releases owned object before base");
    runtime.calls.clear();
    runtime.state.words[15] = 0x1000u;
    runtime.state.words[16] = 0x5000u;
    runtime.owned_flag = 0u;
    Check(pal4::PalBehaviorClassOne::Delete(
              runtime.state, 0x2000u, 3u, runtime) == 0x2000u &&
              runtime.calls == std::vector<std::string>{
                  "read_flag", "base_destruct", "free"} &&
              runtime.freed == std::vector<std::uint32_t>{0x2000u},
          "deleting destructor frees after base cleanup on low flag bit");
    runtime.calls.clear();
    runtime.state.words[16] = 0u;
    pal4::PalBehaviorClassOne::Destruct(runtime.state, runtime);
    Check(runtime.calls == std::vector<std::string>{"base_destruct"},
          "null owned object skips flag read and release");
}

void TestFactory() {
    Runtime runtime;
    runtime.state.words.fill(0xaaaaaaaau);
    runtime.allocation_result = 0u;
    Check(pal4::PalBehaviorClassOne::Create(runtime) == 0u &&
              runtime.calls == std::vector<std::string>{"allocate"},
          "factory returns null without construction after allocation failure");
    runtime.calls.clear();
    runtime.allocation_result = 0x4000u;
    Check(pal4::PalBehaviorClassOne::Create(runtime) == 0x4000u &&
              runtime.calls == std::vector<std::string>{
                  "allocate", "access", "base_construct"} &&
              runtime.state.words[0] == 0x8427d4u,
          "factory allocates 68 bytes and constructs the returned address");
    runtime.calls.clear();
    runtime.state.words.fill(0xaaaaaaaau);
    runtime.throw_on_base = true;
    bool threw = false;
    try {
        static_cast<void>(pal4::PalBehaviorClassOne::Create(runtime));
    } catch (const std::runtime_error&) {
        threw = true;
    }
    Check(threw && runtime.calls == std::vector<std::string>{
              "allocate", "access", "base_construct", "free"} &&
              runtime.freed == std::vector<std::uint32_t>{0x4000u},
          "factory frees allocated object when base construction throws");
}

void TestLinkableStub() {
    pal4::PalBehaviorClassOneRuntimeStub runtime;
    Check(pal4::PalBehaviorClassOne::Create(runtime) == 0u,
          "class-one factory links without allocator or behavior services");
    pal4::PalBehaviorClassOneState state;
    static_cast<void>(pal4::PalBehaviorClassOne::Construct(state, runtime));
    Check(pal4::PalBehaviorClassOne::CreateOwned(state, runtime) == 0u,
          "linkable stub leaves unbound behavior inactive");
}

}  // namespace

int main() {
    TestConstructAndBind();
    TestOwnedCreation();
    TestDestruction();
    TestFactory();
    TestLinkableStub();
}
