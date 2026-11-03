#include "pal4/numeric_spinner_pool.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace {

void Check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

struct Runtime final : pal4::INumericSpinnerPoolRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::unordered_map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> calls;
    std::vector<std::uint32_t> allocation_sizes;
    std::vector<std::string> names;
    std::vector<std::uint32_t> deleted;
    std::uint32_t next_allocation = 0x5000u;
    std::uint32_t freed = 0xffffffffu;
    std::uint32_t updated_spinner = 0u;
    std::int32_t updated_value = -1;
    std::uint32_t removed_parent = 0u;
    std::uint32_t removed_child = 0u;
    std::uint32_t update_count = 0u;
    bool fail_allocation = false;
    int registration_result = 7;
    std::uint32_t registered_cleanup = 0u;

    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.push_back("read:" + std::to_string(address));
        return words[address];
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        calls.push_back("write:" + std::to_string(address));
        words[address] = value;
    }
    std::uint32_t Allocate(std::uint32_t bytes) override {
        calls.emplace_back("allocate");
        allocation_sizes.push_back(bytes);
        if (fail_allocation) return 0u;
        const auto address = next_allocation;
        next_allocation += 0x1000u;
        return address;
    }
    void Free(std::uint32_t address) override {
        calls.emplace_back("free");
        freed = address;
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        calls.emplace_back("read_byte");
        return bytes[address];
    }
    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override {
        calls.emplace_back("write_byte");
        bytes[address] = value;
    }
    int RegisterCleanup(std::uint32_t function_address) override {
        calls.emplace_back("register");
        registered_cleanup = function_address;
        return registration_result;
    }
    std::uint32_t ConstructSpinner(std::uint32_t memory,
                                   std::string_view name,
                                   std::int32_t value) override {
        calls.emplace_back("construct");
        names.emplace_back(name);
        Check(memory != 0u && value == 0,
              "constructor memory and initial value");
        return memory;
    }
    void UpdateValue(std::uint32_t spinner,
                     std::int32_t value) override {
        calls.emplace_back("update");
        updated_spinner = spinner;
        updated_value = value;
        ++update_count;
    }
    void RemoveChild(std::uint32_t parent,
                     std::uint32_t child) override {
        calls.emplace_back("remove_child");
        removed_parent = parent;
        removed_child = child;
    }
    void DeleteSpinner(std::uint32_t spinner,
                       std::uint32_t flag) override {
        calls.emplace_back("delete");
        Check(flag == 1u, "delete flag");
        deleted.push_back(spinner);
    }
};

void CheckCreateAndReuse() {
    Runtime fresh;
    Check(pal4::NumericSpinnerPool::CreateOrGet(0, fresh) == 0x5000u,
          "new spinner pointer");
    Check(fresh.words[pal4::NumericSpinnerPool::kSerial] == 1u &&
          fresh.allocation_sizes == std::vector<std::uint32_t>{0x1e0u} &&
          fresh.names == std::vector<std::string>{"TradeItem1"},
          "new spinner size, serial and name");

    Runtime reused;
    reused.words[pal4::NumericSpinnerPool::kBegin] = 0x2000u;
    reused.words[pal4::NumericSpinnerPool::kEnd] = 0x2008u;
    reused.words[0x2004u] = 0x3000u;
    Check(pal4::NumericSpinnerPool::CreateOrGet(42, reused) == 0x3000u &&
          reused.words[pal4::NumericSpinnerPool::kEnd] == 0x2004u &&
          reused.updated_spinner == 0x3000u &&
          reused.updated_value == 42 && reused.names.empty(),
          "reuse pops last pointer and updates value");

    Runtime failed;
    failed.fail_allocation = true;
    Check(pal4::NumericSpinnerPool::CreateOrGet(0, failed) == 0u &&
          failed.words[pal4::NumericSpinnerPool::kSerial] == 1u &&
          failed.names.empty(),
          "allocation failure still advances serial without construction");
}

void CheckRemove(bool spare) {
    Runtime runtime;
    runtime.words[0x3000u + 436u] = 0x4000u;
    runtime.words[0x4000u + 68u] = 0x4100u;
    runtime.words[0x3000u + 476u] = 9u;
    runtime.words[0x3000u + 316u] = 9u;
    runtime.words[pal4::NumericSpinnerPool::kBegin] = 0x2000u;
    runtime.words[pal4::NumericSpinnerPool::kEnd] = 0x2004u;
    runtime.words[pal4::NumericSpinnerPool::kCapacityEnd] =
        spare ? 0x2008u : 0x2004u;
    runtime.words[0x2000u] = 0x3100u;
    const auto result = pal4::NumericSpinnerPool::Remove(0x3000u, runtime);
    Check(runtime.words[0x3000u + 476u] == 0u &&
          runtime.words[0x3000u + 316u] == 0u &&
          runtime.removed_parent == 0x4100u &&
          runtime.removed_child == 0x4000u,
          "spinner fields cleared and child removed before pooling");
    if (spare) {
        Check(result == 0x2008u &&
              runtime.words[0x2004u] == 0x3000u &&
              runtime.words[pal4::NumericSpinnerPool::kEnd] == 0x2008u &&
              runtime.allocation_sizes.empty(),
              "spare capacity returns new end pointer");
    } else {
        Check(result == 1u && runtime.freed == 0x2000u &&
              runtime.allocation_sizes == std::vector<std::uint32_t>{8u} &&
              runtime.words[0x5000u] == 0x3100u &&
              runtime.words[0x5004u] == 0x3000u &&
              runtime.words[pal4::NumericSpinnerPool::kEnd] == 0x5008u,
              "growth returns old size and preserves pointer order");
    }

    runtime.removed_parent = 0u;
    runtime.words[0x4000u + 68u] = 0u;
    (void)pal4::NumericSpinnerPool::Remove(0x3000u, runtime);
    Check(runtime.removed_parent == 0u,
          "root without parent skips child removal");
}

void CheckClear() {
    Runtime runtime;
    runtime.words[pal4::NumericSpinnerPool::kBegin] = 0x2000u;
    runtime.words[pal4::NumericSpinnerPool::kEnd] = 0x200cu;
    runtime.words[0x2000u] = 0x3000u;
    runtime.words[0x2004u] = 0u;
    runtime.words[0x2008u] = 0x3200u;
    Check(pal4::NumericSpinnerPool::Clear(runtime) == 0x200cu &&
          runtime.words[pal4::NumericSpinnerPool::kEnd] == 0x2000u &&
          runtime.deleted == std::vector<std::uint32_t>{0x3000u, 0x3200u},
          "clear deletes nonnull entries and resets end to begin");
    Runtime empty;
    Check(pal4::NumericSpinnerPool::Clear(empty) == 0u,
          "empty pool clear return");
}

void CheckInitialize() {
    Runtime runtime;
    Check(pal4::NumericSpinnerPool::Initialize(runtime) == 0u,
          "pool initializer final decrement");
    const auto begin = runtime.words[pal4::NumericSpinnerPool::kBegin];
    Check(runtime.names == std::vector<std::string>{"TradeItem1"} &&
          runtime.words[pal4::NumericSpinnerPool::kSerial] == 1u &&
          runtime.words[pal4::NumericSpinnerPool::kEnd] == begin + 4u &&
          runtime.words[begin] == 0x5000u &&
          runtime.updated_spinner == 0x5000u &&
          runtime.updated_value == 0 && runtime.update_count == 99u,
          "100 iterations repeatedly pop and append one spinner");
    Check(runtime.allocation_sizes ==
              std::vector<std::uint32_t>{0x1e0u, 4u},
          "empty-pool initialization allocates one spinner and one slot");
}

void CheckGlobalLifecycle() {
    Runtime runtime;
    runtime.words[pal4::NumericSpinnerPool::kBegin] = 0x2000u;
    runtime.words[pal4::NumericSpinnerPool::kEnd] = 0x2008u;
    runtime.words[pal4::NumericSpinnerPool::kCapacityEnd] = 0x2010u;
    runtime.words[pal4::NumericSpinnerPool::kSerial] = 17u;
    Check(pal4::NumericSpinnerPool::InitializeGlobalsAndRegisterCleanup(
              0x12345678u, runtime) == 7 &&
              runtime.bytes[pal4::NumericSpinnerPool::kInitialByte] == 0x12u &&
              runtime.words[pal4::NumericSpinnerPool::kBegin] == 0u &&
              runtime.words[pal4::NumericSpinnerPool::kEnd] == 0u &&
              runtime.words[pal4::NumericSpinnerPool::kCapacityEnd] == 0u &&
              runtime.words[pal4::NumericSpinnerPool::kSerial] == 17u &&
              runtime.registered_cleanup == 0x4b18d0u &&
              runtime.calls.back() == "register",
          "global initialization and CRT registration");

    runtime.words[pal4::NumericSpinnerPool::kBegin] = 0x3000u;
    runtime.words[pal4::NumericSpinnerPool::kEnd] = 0x3004u;
    runtime.words[pal4::NumericSpinnerPool::kCapacityEnd] = 0x3008u;
    runtime.bytes[pal4::NumericSpinnerPool::kCleanupGuard] = 0xa4u;
    pal4::NumericSpinnerPool::CleanupGlobals(runtime);
    Check(runtime.bytes[pal4::NumericSpinnerPool::kCleanupGuard] == 0xa5u &&
              runtime.freed == 0x3000u &&
              runtime.words[pal4::NumericSpinnerPool::kBegin] == 0u &&
              runtime.words[pal4::NumericSpinnerPool::kEnd] == 0u &&
              runtime.words[pal4::NumericSpinnerPool::kCapacityEnd] == 0u,
          "first cleanup marks guard, frees and resets pool");
    runtime.freed = 0xffffffffu;
    runtime.words[pal4::NumericSpinnerPool::kBegin] = 0x4000u;
    pal4::NumericSpinnerPool::CleanupGlobals(runtime);
    Check(runtime.freed == 0xffffffffu &&
              runtime.words[pal4::NumericSpinnerPool::kBegin] == 0x4000u,
          "guarded cleanup is idempotent");

    Runtime empty;
    pal4::NumericSpinnerPool::CleanupGlobals(empty);
    Check(empty.freed == 0u &&
              empty.bytes[pal4::NumericSpinnerPool::kCleanupGuard] == 1u,
          "null pool is still forwarded to free");
}

}  // namespace

int main() {
    try {
        CheckCreateAndReuse();
        CheckRemove(true);
        CheckRemove(false);
        CheckClear();
        CheckInitialize();
        CheckGlobalLifecycle();
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
