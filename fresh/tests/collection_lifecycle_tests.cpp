#include "pal4/collection_lifecycle.h"

#include <array>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <new>
#include <stdexcept>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::CollectionLifecycleRuntimeStub {
    std::vector<char> events;
    std::size_t allocation_count{};
    bool freed_null{};
    bool freed_base{};
    bool throw_on_allocate{};
    std::uint32_t receiver{};
    std::uint32_t world_clump{};
    std::vector<std::uint32_t> deleted_elements;
    std::vector<std::uint32_t> freed_world_buffers;
    std::uint8_t prefix_byte{};
    std::uint8_t written_prefix{0xffu};

    void ConstructGameWorld(pal4::CollectionState& state) override {
        Check(state.base.vtable == pal4::CollectionState::kVtable &&
                  state.base.references == 1u && !state.first &&
                  !state.last && !state.limit,
              "nested initializer observes cleared vector and derived base");
        events.push_back('I');
    }
    std::uint32_t* AllocateWords(std::size_t count) override {
        allocation_count = count;
        events.push_back('A');
        if (throw_on_allocate) throw std::bad_alloc();
        return CollectionLifecycleRuntimeStub::AllocateWords(count);
    }
    void FreeWords(std::uint32_t* storage) override {
        if (!storage) freed_null = true;
        events.push_back('F');
        CollectionLifecycleRuntimeStub::FreeWords(storage);
    }
    void FreeBase(pal4::CollectionBaseState& base) override {
        Check(base.vtable == pal4::CollectionState::kBaseVtable,
              "base vtable is restored before freeing");
        freed_base = true;
        events.push_back('B');
    }
    int Clear(std::uint32_t value, pal4::CollectionState&,
              std::size_t offset, std::size_t width) override {
        Check(offset == 4u && width == 4u,
              "clear forwards vector header offset and DWORD width");
        receiver = value;
        events.push_back('C');
        return -37;
    }
    int ClearAndFree(std::uint32_t value, pal4::CollectionState&,
                     std::size_t offset, std::size_t width) override {
        Check(offset == 4u && width == 4u,
              "clear-and-free forwards vector header offset and DWORD width");
        receiver = value;
        events.push_back('D');
        return 23;
    }
    void DeleteElement(std::uint32_t address,
                       std::uint32_t flags) override {
        Check(flags == 1u, "elements use virtual deleting destructor flag one");
        deleted_elements.push_back(address);
        events.push_back('E');
    }
    std::uint32_t WorldClump(pal4::CollectionState&) override {
        events.push_back('W');
        return world_clump;
    }
    void RemoveWorldClump(std::uint32_t clump) override {
        Check(clump == world_clump && clump != 0u,
              "world removal receives stored clump");
        events.push_back('M');
    }
    void CleanupAtomicGeometry(pal4::CollectionState&,
                               std::size_t offset) override {
        Check(offset == 0xdcu, "geometry cleanup receives member +0xdc");
        events.push_back('P');
    }
    std::uint32_t GameInstance() override {
        events.push_back('S');
        return 0x1000u;
    }
    int CleanupWorld(pal4::CollectionState&,
                     std::uint32_t owner) override {
        Check(owner == 0x11acu,
              "world cleanup receives game owner +0x1ac");
        events.push_back('G');
        return -11;
    }
    std::uint8_t ReadPrefixByte(std::uint32_t address) override {
        Check(address == 0x8ffu,
              "shared buffer reads its preceding reference byte");
        events.push_back('Q');
        return prefix_byte;
    }
    void WritePrefixByte(std::uint32_t address,
                         std::uint8_t value) override {
        Check(address == 0x8ffu,
              "reference decrement writes preceding byte");
        written_prefix = value;
        events.push_back('T');
    }
    void FreeWorldBuffer(std::uint32_t address) override {
        freed_world_buffers.push_back(address);
        events.push_back('H');
    }
    void DestroyGameWorld(pal4::CollectionState& state) override {
        Check(state.world.vtable == pal4::CollectionWorldState::kVtable &&
                  state.world.field_c8 == 0u &&
                  state.world.field_cc == 0u &&
                  state.world.field_d0 == 0u &&
                  state.world.field_d8 == 0u &&
                  state.world.field_dc == 0u &&
                  state.world.field_e0 == 0u,
              "world base destructor observes cleared owned fields");
        events.push_back('N');
    }
};

void CheckConstructionAndDestruction() {
    pal4::CollectionState state{};
    state.first = reinterpret_cast<std::uint32_t*>(0x1234u);
    state.last = reinterpret_cast<std::uint32_t*>(0x5678u);
    state.limit = reinterpret_cast<std::uint32_t*>(0x9abcu);
    Runtime runtime;
    Check(&pal4::CollectionLifecycle::Construct(state, 0xa5u, 0x5au,
                                                runtime) ==
              &state && state.initial_byte == 0xa5u &&
              state.world.byte_c4 == 0x5au &&
              state.world.byte_d4 == 0x5au &&
              state.world.vtable == pal4::CollectionWorldState::kVtable &&
              state.world.field_c8 == 0u &&
              state.world.field_cc == 0u &&
              state.world.field_d0 == 0u &&
              state.world.field_d8 == 0u &&
              state.world.field_dc == 0u &&
              state.world.field_e0 == 0u &&
              runtime.allocation_count == 8u && runtime.freed_null &&
              runtime.events == std::vector<char>{'I', 'A', 'F'} &&
              state.first != nullptr && state.last == state.first &&
              state.limit == state.first + 8,
          "constructor initializes fields and allocates eight DWORDs");

    runtime.events.clear();
    pal4::CollectionLifecycle::Destroy(state, runtime);
    Check(runtime.events == std::vector<char>{
              'W', 'P', 'S', 'G', 'P', 'H', 'N', 'F'} &&
              state.base.vtable == pal4::CollectionState::kBaseVtable &&
              state.base.references == 1u && !state.first && !state.last &&
              !state.limit && state.initial_byte == 0xa5u,
          "destructor cleans nested state, restores base and frees vector");
}

void CheckNestedDestruction() {
    pal4::CollectionState state{};
    state.world.vtable = 0u;
    state.world.field_c8 = 0x700u;
    state.world.field_cc = 0x701u;
    state.world.field_d0 = 0x702u;
    state.world.field_d8 = 0x900u;
    state.world.field_dc = 0x901u;
    state.world.field_e0 = 0x902u;
    Runtime runtime;
    runtime.prefix_byte = 3u;
    pal4::CollectionLifecycle::DestroyNested(state, runtime);
    Check(runtime.events == std::vector<char>{'P', 'Q', 'T', 'H', 'N'} &&
              runtime.written_prefix == 2u &&
              runtime.freed_world_buffers ==
                  std::vector<std::uint32_t>{0x700u},
          "ordinary shared prefix decrements before owned buffer release");

    for (auto byte : {std::uint8_t{0u}, std::uint8_t{0xffu}}) {
        state.world.field_d8 = 0x900u;
        state.world.field_c8 = 0x700u;
        runtime.events.clear();
        runtime.freed_world_buffers.clear();
        runtime.prefix_byte = byte;
        pal4::CollectionLifecycle::DestroyNested(state, runtime);
        Check(runtime.events ==
                  std::vector<char>{'P', 'Q', 'H', 'H', 'N'} &&
                  runtime.freed_world_buffers ==
                  std::vector<std::uint32_t>{0x8ffu, 0x700u},
              "zero and 0xff shared prefixes free the prefixed allocation");
    }
}

void CheckCleanupFunctions() {
    std::array<std::uint32_t, 4> objects{0x100u, 0u, 0x300u, 0u};
    pal4::CollectionState state{};
    state.first = objects.data();
    state.last = objects.data() + 3;
    state.limit = objects.data() + objects.size();
    Runtime runtime;
    pal4::CollectionLifecycle::CleanupElements(state, runtime);
    Check(runtime.deleted_elements ==
              std::vector<std::uint32_t>{0x100u, 0x300u} &&
              runtime.events == std::vector<char>{'E', 'E'} &&
              state.last == state.first && state.limit == objects.data() + 4,
          "element cleanup skips nulls, preserves capacity and clears count");

    runtime.events.clear();
    runtime.world_clump = 0x4433u;
    Check(pal4::CollectionLifecycle::CleanupGameObjects(state, runtime) ==
              -11 && runtime.events ==
              std::vector<char>{'W', 'M', 'P', 'S', 'G'},
          "world cleanup removes nonzero clump before geometry and owner");
    runtime.events.clear();
    runtime.world_clump = 0u;
    Check(pal4::CollectionLifecycle::CleanupGameObjects(state, runtime) ==
              -11 && runtime.events ==
              std::vector<char>{'W', 'P', 'S', 'G'},
          "world cleanup skips removal when clump is absent");
}

void CheckBaseAndForwarders() {
    pal4::CollectionBaseState base{};
    pal4::CollectionLifecycle::ConstructBase(base);
    Check(base.vtable == pal4::CollectionState::kBaseVtable &&
              base.references == 1u,
          "base constructor writes observed fields");
    Runtime runtime;
    Check(&pal4::CollectionLifecycle::DeleteBase(base, 2u, runtime) ==
              &base && !runtime.freed_base,
          "base deletion ignores flags without low bit");
    Check(&pal4::CollectionLifecycle::DeleteBase(base, 3u, runtime) ==
              &base && runtime.freed_base &&
              runtime.events == std::vector<char>{'B'},
          "base deletion frees only with low flag bit");
    Check(std::strcmp(pal4::CollectionLifecycle::IdGeneratorName(),
                      "palIdGenerator") == 0,
          "ID generator name matches IDA string");

    pal4::CollectionState state{};
    Check(pal4::CollectionLifecycle::Clear(state, 0x1223u, runtime) ==
              -37 && runtime.receiver == 0x1223u,
          "clear forwards receiver and return value");
    Check(pal4::CollectionLifecycle::ClearAndFree(state, 0x3445u,
                                                   runtime) == 23 &&
              runtime.receiver == 0x3445u &&
              runtime.events == std::vector<char>{'B', 'C', 'D'},
          "clear-and-free forwards receiver and return value");
}

void CheckAllocationFailure() {
    pal4::CollectionState state{};
    Runtime runtime;
    runtime.throw_on_allocate = true;
    bool threw = false;
    try {
        (void)pal4::CollectionLifecycle::Construct(state, 0x3cu, 0x23u,
                                                   runtime);
    } catch (const std::bad_alloc&) {
        threw = true;
    }
    Check(threw && runtime.events == std::vector<char>{'I', 'A'} &&
              state.initial_byte == 0x3cu && !state.first && !state.last &&
              !state.limit,
          "allocation failure leaves vector header empty");
}

void CheckNestedInitialization() {
    pal4::CollectionState state{};
    state.base.vtable = pal4::CollectionState::kVtable;
    state.base.references = 1u;
    state.world = {0xffffffffu, 0xffu, 9u, 8u, 7u, 0xffu,
                   6u, 5u, 4u};
    Runtime runtime;
    pal4::CollectionLifecycle::InitializeNested(state, 0x71u, runtime);
    Check(state.world.vtable == pal4::CollectionWorldState::kVtable &&
              state.world.byte_c4 == 0x71u &&
              state.world.byte_d4 == 0x71u &&
              state.world.field_c8 == 0u && state.world.field_cc == 0u &&
              state.world.field_d0 == 0u && state.world.field_d8 == 0u &&
              state.world.field_dc == 0u && state.world.field_e0 == 0u &&
              runtime.events == std::vector<char>{'I'},
          "nested initializer clears six words after world construction");
}

}  // namespace

int main() {
    CheckConstructionAndDestruction();
    CheckBaseAndForwarders();
    CheckCleanupFunctions();
    CheckAllocationFailure();
    CheckNestedInitialization();
    CheckNestedDestruction();
    std::cout << "collection lifecycle tests passed\n";
}
