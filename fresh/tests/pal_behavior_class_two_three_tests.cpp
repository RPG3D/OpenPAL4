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
    std::uint32_t type_id = 2u;
    std::uint32_t allocation = 0x4000u;
    std::uint32_t allocated_bytes = 0x44u;
    std::uint32_t bound = 0x1004u;
    std::uint32_t helper = 0x5000u;
    std::uint32_t owner_complete = 0u;
    std::uint8_t helper_flag = 1u;
    bool duration_complete = false;
    bool throw_base = false;
    bool throw_helper = false;
    bool throw_write = false;

    void ConstructBase(pal4::PalBehaviorTaskState& target) override {
        calls.emplace_back("base_construct");
        if (throw_base) throw std::runtime_error("base");
        Check(target.words[0] == 0xaaaaaaaau,
              "base construction runs before vtable replacement");
        target.words[2] = 0x12345678u;
    }
    void DestructBase(pal4::PalBehaviorTaskState& target) override {
        calls.emplace_back("base_destruct");
        Check(target.words[0] == pal4::PalBehaviorTaskBase::kVtable &&
                  target.words[15] == 0u && target.words[16] == 0u,
              "base destruction follows clear and vtable transition");
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        calls.emplace_back("read_flag");
        Check(address == 0x50b8u, "helper flag read at plus 0xb8");
        return helper_flag;
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        calls.emplace_back("write_flag");
        Check(type_id == 2u &&
                  address == state.words[16] + 0xb8u && value == 0u,
              "class two clears helper flag even for a null helper");
        if (throw_write) throw std::runtime_error("invalid write");
    }
    void ReleaseOwned(std::uint32_t owner, std::uint32_t object) override {
        calls.emplace_back("release");
        Check(owner == 0x1000u && object == 0x5000u,
              "release uses stored owner and helper");
    }
    void Free(std::uint32_t address) override {
        calls.emplace_back("free");
        Check(address == 0x4000u || address == 0x2000u || address == 0u,
              "only task or helper allocation is freed");
    }
    std::uint32_t Bind(std::uint32_t binder,
                       std::uint32_t& requested_type) override {
        calls.emplace_back("bind");
        Check(binder == 0x6000u && requested_type == type_id,
              "binder receives observed class type");
        requested_type = 99u;
        return bound;
    }
    std::uint32_t Allocate(std::uint32_t bytes) override {
        calls.emplace_back("allocate");
        Check(bytes == allocated_bytes, "allocation uses observed size");
        return allocation;
    }
    std::uint32_t ConstructVectorObject(std::uint32_t) override {
        Check(false, "class two and three use dedicated helpers");
        return 0u;
    }
    std::uint32_t ConstructExtendedObject(std::uint32_t) override {
        Check(false, "class two and three use dedicated helpers");
        return 0u;
    }
    std::uint32_t ConstructSmallObject(std::uint32_t address) override {
        calls.emplace_back("small_construct");
        Check(type_id == 2u && address == allocation,
              "class two constructs the 0xbc-byte helper");
        if (throw_helper) throw std::runtime_error("helper");
        return helper;
    }
    std::uint32_t ConstructRandomObject(std::uint32_t address) override {
        calls.emplace_back("random_construct");
        Check(type_id == 3u && address == allocation,
              "class three constructs the 0xe0-byte helper");
        if (throw_helper) throw std::runtime_error("helper");
        return helper;
    }
    void OnCreated(std::uint32_t owner, std::uint32_t object) override {
        calls.emplace_back("created");
        Check(owner == 0x1000u && object == state.words[16],
              "owner is called after helper storage and flag write");
    }
    void UpdateOwned(std::uint32_t owner, std::uint32_t object) override {
        calls.emplace_back("update");
        Check(owner == 0x1000u && object == 0x5000u,
              "run uses owner virtual slot plus eight");
    }
    std::uint32_t IsOwnedComplete(std::uint32_t owner,
                                  std::uint32_t object) override {
        calls.emplace_back("owner_complete");
        Check(owner == 0x1000u && object == 0x5000u,
              "class two uses owner virtual slot plus twelve");
        return owner_complete;
    }
    pal4::PalBehaviorTaskState& Access(std::uint32_t address) override {
        calls.emplace_back("access");
        Check(address == allocation, "factory accesses allocated task");
        return state;
    }
    bool IsDurationComplete(pal4::PalBehaviorTaskState& target) override {
        calls.emplace_back("duration");
        Check(&target == &state, "duration predicate receives task");
        return duration_complete;
    }
};

template <typename Behavior>
void TestLifecycle(std::uint32_t type_id, std::uint32_t helper_bytes,
                   const char* helper_call) {
    Runtime runtime;
    runtime.type_id = type_id;
    runtime.state.words.fill(0xaaaaaaaau);
    Check(&Behavior::Construct(runtime.state, runtime) == &runtime.state &&
              runtime.state.words[0] == Behavior::kVtable &&
              runtime.state.words[2] == 0x12345678u &&
              runtime.state.words[15] == 0u &&
              runtime.state.words[16] == 0u &&
              runtime.calls == std::vector<std::string>{"base_construct"},
          "task constructor initializes exact derived fields");
    runtime.calls.clear();
    Check(Behavior::BindOwner(runtime.state, 0x6000u, runtime) == 0x1000u &&
              runtime.calls == std::vector<std::string>{"bind"},
          "binder stores callback pointer minus four");
    runtime.bound = 0u;
    Check(Behavior::BindOwner(runtime.state, 0x6000u, runtime) == 0u &&
              runtime.state.words[15] == 0u,
          "null binding clears owner");
    runtime.calls.clear();
    Check(Behavior::CreateOwned(runtime.state, runtime) == 0u &&
              runtime.calls.empty(), "unbound task skips helper creation");

    runtime.state.words[15] = 0x1000u;
    runtime.allocated_bytes = helper_bytes;
    Check(Behavior::CreateOwned(runtime.state, runtime) == 1u &&
              runtime.state.words[16] == 0x5000u,
          "bound task creates helper");
    std::vector<std::string> expected{"allocate", helper_call};
    if (type_id == 2u) expected.emplace_back("write_flag");
    expected.emplace_back("created");
    Check(runtime.calls == expected,
          "class-specific helper construction and publication order");

    runtime.calls.clear();
    runtime.helper_flag = 1u;
    Check(Behavior::Delete(runtime.state, 0x2000u, 1u, runtime) == 0x2000u &&
              runtime.calls == std::vector<std::string>{
                  "read_flag", "release", "base_destruct", "free"},
          "deleting destructor releases helper and task in order");
    runtime.calls.clear();
    runtime.state.words[16] = 0x5000u;
    runtime.helper_flag = 0u;
    Check(Behavior::Delete(runtime.state, 0x2000u, 0u, runtime) == 0x2000u &&
              runtime.calls == std::vector<std::string>{
                  "read_flag", "base_destruct"},
          "clear helper flag and delete bit skip both frees");
    runtime.calls.clear();
    runtime.state.words[16] = 0u;
    Behavior::Destruct(runtime.state, runtime);
    Check(runtime.calls == std::vector<std::string>{"base_destruct"},
          "null helper skips flag read");

    runtime.calls.clear();
    runtime.allocation = 0u;
    runtime.allocated_bytes = 0x44u;
    Check(Behavior::Create(runtime) == 0u &&
              runtime.calls == std::vector<std::string>{"allocate"},
          "factory returns null on allocation failure");
    runtime.calls.clear();
    runtime.allocation = 0x4000u;
    runtime.state.words.fill(0xaaaaaaaau);
    Check(Behavior::Create(runtime) == 0x4000u &&
              runtime.state.words[0] == Behavior::kVtable &&
              runtime.calls == std::vector<std::string>{
                  "allocate", "access", "base_construct"},
          "factory constructs allocated task");
    runtime.calls.clear();
    runtime.throw_base = true;
    bool threw = false;
    try { static_cast<void>(Behavior::Create(runtime)); }
    catch (const std::runtime_error&) { threw = true; }
    Check(threw && runtime.calls == std::vector<std::string>{
              "allocate", "access", "base_construct", "free"},
          "factory frees allocation on constructor exception");
    runtime.calls.clear();
    runtime.throw_base = false;
    runtime.throw_helper = true;
    runtime.allocated_bytes = helper_bytes;
    runtime.state.words[15] = 0x1000u;
    threw = false;
    try { static_cast<void>(Behavior::CreateOwned(runtime.state, runtime)); }
    catch (const std::runtime_error&) { threw = true; }
    Check(threw && runtime.calls == std::vector<std::string>{
              "allocate", helper_call, "free"},
          "helper constructor exception frees helper allocation");
}

void TestRun() {
    Runtime runtime;
    Check(!pal4::PalBehaviorTaskBase::Run(runtime.state, runtime) &&
              runtime.calls.empty(), "no helper skips run");
    runtime.state.words[16] = 0x5000u;
    Check(!pal4::PalBehaviorTaskBase::Run(runtime.state, runtime) &&
              runtime.calls.empty(), "no owner skips flag read");
    runtime.state.words[15] = 0x1000u;
    runtime.helper_flag = 0u;
    Check(pal4::PalBehaviorTaskBase::Run(runtime.state, runtime) &&
              runtime.calls == std::vector<std::string>{"read_flag"},
          "inactive helper still makes run return true");
    runtime.calls.clear();
    runtime.helper_flag = 1u;
    Check(pal4::PalBehaviorTaskBase::Run(runtime.state, runtime) &&
              runtime.calls == std::vector<std::string>{
                  "read_flag", "update"},
          "active helper dispatches owner update");
}

void TestCompletion() {
    Runtime runtime;
    runtime.state.words[16] = 0x5000u;
    Check(!pal4::PalBehaviorClassTwo::IsComplete(runtime.state, runtime) &&
              !pal4::PalBehaviorClassThree::IsComplete(runtime.state, runtime) &&
              runtime.calls.empty(), "missing owner skips both predicates");
    runtime.state.words[15] = 0x1000u;
    runtime.owner_complete = 0x100u;
    Check(!pal4::PalBehaviorClassTwo::IsComplete(runtime.state, runtime) &&
              runtime.calls == std::vector<std::string>{"owner_complete"},
          "class two returns low byte of owner predicate");
    runtime.calls.clear();
    runtime.owner_complete = 0x101u;
    Check(pal4::PalBehaviorClassTwo::IsComplete(runtime.state, runtime),
          "class two accepts nonzero low byte");
    runtime.calls.clear();
    Check(!pal4::PalBehaviorClassThree::IsComplete(runtime.state, runtime) &&
              runtime.calls == std::vector<std::string>{"duration"},
          "class three delegates to duration predicate");
    runtime.duration_complete = true;
    Check(pal4::PalBehaviorClassThree::IsComplete(runtime.state, runtime),
          "class three forwards true duration result");
    runtime.state.words[16] = 0u;
    runtime.calls.clear();
    Check(!pal4::PalBehaviorClassTwo::IsComplete(runtime.state, runtime) &&
              !pal4::PalBehaviorClassThree::IsComplete(runtime.state, runtime) &&
              runtime.calls.empty(), "missing helper skips both predicates");
}

void TestAllocationFailure() {
    Runtime runtime;
    runtime.state.words[15] = 0x1000u;
    runtime.allocated_bytes = 0xbcu;
    runtime.allocation = 0u;
    Check(pal4::PalBehaviorClassTwo::CreateOwned(runtime.state, runtime) == 1u &&
              runtime.calls == std::vector<std::string>{
                  "allocate", "write_flag", "created"} &&
              runtime.state.words[16] == 0u,
          "class two retains the invalid write on failed allocation");
    runtime.calls.clear();
    runtime.throw_write = true;
    bool threw = false;
    try {
        static_cast<void>(pal4::PalBehaviorClassTwo::CreateOwned(
            runtime.state, runtime));
    } catch (const std::runtime_error&) { threw = true; }
    Check(threw && runtime.calls == std::vector<std::string>{
              "allocate", "write_flag", "free"},
          "failed flag write invokes the observed cleanup before callback");
    runtime.calls.clear();
    runtime.throw_write = false;
    runtime.type_id = 3u;
    runtime.allocated_bytes = 0xe0u;
    Check(pal4::PalBehaviorClassThree::CreateOwned(runtime.state, runtime) ==
                  1u &&
              runtime.calls == std::vector<std::string>{
                  "allocate", "created"},
          "class three publishes null helper after allocation failure");
}

void TestStubs() {
    pal4::PalBehaviorTaskRuntimeStub stub;
    Check(pal4::PalBehaviorClassTwo::Create(stub) == 0u &&
              pal4::PalBehaviorClassThree::Create(stub) == 0u,
          "new classes link without game allocator or virtual methods");
}

}  // namespace

int main() {
    TestLifecycle<pal4::PalBehaviorClassTwo>(2u, 0xbcu, "small_construct");
    TestLifecycle<pal4::PalBehaviorClassThree>(3u, 0xe0u,
                                               "random_construct");
    TestRun();
    TestCompletion();
    TestAllocationFailure();
    TestStubs();
}
