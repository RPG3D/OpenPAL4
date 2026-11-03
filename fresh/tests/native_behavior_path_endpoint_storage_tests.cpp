#include "pal4/native_behavior_path_endpoint_storage.h"
#include "pal4/native_base_object_memory.h"

#include <bit>
#include <cstring>
#include <iostream>
#include <stdexcept>

namespace {
void Check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}
}  // namespace

int main() {
    try {
        pal4::NativeBaseObjectMemory memory;
        std::uint32_t next_identifier = 37u;
        bool reject_cleanup = false;
        std::uint32_t destroyed = 0u;
        pal4::NativeBehaviorPathEndpointStorage storage(memory, next_identifier,
            [&](std::uint32_t association, bool free_self) {
                Check(free_self, "nested association uses deleting cleanup");
                if (reject_cleanup) throw std::logic_error("association cleanup unavailable");
                ++destroyed;
                (void)memory.Free(association);
            });
        const auto allocation = memory.Allocate(0xe0u + 16u);
        std::memset(reinterpret_cast<void*>(allocation), 0xa5, 0xe0u + 16u);
        const auto address = allocation + 8u;
        auto& image = *reinterpret_cast<pal4::BehaviorPathEndpointState*>(address);
        storage.Construct(address);
        const auto f = [&](std::uint32_t word) { return std::bit_cast<float>(image.words[word]); };
        Check(image.words[0] == 0x842898u && f(1) == 1.0f && f(5) == 1.0f &&
                  f(9) == 1.0f && f(2) == 0.0f && f(3) == 0.0f &&
                  image.words[22] == 37u && next_identifier == 38u,
              "native constructor uses reordered identity basis and shared next ID");
        Check(f(23) == 1.0f && f(24) == 0.5f && f(25) == 0.0f &&
                  f(26) == 0.1f && f(27) == 1.0f,
              "original five constructor slots initialize actual scalar words");
        Check(image.words[42] == 0u && image.words[43] == 0u &&
                  image.words[44] == 0u && image.words[45] == 0u &&
                  memory.ReadByte(address + 0xb8u) == 0u &&
                  memory.ReadByte(address + 0xb9u) == 0u &&
                  memory.ReadByte(address + 0xbau) == 0xa5u &&
                  memory.ReadByte(address + 0xbbu) == 0xa5u &&
                  image.words[47] == 0u && image.words[48] == 0xa5a5a5a5u &&
                  image.words[49] == 0xa5a50000u,
              "derived fields initialize without erasing original untouched bytes");
        for (std::uint32_t word = 50u; word < 56u; ++word)
            Check(image.words[word] == 0u, "cached endpoint vectors are cleared");
        for (std::uint32_t byte = 0u; byte < 8u; ++byte)
            Check(memory.ReadByte(allocation + byte) == 0xa5u &&
                      memory.ReadByte(address + 0xe0u + byte) == 0xa5u,
                  "constructor stays within original 0xE0 extent");
        pal4::NativeBehaviorPathEndpointStorage::SetScalar(image, 0x68u, 123.5f);
        Check(f(25) == 123.5f && f(27) == 1.0f,
              "restore scalar setter reuses original speed slot");
        const auto before_invalid = image.words;
        bool invalid_slot = false;
        try { pal4::NativeBehaviorPathEndpointStorage::SetScalar(image, 0u, 2.0f); }
        catch (const std::invalid_argument&) { invalid_slot = true; }
        Check(invalid_slot && image.words == before_invalid,
              "unknown scalar slot fails without altering helper image");

        image.words[42] = memory.Allocate(36u);
        image.words[43] = memory.Allocate(24u);
        image.words[47] = 0x1234u;
        reject_cleanup = true;
        bool rejected_cleanup = false;
        try { storage.Destruct(address); }
        catch (const std::logic_error&) { rejected_cleanup = true; }
        Check(rejected_cleanup && image.words[0] == 0x84b600u &&
                  image.words[47] == 0u && image.words[42] != 0u &&
                  image.words[43] != 0u && memory.OutstandingAllocations() == 3u,
              "failed association cleanup retains ownership and original destructor progress");
        reject_cleanup = false;
        storage.Destruct(address);
        Check(destroyed == 1u && image.words[0] == 0x84b6e8u &&
                  image.words[42] == 0u && image.words[43] == 0u &&
                  memory.OutstandingAllocations() == 1u,
              "native destructor releases association and owned buffer, preserving outer storage");
        storage.Destruct(address);
        Check(destroyed == 1u && memory.OutstandingAllocations() == 1u,
              "repeated helper cleanup does not release owned buffers twice");
        pal4::NativeBehaviorPathEndpointStorage another(memory, next_identifier,
            [](std::uint32_t, bool) { throw std::logic_error("unexpected association"); });
        another.Construct(address);
        Check(image.words[22] == 38u && next_identifier == 39u,
              "separate helper storage adapters share identifier ownership");
        another.Destruct(address);
        (void)memory.Free(allocation);
        Check(memory.OutstandingAllocations() == 0u, "native helper test leaves no owned allocations");
        std::cout << "PASS: native endpoint helper construction, scalar dispatch and ownership cleanup\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
