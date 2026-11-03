#include "pal4/native_actor_message_owner.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

const std::uint32_t* Words(std::uint32_t address) {
    return reinterpret_cast<const std::uint32_t*>(
        static_cast<std::uintptr_t>(address));
}

void TestLifecycle() {
    pal4::NativeActorMessageOwner owner;
    Check(owner.Construct() != 0u && owner.OutstandingAllocations() == 3u,
          "native adapter, sentinel and factory record are owned");
    const auto address = owner.Address();
    const auto* object = Words(address);
    const auto sentinel = object[9u];
    const auto* node = Words(sentinel);
    const auto* record = Words(node[2u]);
    Check(object[0u] == pal4::ActorMessageAdapter::kCreateCallback &&
              object[1u] == pal4::ActorMessageAdapter::kFreeCallback &&
              object[2u] == pal4::ActorMessageAdapter::kCopyCallback &&
              object[8u] == sentinel && node[0u] == sentinel &&
              node[1u] == sentinel && node[3u] == address &&
              record[0u] == 0u && record[16u] == 0u &&
              record[17u] == 0u,
          "native collection matches the IDA factory and sentinel layout");
    owner.Clear();
    Check(owner.Address() == address && owner.OutstandingAllocations() == 3u &&
              Words(sentinel)[0u] == sentinel,
          "Scene entry clear retains the empty collection storage");
    bool rejected = false;
    try {
        static_cast<void>(owner.Construct());
    } catch (const std::logic_error&) {
        rejected = true;
    }
    Check(rejected, "duplicate native construction is rejected");
    owner.Destroy();
    Check(owner.Address() == 0u && owner.OutstandingAllocations() == 0u,
          "native adapter releases record, sentinel and object");
    Check(owner.Construct() != 0u && owner.OutstandingAllocations() == 3u,
          "native adapter can be constructed after teardown");
}

}  // namespace

int main() {
    TestLifecycle();
    std::cout << "native actor message owner lifecycle passed\n";
}
