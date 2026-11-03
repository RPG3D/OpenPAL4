#include "pal4/callback_collection_cursor.h"

#include <cstdlib>
#include <iostream>
#include <stdexcept>
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

class FakeRuntime final : public pal4::ICallbackCollectionCursorRuntime {
public:
    [[nodiscard]] std::uint32_t ReadDword(
        const std::uint32_t address) override {
        reads.push_back(address);
        return memory[address];
    }

    void WriteDword(const std::uint32_t address,
                    const std::uint32_t value) override {
        memory[address] = value;
        events.emplace_back("write:" + std::to_string(address) + ":" +
                            std::to_string(value));
    }

    void InvokeObject(const std::uint32_t table_address,
                      const std::uint32_t object) override {
        events.emplace_back("invoke:" + std::to_string(table_address) + ":" +
                            std::to_string(object));
        if (object == throw_once_for_object) {
            throw_once_for_object = 0;
            throw std::runtime_error("callback failed");
        }
    }

    void Free(const std::uint32_t address) override {
        events.emplace_back("free:" + std::to_string(address));
    }

    std::unordered_map<std::uint32_t, std::uint32_t> memory;
    std::vector<std::uint32_t> reads;
    std::vector<std::string> events;
    std::uint32_t throw_once_for_object = 0;
};

void SetPointersAndAdvanceThroughSentinel() {
    pal4::CallbackCollectionState collection;
    collection.Write(0x18, 2);
    collection.Write(0x24, 0x5000);
    FakeRuntime runtime;
    runtime.memory[0x5000] = 0x6000;
    runtime.memory[0x6008] = 0xaaaa;
    runtime.memory[0x6004] = 0x6100;
    runtime.memory[0x6108] = 0xbbbb;
    runtime.memory[0x6104] = 0x5000;

    Check(pal4::CallbackCollectionCursor::SetInternalPointers(collection,
                                                               runtime) ==
              0xaaaa,
          "initialization returns first element payload");
    Check(collection.Read(0x0c) == 0 && collection.Read(0x20) == 0x6000 &&
              collection.Read(0x14) == 2,
          "initialization resets status and stores node/count fields");
    Check(pal4::CallbackCollectionCursor::Advance(collection, runtime) ==
              0xbbbb,
          "advance returns next payload");
    Check(collection.Read(0x20) == 0x6100 && collection.Read(0x14) == 3,
          "advance stores next node and increments index");
    Check(pal4::CallbackCollectionCursor::Advance(collection, runtime) == 0,
          "advance to sentinel returns null");
    Check(collection.Read(0x20) == 0x5000 && collection.Read(0x14) == 0,
          "sentinel end resets cursor index");
}

void EmptyInitializationStillCopiesSentinelFirst() {
    pal4::CallbackCollectionState collection;
    collection.Write(0x24, 0x5000);
    FakeRuntime runtime;
    runtime.memory[0x5000] = 0x5000;
    Check(pal4::CallbackCollectionCursor::SetInternalPointers(collection,
                                                               runtime) == 0,
          "empty count returns null payload");
    Check(collection.Read(0x20) == 0x5000 && collection.Read(0x14) == 0,
          "empty path still sets current from sentinel first pointer");
    Check(runtime.reads == std::vector<std::uint32_t>{0x5000},
          "empty path does not dereference payload");
}

void ReverseInitializationUsesSentinelPreviousAndCount() {
    pal4::CallbackCollectionState collection;
    collection.Write(0x18, 2);
    collection.Write(0x20, 0x7777);
    collection.Write(0x24, 0x5000);
    FakeRuntime runtime;
    runtime.memory[0x5004] = 0x6000;
    runtime.memory[0x6008] = 0xaaaa;
    Check(pal4::CallbackCollectionCursor::InitializeReverse(collection,
                                                             runtime) == 0xaaaa,
          "reverse initialization returns the last node payload");
    Check(collection.Read(0x0c) == 0 && collection.Read(0x20) == 0x6000 &&
              collection.Read(0x14) == 1,
          "reverse initialization replaces a stale cursor");

    collection.Write(0x18, 0);
    runtime.reads.clear();
    Check(pal4::CallbackCollectionCursor::InitializeReverse(collection,
                                                             runtime) == 0 &&
              collection.Read(0x14) == 0 &&
              runtime.reads == std::vector<std::uint32_t>{0x5004},
          "empty reverse initialization skips payload access");
}

void SafeLookupUsesReverseOrderAndIncludesBothValidBounds() {
    pal4::CallbackCollectionState collection;
    collection.Write(0x18, 3);
    collection.Write(0x24, 0x5000);
    FakeRuntime runtime;
    runtime.memory[0x5004] = 0x6100;
    runtime.memory[0x6108] = 0xcccc;
    runtime.memory[0x6104] = 0x6200;
    runtime.memory[0x6208] = 0xbbbb;
    runtime.memory[0x6204] = 0x6300;
    runtime.memory[0x6308] = 0xaaaa;
    runtime.memory[0x6304] = 0x5000;

    Check(pal4::CallbackCollectionCursor::GetElementByIndexSafe(
              collection, 0, runtime) == 0xcccc,
          "index zero selects the tail element");
    Check(pal4::CallbackCollectionCursor::GetElementByIndexSafe(
              collection, 2, runtime) == 0xaaaa,
          "last valid index reaches the head element");
    Check(collection.Read(0x20) == 0x6300 && collection.Read(0x14) == 3,
          "lookup leaves the internal reverse cursor at the selected node");
}

void SafeLookupRejectsNegativeAndUpperBoundIndices() {
    pal4::CallbackCollectionState collection;
    collection.Write(0x18, 2);
    collection.Write(0x24, 0x5000);
    collection.Write(0x20, 0x7777);
    FakeRuntime runtime;

    Check(pal4::CallbackCollectionCursor::GetElementByIndexSafe(
              collection, -1, runtime) == 0,
          "the helper rejects negative indices even though the outer check only tests the upper bound");
    Check(pal4::CallbackCollectionCursor::GetElementByIndexSafe(
              collection, 2, runtime) == 0,
          "index equal to count is rejected");
    Check(collection.Read(0x20) == 0x7777 && runtime.reads.empty(),
          "invalid indices do not initialize or traverse the cursor");
    Check(collection.Read(0x0c) == 0,
          "safe lookup clears status on rejected requests");
}

void LookupStopsWhenReverseLinksReachNullPayload() {
    pal4::CallbackCollectionState collection;
    collection.Write(0x18, 2);
    collection.Write(0x24, 0x5000);
    FakeRuntime runtime;
    runtime.memory[0x5004] = 0x6100;
    runtime.memory[0x6108] = 0xaaaa;
    runtime.memory[0x6104] = 0x6200;
    runtime.memory[0x6208] = 0;

    Check(pal4::CallbackCollectionCursor::GetElementByIndexSafe(
              collection, 1, runtime) == 0,
          "null payload ends the traversal with a null result");
    Check(collection.Read(0x20) == 0x6200 && collection.Read(0x14) == 2,
          "the cursor records the node where traversal stopped");
}

void ClearSkipsWhenCountIsNotPositive() {
    pal4::CallbackCollectionState collection;
    collection.Write(0x18, 0);
    collection.Write(0x20, 0x6000);
    collection.Write(0x24, 0x5000);
    FakeRuntime runtime;
    pal4::CallbackCollectionCursor::Clear(collection, runtime);
    Check(runtime.events.empty() && runtime.reads.empty(),
          "non-positive count returns without runtime effects");
    Check(collection.Read(0x20) == 0x6000,
          "non-positive count preserves cursor fields");
}

void ClearInvokesEachObjectFreesNodesAndResetsSentinelAndState() {
    pal4::CallbackCollectionState collection;
    collection.Write(0x18, 2);
    collection.Write(0x20, 0x7777);
    collection.Write(0x24, 0x5000);
    collection.Write(0x1c, 77);
    FakeRuntime runtime;
    runtime.memory[0x5004] = 0x6000;
    runtime.memory[0x6004] = 0x6100;
    runtime.memory[0x6008] = 0xaaaa;
    runtime.memory[0x600c] = 0x7000;
    runtime.memory[0x6104] = 0x5000;
    runtime.memory[0x6108] = 0xbbbb;
    runtime.memory[0x610c] = 0x7100;
    pal4::CallbackCollectionCursor::Clear(collection, runtime);

    Check(runtime.events == std::vector<std::string>{
                               "invoke:28676:43690",
                               "free:24576", "invoke:28932:48059",
                               "free:24832", "write:20480:20480",
                               "write:20484:20480"},
          "clear invokes and frees each element, then resets sentinel");
    Check(runtime.reads.size() >= 2 && runtime.reads[0] == 0x5004 &&
              runtime.reads[1] == 0x6008,
          "clear initializes cursor from sentinel previous node and reads payload");
    Check(collection.Read(0x10) == 0 && collection.Read(0x14) == 0 &&
              collection.Read(0x18) == 0 && collection.Read(0x1c) == 0 &&
              collection.Read(0x20) == 0x5000,
          "clear resets observed counters and leaves current at sentinel");
}

void DestroyStorageWalksRemainingNodesThenDestroysSentinelAndTail() {
    pal4::CallbackCollectionState collection;
    collection.Write(0x20, 0x7777);
    collection.Write(0x24, 0x5000);
    FakeRuntime runtime;
    runtime.memory[0x5004] = 0x6000;
    runtime.memory[0x6004] = 0x5000;
    runtime.memory[0x6008] = 0xaaaa;
    runtime.memory[0x600c] = 0x7000;
    runtime.memory[0x5008] = 0xbbbb;
    runtime.memory[0x500c] = 0x7100;
    std::uint32_t tail = 0x8000;

    pal4::CallbackCollectionCursor::DestroyStorage(collection, tail, runtime);

    Check(runtime.events == std::vector<std::string>{
              "invoke:28676:43690", "free:24576",
              "invoke:28932:48059", "free:20480", "free:32768"},
          "destructor invokes and frees remaining nodes, sentinel payload, then tail object");
    Check(collection.Read(0x20) == 0x5000 && tail == 0,
          "storage destruction starts from sentinel predecessor and clears tail field");
}

void DestroyRunsCollectionClearBeforeSentinelDestruction() {
    pal4::CallbackCollectionState collection;
    collection.Write(0x18, 1);
    collection.Write(0x20, 0x6000);
    collection.Write(0x24, 0x5000);
    FakeRuntime runtime;
    runtime.memory[0x5004] = 0x6000;
    runtime.memory[0x6004] = 0x5000;
    runtime.memory[0x6008] = 0xaaaa;
    runtime.memory[0x600c] = 0x7000;
    runtime.memory[0x5008] = 0xbbbb;
    runtime.memory[0x500c] = 0x7100;
    std::uint32_t tail = 0;
    pal4::CallbackCollectionCursor::Destroy(collection, tail, runtime);

    Check(runtime.events == std::vector<std::string>{
              "invoke:28676:43690", "free:24576",
              "write:20480:20480", "write:20484:20480",
              "invoke:28932:48059", "free:20480"},
          "combined destructor clears elements before destroying sentinel");
}

void DestroyRunsStorageCleanupAfterClearThrows() {
    pal4::CallbackCollectionState collection;
    collection.Write(0x18, 1);
    collection.Write(0x20, 0x6000);
    collection.Write(0x24, 0x5000);
    FakeRuntime runtime;
    runtime.memory[0x5004] = 0x6000;
    runtime.memory[0x6004] = 0x5000;
    runtime.memory[0x6008] = 0xaaaa;
    runtime.memory[0x600c] = 0x7000;
    runtime.memory[0x5008] = 0xbbbb;
    runtime.memory[0x500c] = 0x7100;
    runtime.throw_once_for_object = 0xaaaa;
    std::uint32_t tail = 0x8000;
    bool caught = false;
    try {
        pal4::CallbackCollectionCursor::Destroy(collection, tail, runtime);
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught && runtime.events == std::vector<std::string>{
              "invoke:28676:43690", "invoke:28676:43690",
              "free:24576", "invoke:28932:48059", "free:20480",
              "free:32768"} &&
              collection.Read(0x20) == 0x5000 && tail == 0,
          "clear exception retries the remaining node before sentinel and tail cleanup");
}

}  // namespace

int main() {
    SetPointersAndAdvanceThroughSentinel();
    EmptyInitializationStillCopiesSentinelFirst();
    ReverseInitializationUsesSentinelPreviousAndCount();
    SafeLookupUsesReverseOrderAndIncludesBothValidBounds();
    SafeLookupRejectsNegativeAndUpperBoundIndices();
    LookupStopsWhenReverseLinksReachNullPayload();
    ClearSkipsWhenCountIsNotPositive();
    ClearInvokesEachObjectFreesNodesAndResetsSentinelAndState();
    DestroyStorageWalksRemainingNodesThenDestroysSentinelAndTail();
    DestroyRunsCollectionClearBeforeSentinelDestruction();
    DestroyRunsStorageCleanupAfterClearThrows();
    std::cout << "11 callback-collection cursor tests passed\n";
}
