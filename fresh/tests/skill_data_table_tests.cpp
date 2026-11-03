#include "pal4/skill_data_table.h"

#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Call {
    std::string kind;
    std::uint32_t address;
    std::uint32_t value;
    bool operator==(const Call&) const = default;
};

struct Runtime final : pal4::ISkillDataTableRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<Call> calls;
    std::uint32_t allocation = 0x1000;
    std::uint32_t mutate_skill_end = 0;
    std::uint32_t fail_skill_record = 0;

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
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        bytes[address] = value;
        calls.push_back({"byte", address, value});
    }
    std::uint32_t ClearEffectData(std::uint32_t object) override {
        calls.push_back({"clear_effect", object, 0});
        return 0x55;
    }
    std::uint32_t DestroySkillRecord(std::uint32_t record) override {
        calls.push_back({"destroy_skill", record, 0});
        if (record == fail_skill_record)
            throw std::runtime_error("skill destruction failed");
        return record + 1;
    }
    void DestroyPropertyRecord(std::uint32_t record) override {
        calls.push_back({"destroy_property", record, 0});
    }
    void ProcessElements(std::uint32_t address) override {
        calls.push_back({"process_elements", address, 0});
        if (mutate_skill_end)
            words[address - 108 + 100] = mutate_skill_end;
    }
    void Free(std::uint32_t address) override {
        calls.push_back({"free", address, 0});
    }
    std::uint32_t DestroyBase(std::uint32_t object) override {
        calls.push_back({"destroy_base", object, 0});
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
    Check(pal4::SkillDataTable::Create(0x1234, runtime) == 0x1000 &&
              runtime.calls.front() ==
                  Call{"allocate", 140, 0x1000} &&
              runtime.calls[1] ==
                  Call{"construct_base", 0x1000, 0x1234} &&
              runtime.calls.back() ==
                  Call{"word", 0x1000, 0x8446ec},
          "create allocates, constructs base, initializes fields and vtable");
    for (auto offset : {92u, 108u, 124u}) {
        Check(runtime.bytes[0x1000 + offset] == 0x34,
              "text seed byte uses low byte of name pointer");
        for (auto word_offset : {4u, 8u, 12u})
            Check(runtime.words[0x1000 + offset + word_offset] == 0,
                  "each array triplet is initialized to zero");
    }
    Runtime failed;
    failed.allocation = 0;
    Check(pal4::SkillDataTable::Create(0x1234, failed) == 0 &&
              failed.calls ==
                  std::vector<Call>{{"allocate", 140, 0}},
          "failed allocation skips base and fields");
}

void CheckDestroy() {
    Runtime runtime;
    runtime.words[0x1000 + 96] = 0x2000;
    runtime.words[0x1000 + 100] = 0x2000 + 264;
    runtime.words[0x1000 + 112] = 0x3000;
    runtime.words[0x1000 + 116] = 0x3000 + 120;
    runtime.words[0x1000 + 128] = 0x4000;
    runtime.mutate_skill_end = 0x2000 + 132;
    Check(pal4::SkillDataTable::Destroy(0x1000, runtime) == 0xbeef &&
              runtime.calls.front() ==
                  Call{"word", 0x1000, 0x8446ec} &&
              runtime.calls[1] ==
                  Call{"clear_effect", 0x1000, 0} &&
              runtime.calls.back() ==
                  Call{"destroy_base", 0x1000, 0},
          "destruction sets derived vtable and returns base result");
    Check(Addresses(runtime, "destroy_skill") ==
              std::vector<std::uint32_t>{0x2000, 0x2000 + 132, 0x2000} &&
              Addresses(runtime, "destroy_property") ==
                  std::vector<std::uint32_t>{0x3000, 0x3000 + 60} &&
              Addresses(runtime, "free") ==
                  std::vector<std::uint32_t>{0x4000, 0x3000, 0x2000},
          "ranges and frees follow native order with post-process reread");
    for (auto offset : {96u, 100u, 104u, 112u, 116u, 120u,
                        128u, 132u, 136u})
        Check(runtime.words[0x1000 + offset] == 0,
              "all three pointer triplets are reset");
    Runtime interrupted;
    interrupted.words[0x1000 + 96] = 0x2000;
    interrupted.words[0x1000 + 100] = 0x2000 + 132;
    interrupted.fail_skill_record = 0x2000;
    try {
        static_cast<void>(pal4::SkillDataTable::Destroy(
            0x1000, interrupted));
        Check(false, "record destruction failure must propagate");
    } catch (const std::runtime_error& error) {
        Check(std::string(error.what()) == "skill destruction failed" &&
                  Addresses(interrupted, "free").empty() &&
                  Addresses(interrupted, "destroy_skill") ==
                      std::vector<std::uint32_t>{0x2000},
              "failure stops before array release and base destructor");
    }
}

void CheckArrayAndClear() {
    Runtime runtime;
    runtime.words[0x5000 + 4] = 0x6000;
    runtime.words[0x5000 + 8] = 0x6000 + 264;
    runtime.words[0x5000 + 12] = 0x7000;
    Check(pal4::SkillDataTable::ReleaseRecordArray(
              0x5000, runtime) == 0 &&
              Addresses(runtime, "destroy_skill") ==
                  std::vector<std::uint32_t>{0x6000, 0x6000 + 132} &&
              Addresses(runtime, "free") ==
                  std::vector<std::uint32_t>{0x6000} &&
              runtime.words[0x5004] == 0 &&
              runtime.words[0x5008] == 0 &&
              runtime.words[0x500c] == 0,
          "standalone array destroys rows, frees and zeros pointers");

    Runtime retained;
    retained.words[0x1000 + 96] = 0x2000;
    retained.words[0x1000 + 100] = 0x2000 + 264;
    Check(pal4::SkillDataTable::ClearRows(0x1000, retained) ==
              0x2000 + 132 + 1 &&
              retained.words[0x1000 + 96] == 0x2000 &&
              retained.words[0x1000 + 100] == 0x2000 &&
              Addresses(retained, "free").empty(),
          "clear forwards last destructor result and retains capacity");
    retained.calls.clear();
    Check(pal4::SkillDataTable::ClearRows(0x1000, retained) == 0x55 &&
              Addresses(retained, "destroy_skill").empty(),
          "empty clear forwards Effect clear result");
}

}  // namespace

int main() {
    try {
        CheckCreate();
        CheckDestroy();
        CheckArrayAndClear();
        std::cout << "skill_data_table_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
