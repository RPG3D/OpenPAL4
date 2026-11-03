#include "pal4/cegui_text_record_array.h"

#include <cstdlib>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

class Memory final : public pal4::ICeguiTextRecordArrayRuntime {
public:
    std::uint8_t ReadByte(std::uint32_t address) override {
        if ((address & 15u) == 0u) {
            last_record_source = address;
        }
        return bytes[address];
    }
    std::uint32_t ReadDword(std::uint32_t address) override {
        read_dwords.push_back(address);
        return static_cast<std::uint32_t>(bytes[address]) |
               (static_cast<std::uint32_t>(bytes[address + 1u]) << 8u) |
               (static_cast<std::uint32_t>(bytes[address + 2u]) << 16u) |
               (static_cast<std::uint32_t>(bytes[address + 3u]) << 24u);
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        bytes[address] = value;
    }
    void WriteDword(std::uint32_t address, std::uint32_t value) override {
        for (std::uint32_t index = 0u; index < 4u; ++index) {
            bytes[address + index] =
                static_cast<std::uint8_t>(value >> (8u * index));
        }
    }
    std::uint32_t EmptyTextAddress() override {
        return pal4::CeguiTextRecord::kEmptyTextAddress;
    }
    void SetErrorFlag(std::uint32_t record,
                      std::uint8_t flag) override {
        Check(flag == 1u, "old records and shared copies set error flag 1");
        marked.push_back(record);
    }
    bool AllocateText(std::uint32_t record, std::uint32_t size,
                      std::uint8_t mode) override {
        allocations.push_back({record, size, mode});
        record_copies.push_back({record, last_record_source});
        if (allocate_text_result) {
            WriteDword(record + 4u, next_text_address);
        }
        return allocate_text_result;
    }
    void DetachAndReset(std::uint32_t record) override {
        detached.push_back(record);
        WriteDword(record + 4u, 0u);
        WriteDword(record + 8u, 0u);
        WriteDword(record + 12u, 0u);
    }
    void SetTextSize(std::uint32_t record,
                     std::uint32_t size) override {
        sized.push_back({record, size});
        WriteDword(record + 8u, size);
    }
    void CopyBytes(std::uint32_t destination,
                   std::uint32_t source, std::uint32_t size) override {
        byte_copies.push_back({destination, source, size});
        for (std::uint32_t index = 0u; index < size; ++index) {
            bytes[destination + index] = bytes[source + index];
        }
    }
    [[noreturn]] void ThrowRange() override {
        throw std::out_of_range("record self copy");
    }
    std::uint32_t AllocateRecords(std::uint32_t size) override {
        record_allocation_sizes.push_back(size);
        return next_record_allocation;
    }
    void FreeRecords(std::uint32_t address) override {
        freed.push_back(address);
    }
    void AssignRecord(std::uint32_t destination,
                      std::uint32_t source) override {
        assignments.push_back({destination, source});
    }

    void EmptyRecord(std::uint32_t address, std::uint8_t flag = 3u) {
        WriteByte(address, flag);
        WriteDword(address + 4u, 0u);
        WriteDword(address + 8u, 0u);
        WriteDword(address + 12u, 0u);
    }

    struct Allocation {
        std::uint32_t record;
        std::uint32_t size;
        std::uint8_t mode;
    };
    struct ByteCopy {
        std::uint32_t destination;
        std::uint32_t source;
        std::uint32_t size;
    };
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::uint32_t> marked;
    std::vector<std::uint32_t> read_dwords;
    std::vector<Allocation> allocations;
    std::vector<std::pair<std::uint32_t, std::uint32_t>> record_copies;
    std::vector<std::uint32_t> detached;
    std::vector<std::pair<std::uint32_t, std::uint32_t>> sized;
    std::vector<ByteCopy> byte_copies;
    std::vector<std::uint32_t> record_allocation_sizes;
    std::vector<std::uint32_t> freed;
    std::vector<std::pair<std::uint32_t, std::uint32_t>> assignments;
    std::uint32_t last_record_source{};
    std::uint32_t next_record_allocation{0x5000u};
    std::uint32_t next_text_address{0x6000u};
    bool allocate_text_result{};
};

void NullAndSharedRecordCopies() {
    Memory runtime;
    runtime.EmptyRecord(0x1000u, 7u);
    runtime.WriteDword(0x1004u, 0x4001u);
    runtime.WriteDword(0x1008u, 3u);
    runtime.WriteDword(0x100cu, 12u);
    runtime.WriteByte(0x4000u, 2u);
    pal4::CeguiTextRecord::Copy(0u, 0x1000u, runtime);
    Check(runtime.record_copies.empty() && runtime.marked.empty(),
          "null destination skips all record reads and writes");
    pal4::CeguiTextRecord::Copy(0x2000u, 0x1000u, runtime);
    Check(runtime.ReadByte(0x2000u) == 7u &&
              runtime.ReadDword(0x2004u) == 0x4001u &&
              runtime.ReadDword(0x2008u) == 3u &&
              runtime.ReadDword(0x200cu) == 12u &&
              runtime.ReadByte(0x4000u) == 3u,
          "shareable text pointer and metadata are copied and incref'd");
    Check(runtime.marked == std::vector<std::uint32_t>{0x2000u} &&
              runtime.allocations.empty(),
          "shared branch sets flag without allocating text");
}

void DeepAndFailedRecordCopies() {
    Memory runtime;
    runtime.EmptyRecord(0x1000u, 5u);
    runtime.WriteDword(0x1004u, 0x4001u);
    runtime.WriteDword(0x1008u, 3u);
    runtime.WriteByte(0x4000u, 0xfeu);
    runtime.WriteByte(0x4001u, 'a');
    runtime.WriteByte(0x4002u, 'b');
    runtime.WriteByte(0x4003u, 'c');
    runtime.allocate_text_result = true;
    pal4::CeguiTextRecord::Copy(0x2000u, 0x1000u, runtime);
    Check(runtime.allocations.size() == 1u &&
              runtime.allocations[0].record == 0x2000u &&
              runtime.allocations[0].size == 3u &&
              runtime.allocations[0].mode == 1u &&
              runtime.ReadDword(0x2004u) == 0x6000u &&
              runtime.ReadDword(0x2008u) == 3u &&
              runtime.ReadByte(0x6000u) == 'a' &&
              runtime.ReadByte(0x6002u) == 'c' &&
              runtime.ReadByte(0x6003u) == 0u,
          "saturated reference count takes deep copy and terminates text");
    Check(runtime.byte_copies.size() == 1u &&
              runtime.byte_copies[0].source == 0x4001u &&
              runtime.byte_copies[0].size == 3u,
          "deep copy uses exact source and length");
    runtime.allocate_text_result = false;
    pal4::CeguiTextRecord::Copy(0x3000u, 0x1000u, runtime);
    Check(runtime.ReadByte(0x3000u) == 5u &&
              runtime.ReadDword(0x3004u) == 0u &&
              runtime.ReadDword(0x3008u) == 0u &&
              runtime.byte_copies.size() == 1u,
          "allocation failure retains initialized empty destination");
}

void SelfCopyUsesObservedResetPath() {
    Memory runtime;
    runtime.EmptyRecord(0x1000u, 4u);
    runtime.WriteDword(0x1004u, 0x4001u);
    runtime.WriteDword(0x1008u, 3u);
    pal4::CeguiTextRecord::Copy(0x1000u, 0x1000u, runtime);
    Check(runtime.detached ==
              std::vector<std::uint32_t>{0x1000u, 0x1000u} &&
              runtime.ReadDword(0x1008u) == 0u &&
              runtime.allocations.empty(),
          "self copy reads zeroed size and calls detach twice");
}

void EmptySourceAndFailedAllocationSkipTextRead() {
    Memory runtime;
    runtime.EmptyRecord(0x1000u);
    runtime.WriteDword(0x1004u, 0x4001u);
    pal4::CeguiTextRecord::Copy(0x2000u, 0x1000u, runtime);
    Check(runtime.allocations.size() == 1u &&
              runtime.allocations[0].size == 0u &&
              runtime.read_dwords == std::vector<std::uint32_t>{0x1008u},
          "zero-size failed allocation does not read source text pointer");
}

void ReallocationInsertsAndReleasesOldRecords() {
    Memory runtime;
    runtime.EmptyRecord(0x1000u);
    runtime.EmptyRecord(0x1010u);
    runtime.EmptyRecord(0x3000u, 8u);
    pal4::CeguiTextRecordArrayState state{0xdeadbeefu, 0x1000u,
                                          0x1020u, 0x1020u};
    pal4::CeguiTextRecordArray::Insert(
        state, 0x1010u, 2u, 0x3000u, runtime);
    Check(runtime.record_allocation_sizes ==
              std::vector<std::uint32_t>{64u} &&
              state.begin == 0x5000u && state.end == 0x5040u &&
              state.capacity == 0x5040u && state.unused == 0xdeadbeefu,
          "capacity growth allocates old count plus max(old,count)");
    Check(runtime.record_copies ==
              std::vector<std::pair<std::uint32_t, std::uint32_t>>({
                  {0x5000u, 0x1000u}, {0x5010u, 0x3000u},
                  {0x5020u, 0x3000u}, {0x5030u, 0x1010u}}),
          "new array copies prefix, fill records and suffix in order");
    Check(runtime.marked ==
              std::vector<std::uint32_t>({0x1000u, 0x1010u}) &&
              runtime.freed == std::vector<std::uint32_t>{0x1000u},
          "old records are marked before vector storage is freed");

    Memory empty_runtime;
    empty_runtime.EmptyRecord(0x3000u);
    pal4::CeguiTextRecordArrayState empty{};
    pal4::CeguiTextRecordArray::Insert(
        empty, 0u, 1u, 0x3000u, empty_runtime);
    Check(empty.begin == 0x5000u && empty.end == 0x5010u &&
              empty.capacity == 0x5010u &&
              empty_runtime.freed == std::vector<std::uint32_t>{0u},
          "empty array allocates one record and still calls free on null old base");
}

void InPlaceBranchesAndZeroCount() {
    Memory short_tail;
    for (std::uint32_t address : {0x1000u, 0x1010u, 0x1020u,
                                  0x3000u}) {
        short_tail.EmptyRecord(address);
    }
    pal4::CeguiTextRecordArrayState short_state{
        0u, 0x1000u, 0x1030u, 0x1080u};
    pal4::CeguiTextRecordArray::Insert(
        short_state, 0x1020u, 3u, 0x3000u, short_tail);
    Check(short_state.end == 0x1060u &&
              short_tail.record_allocation_sizes.empty() &&
              short_tail.record_copies ==
                  std::vector<std::pair<std::uint32_t, std::uint32_t>>({
                      {0x1050u, 0x1020u}, {0x1030u, 0x3000u},
                      {0x1040u, 0x3000u}}) &&
              short_tail.assignments ==
                  std::vector<std::pair<std::uint32_t, std::uint32_t>>({
                      {0x1020u, 0x3000u}}),
          "short tail copies into free space then assigns old slots");

    Memory long_tail;
    for (std::uint32_t address : {0x1000u, 0x1010u, 0x1020u,
                                  0x1030u, 0x3000u}) {
        long_tail.EmptyRecord(address);
    }
    pal4::CeguiTextRecordArrayState long_state{
        0u, 0x1000u, 0x1040u, 0x1080u};
    pal4::CeguiTextRecordArray::Insert(
        long_state, 0x1010u, 1u, 0x3000u, long_tail);
    Check(long_state.end == 0x1050u &&
              long_tail.record_copies ==
                  std::vector<std::pair<std::uint32_t, std::uint32_t>>({
                      {0x1040u, 0x1030u}}) &&
              long_tail.assignments ==
                  std::vector<std::pair<std::uint32_t, std::uint32_t>>({
                      {0x1030u, 0x1020u}, {0x1020u, 0x1010u},
                      {0x1010u, 0x3000u}}),
          "long tail copies last record, shifts backward, then fills hole");
    long_tail.record_copies.clear();
    long_tail.assignments.clear();
    pal4::CeguiTextRecordArray::Insert(
        long_state, 0x1020u, 0u, 0x3000u, long_tail);
    Check(long_state.end == 0x1050u &&
              long_tail.record_copies.empty() &&
              long_tail.assignments.empty(),
          "zero insertion makes no record calls");
}

void AllocationFailureFollowsObservedWrites() {
    Memory runtime;
    runtime.next_record_allocation = 0u;
    runtime.EmptyRecord(0x1000u);
    runtime.EmptyRecord(0x3000u);
    pal4::CeguiTextRecordArrayState state{0u, 0x1000u,
                                          0x1010u, 0x1010u};
    pal4::CeguiTextRecordArray::Insert(
        state, 0x1010u, 1u, 0x3000u, runtime);
    Check(runtime.record_allocation_sizes ==
              std::vector<std::uint32_t>{32u} &&
              runtime.record_copies ==
                  std::vector<std::pair<std::uint32_t, std::uint32_t>>({
                      {0x10u, 0x3000u}}) &&
              runtime.marked == std::vector<std::uint32_t>{0x1000u} &&
              runtime.freed == std::vector<std::uint32_t>{0x1000u} &&
              state.begin == 0u && state.end == 0x20u &&
              state.capacity == 0x20u,
          "null allocation does not stop later copy or pointer updates");
}

}  // namespace

int main() {
    NullAndSharedRecordCopies();
    DeepAndFailedRecordCopies();
    SelfCopyUsesObservedResetPath();
    EmptySourceAndFailedAllocationSkipTextRead();
    ReallocationInsertsAndReleasesOldRecords();
    InPlaceBranchesAndZeroCount();
    AllocationFailureFollowsObservedWrites();
    std::cout << "7 CEGUI text record array test cases passed\n";
}
