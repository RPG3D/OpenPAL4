#include "pal4/exception_handler_pointer_array.h"

#include <cstdint>
#include <cstdlib>
#include <deque>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

class Runtime final : public pal4::IExceptionHandlerPointerArrayRuntime {
public:
    [[nodiscard]] std::uint32_t Read(
        const std::uint32_t address) const override {
        const auto found = words.find(address);
        return found == words.end() ? 0u : found->second;
    }

    void Write(const std::uint32_t address,
               const std::uint32_t value) override {
        words[address] = value;
        events.emplace_back("write");
    }

    void CopyDwords(const std::uint32_t destination,
                    const std::uint32_t source,
                    const std::uint32_t count) override {
        events.emplace_back("copy");
        copy_count = count;
        for (std::uint32_t index = 0; index < count; ++index) {
            words[destination + 4u * index] = Read(source + 4u * index);
        }
    }

    [[nodiscard]] std::uint32_t GetProcessHeap() override {
        events.emplace_back("heap");
        return 0x5000u;
    }

    [[nodiscard]] std::uint32_t HeapAllocate(
        const std::uint32_t heap, const std::uint32_t flags,
        const std::uint32_t bytes) override {
        Check(heap == 0x5000u && flags == 0xcu,
              "allocation uses process heap and flags 0xC");
        allocated_sizes.push_back(bytes);
        events.emplace_back("allocate");
        if (allocation_results.empty()) {
            return 0u;
        }
        const std::uint32_t result = allocation_results.front();
        allocation_results.pop_front();
        return result;
    }

    [[nodiscard]] std::uint32_t HeapFree(
        const std::uint32_t heap, const std::uint32_t flags,
        const std::uint32_t address) override {
        Check(heap == 0x5000u && flags == 0u,
              "old allocation is freed from process heap with zero flags");
        freed.push_back(address);
        events.emplace_back("free");
        return free_result;
    }

    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::deque<std::uint32_t> allocation_results;
    std::vector<std::uint32_t> allocated_sizes;
    std::vector<std::uint32_t> freed;
    std::vector<std::string> events;
    std::uint32_t copy_count = 0u;
    std::uint32_t free_result = 1u;
};

void CheckNullAndFailedAllocation() {
    Runtime runtime;
    runtime.words[pal4::ExceptionHandlerPointerArray::kBuffer] = 0x1000u;
    runtime.words[pal4::ExceptionHandlerPointerArray::kCount] = 2u;
    Check(pal4::ExceptionHandlerPointerArray::Append(0u, runtime) == 0u &&
              runtime.events.empty(),
          "null element exits before heap or state operations");
    runtime.allocation_results.push_back(0u);
    Check(pal4::ExceptionHandlerPointerArray::Append(0x33u, runtime) == 0u &&
              runtime.Read(pal4::ExceptionHandlerPointerArray::kBuffer) ==
                  0x1000u &&
              runtime.Read(pal4::ExceptionHandlerPointerArray::kCount) == 2u &&
              runtime.freed.empty(),
          "allocation failure preserves old array and count");
    Check(runtime.allocated_sizes == std::vector<std::uint32_t>{12u},
          "requested byte count equals four times new element count");
}

void CheckFirstAndSubsequentAppend() {
    Runtime runtime;
    runtime.allocation_results = {0x1000u, 0x2000u, 0x3000u};
    Check(pal4::ExceptionHandlerPointerArray::Append(0xaau, runtime) == 1u &&
              runtime.Read(pal4::ExceptionHandlerPointerArray::kBuffer) ==
                  0x1000u &&
              runtime.Read(pal4::ExceptionHandlerPointerArray::kCount) == 1u &&
              runtime.Read(0x1000u) == 0xaau && runtime.freed.empty(),
          "first element occupies new one-DWORD allocation");
    runtime.events.clear();
    Check(pal4::ExceptionHandlerPointerArray::Append(0xbbu, runtime) == 1u &&
              runtime.Read(0x2000u) == 0xaau &&
              runtime.Read(0x2004u) == 0xbbu &&
              runtime.Read(pal4::ExceptionHandlerPointerArray::kCount) == 2u,
          "second append copies first and updates count");
    Check(runtime.allocated_sizes == std::vector<std::uint32_t>{4u, 8u} &&
              runtime.freed == std::vector<std::uint32_t>{0x1000u} &&
              runtime.copy_count == 1u &&
              runtime.events == std::vector<std::string>{
                  "heap", "allocate", "copy", "heap", "free",
                  "write", "write", "write"},
          "old buffer is copied and freed before publishing replacement");

    runtime.free_result = 0u;
    Check(pal4::ExceptionHandlerPointerArray::Append(0xccu, runtime) == 1u &&
              runtime.Read(0x3000u) == 0xaau &&
              runtime.Read(0x3004u) == 0xbbu &&
              runtime.Read(0x3008u) == 0xccu &&
              runtime.Read(pal4::ExceptionHandlerPointerArray::kBuffer) ==
                  0x3000u &&
              runtime.Read(pal4::ExceptionHandlerPointerArray::kCount) == 3u,
          "HeapFree failure is ignored and new array is still published");
}

void CheckExistingPointerWithZeroCount() {
    Runtime runtime;
    runtime.words[pal4::ExceptionHandlerPointerArray::kBuffer] = 0x1000u;
    runtime.allocation_results.push_back(0x2000u);
    Check(pal4::ExceptionHandlerPointerArray::Append(0x44u, runtime) == 1u &&
              runtime.copy_count == 0u &&
              runtime.freed == std::vector<std::uint32_t>{0x1000u} &&
              runtime.Read(0x2000u) == 0x44u,
          "non-null old pointer is freed even when count is zero");
}

void CheckNullPointerWithStaleCount() {
    Runtime runtime;
    runtime.words[pal4::ExceptionHandlerPointerArray::kCount] = 2u;
    runtime.allocation_results.push_back(0x2000u);
    Check(pal4::ExceptionHandlerPointerArray::Append(0x55u, runtime) == 1u &&
              runtime.allocated_sizes == std::vector<std::uint32_t>{12u} &&
              runtime.Read(0x2000u) == 0x55u &&
              runtime.Read(0x2008u) == 0u &&
              runtime.Read(pal4::ExceptionHandlerPointerArray::kCount) == 3u,
          "null old buffer writes first slot even with nonzero old count");
}

}  // namespace

int main() {
    CheckNullAndFailedAllocation();
    CheckFirstAndSubsequentAppend();
    CheckExistingPointerWithZeroCount();
    CheckNullPointerWithStaleCount();
    std::cout << "4 exception handler pointer array test cases passed\n";
}
