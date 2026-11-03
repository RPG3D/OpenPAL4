#include "pal4/process_heap_finalizer.h"

#include <cstdint>
#include <cstdlib>
#include <deque>
#include <iostream>
#include <unordered_map>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

class Runtime final : public pal4::IProcessHeapFinalizerRuntime {
public:
    [[nodiscard]] std::uint32_t ReadNext(
        const std::uint32_t node) const override {
        const auto found = next.find(node);
        return found == next.end() ? 0u : found->second;
    }

    [[nodiscard]] std::uint32_t GetProcessHeap() override {
        ++heap_calls;
        return 0xa000u;
    }

    [[nodiscard]] std::uint32_t FreeFromHeap(
        const std::uint32_t heap, const std::uint32_t node) override {
        Check(heap == 0xa000u, "free uses process heap handle");
        freed.push_back(node);
        if (results.empty()) {
            return 1u;
        }
        const std::uint32_t result = results.front();
        results.pop_front();
        return result;
    }

    std::unordered_map<std::uint32_t, std::uint32_t> next;
    std::deque<std::uint32_t> results;
    std::vector<std::uint32_t> freed;
    int heap_calls = 0;
};

void CheckNullAndSingleNode() {
    std::uint32_t empty = 0u;
    Runtime runtime;
    Check(pal4::ProcessHeapFinalizer(empty).Finalize(runtime) == 0u,
          "empty head returns zero");
    Check(runtime.heap_calls == 0, "empty list does not query process heap");

    std::uint32_t single = 0x1000u;
    runtime.results.push_back(7u);
    Check(pal4::ProcessHeapFinalizer(single).Finalize(runtime) == 7u,
          "single node returns HeapFree result");
    Check(single == 0x1000u && runtime.freed.size() == 1u &&
              runtime.freed[0] == 0x1000u,
          "last freed node remains in global head");
}

void CheckChainAndIntermediateFailure() {
    std::uint32_t head = 0x2000u;
    Runtime runtime;
    runtime.next[0x2000u] = 0x3000u;
    runtime.next[0x3000u] = 0x4000u;
    runtime.results = {1u, 1u, 9u};
    Check(pal4::ProcessHeapFinalizer(head).Finalize(runtime) == 9u,
          "chain returns final free result");
    Check(runtime.freed == std::vector<std::uint32_t>{0x2000u, 0x3000u,
                                                      0x4000u} &&
              runtime.heap_calls == 3 && head == 0x4000u,
          "chain frees in order and preserves final head value");

    head = 0x2000u;
    Runtime failure;
    failure.next = runtime.next;
    failure.results = {1u, 0u};
    Check(pal4::ProcessHeapFinalizer(head).Finalize(failure) == 0u,
          "intermediate HeapFree failure stops traversal");
    Check(head == 0x3000u &&
              failure.freed == std::vector<std::uint32_t>{0x2000u, 0x3000u},
          "failed node remains head and later node is not freed");
}

}  // namespace

int main() {
    CheckNullAndSingleNode();
    CheckChainAndIntermediateFailure();
    std::cout << "2 process heap finalizer test cases passed\n";
}
