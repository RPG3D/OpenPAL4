#include "pal4/magic_data_table.h"

#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

struct Call {
    std::string kind;
    std::uint32_t address;
    std::uint32_t value = 0;
    bool operator==(const Call&) const = default;
};

struct Runtime final : pal4::IMagicDataTableRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<Call> calls;
    std::uint32_t allocation = 0x1000;
    std::uint32_t changed_end = 0;
    std::uint32_t fail_record = 0;

    std::uint32_t Allocate(std::uint32_t size) override {
        calls.push_back({"allocate", size, allocation});
        return allocation;
    }
    void ConstructBase(std::uint32_t object,
                       std::uint32_t name) override {
        calls.push_back({"construct_base", object, name});
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
        calls.push_back({"word", address, value});
    }
    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override {
        bytes[address] = value;
        calls.push_back({"byte", address, value});
    }
    std::uint32_t ClearEffectData(std::uint32_t object) override {
        calls.push_back({"clear_effect", object});
        return 0x55;
    }
    std::uint32_t DestroyMagicRecord(std::uint32_t record) override {
        calls.push_back({"destroy_magic", record});
        if (record == fail_record)
            throw std::runtime_error("magic destruction failed");
        return record + 1;
    }
    void DestroyPropertyRecord(std::uint32_t record) override {
        calls.push_back({"destroy_property", record});
    }
    void ProcessElements(std::uint32_t address) override {
        calls.push_back({"process_elements", address});
        if (changed_end) words[address - 108 + 100] = changed_end;
    }
    void Free(std::uint32_t address) override {
        calls.push_back({"free", address});
    }
    std::uint32_t DestroyBase(std::uint32_t object) override {
        calls.push_back({"destroy_base", object});
        return 0xbeef;
    }
};

std::vector<std::uint32_t> Addresses(const Runtime& runtime,
                                     const std::string& kind) {
    std::vector<std::uint32_t> result;
    for (const auto& call : runtime.calls)
        if (call.kind == kind) result.push_back(call.address);
    return result;
}

void CheckCreate() {
    Runtime runtime;
    Check(pal4::MagicDataTable::Create(0x1234, runtime) == 0x1000 &&
              runtime.calls.front() == Call{"allocate", 140, 0x1000} &&
              runtime.calls[1] ==
                  Call{"construct_base", 0x1000, 0x1234} &&
              runtime.calls.back() ==
                  Call{"word", 0x1000, 0x84472c},
          "constructor allocates, initializes base and installs Magic vtable");
    for (auto offset : {92u, 108u, 124u}) {
        Check(runtime.bytes[0x1000 + offset] == 0x34,
              "embedded collection seed uses low name byte");
        for (auto word_offset : {4u, 8u, 12u})
            Check(runtime.words[0x1000 + offset + word_offset] == 0,
                  "each embedded pointer triplet starts empty");
    }
    Runtime failed;
    failed.allocation = 0;
    Check(pal4::MagicDataTable::Create(0x1234, failed) == 0 &&
              failed.calls == std::vector<Call>{{"allocate", 140, 0}},
          "failed allocation skips base and field initialization");
}

void CheckDestroy() {
    Runtime runtime;
    runtime.words[0x1060] = 0x2000;
    runtime.words[0x1064] = 0x2000 + 2 * 152;
    runtime.words[0x1070] = 0x3000;
    runtime.words[0x1074] = 0x3000 + 2 * 60;
    runtime.words[0x1080] = 0x4000;
    runtime.changed_end = 0x2000 + 152;
    Check(pal4::MagicDataTable::Destroy(0x1000, runtime) == 0xbeef &&
              runtime.calls.front() ==
                  Call{"word", 0x1000, 0x84472c} &&
              runtime.calls[1] == Call{"clear_effect", 0x1000} &&
              runtime.calls.back() == Call{"destroy_base", 0x1000},
          "destructor restores vtable and forwards base result");
    Check(Addresses(runtime, "destroy_magic") ==
              std::vector<std::uint32_t>{
                  0x2000, 0x2000 + 152, 0x2000} &&
              Addresses(runtime, "destroy_property") ==
                  std::vector<std::uint32_t>{0x3000, 0x3000 + 60} &&
              Addresses(runtime, "free") ==
                  std::vector<std::uint32_t>{0x4000, 0x3000, 0x2000},
          "two Magic cleanup passes and property cleanup follow native order");
    for (auto offset : {96u, 100u, 104u, 112u, 116u, 120u,
                        128u, 132u, 136u})
        Check(runtime.words[0x1000 + offset] == 0,
              "all owned pointer triplets are zeroed");

    Runtime interrupted;
    interrupted.words[0x1060] = 0x2000;
    interrupted.words[0x1064] = 0x2000 + 152;
    interrupted.fail_record = 0x2000;
    try {
        static_cast<void>(pal4::MagicDataTable::Destroy(
            0x1000, interrupted));
        Check(false, "record destructor failure must propagate");
    } catch (const std::runtime_error& error) {
        Check(std::string(error.what()) == "magic destruction failed" &&
                  Addresses(interrupted, "free").empty() &&
                  Addresses(interrupted, "destroy_magic") ==
                      std::vector<std::uint32_t>{0x2000},
              "destructor failure stops before storage and base release");
    }
}

void CheckArrayAndClear() {
    Runtime runtime;
    runtime.words[0x5004] = 0x6000;
    runtime.words[0x5008] = 0x6000 + 2 * 152;
    Check(pal4::MagicDataTable::ReleaseRecordArray(
              0x5000, runtime) == 0 &&
              Addresses(runtime, "destroy_magic") ==
                  std::vector<std::uint32_t>{0x6000, 0x6000 + 152} &&
              Addresses(runtime, "free") ==
                  std::vector<std::uint32_t>{0x6000} &&
              runtime.words[0x5004] == 0 &&
              runtime.words[0x5008] == 0 &&
              runtime.words[0x500c] == 0,
          "standalone array destroys, frees and clears its pointer triplet");

    Runtime retained;
    retained.words[0x1060] = 0x2000;
    retained.words[0x1064] = 0x2000 + 2 * 152;
    Check(pal4::MagicDataTable::ClearRows(0x1000, retained) ==
              0x2000 + 152 + 1 &&
              retained.words[0x1060] == 0x2000 &&
              retained.words[0x1064] == 0x2000 &&
              Addresses(retained, "free").empty(),
          "clear retains capacity and returns last destructor result");
    retained.calls.clear();
    Check(pal4::MagicDataTable::ClearRows(0x1000, retained) == 0x55 &&
              Addresses(retained, "destroy_magic").empty(),
          "empty clear forwards Effect clear result");
}

}  // namespace

int main() {
    try {
        CheckCreate();
        CheckDestroy();
        CheckArrayAndClear();
        std::cout << "magic_data_table_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
