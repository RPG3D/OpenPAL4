#include "pal4/cegui_stream_io.h"

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

class Runtime final : public pal4::CeguiStreamIoRuntimeStub {
public:
    std::vector<std::string> calls;
    std::vector<std::uint32_t> reads;
    std::vector<std::uint32_t> writes;
    std::uint32_t clear_address = 0u;
    std::uint32_t clear_flags = 0u;
    bool clear_rethrow = false;
    bool process_result = false;
    std::uint32_t copied = 0u;
    bool init_throws = false;
    std::uint32_t init_address = 0u;
    std::uint32_t standard_address = 0u;
    std::uint8_t fill_byte = 0u;

    std::uint32_t ReadDword(std::uint32_t address) override {
        calls.emplace_back("read");
        reads.push_back(address);
        return pal4::CeguiStreamIoRuntimeStub::ReadDword(address);
    }
    void WriteDword(std::uint32_t address,
                    std::uint32_t value) override {
        calls.emplace_back("write");
        writes.push_back(address);
        pal4::CeguiStreamIoRuntimeStub::WriteDword(address, value);
    }
    std::uint32_t DestroyIosBase(std::uint32_t address) override {
        Check(address == 0x4000u &&
                  pal4::CeguiStreamIoRuntimeStub::ReadDword(address) ==
                      pal4::CeguiStreamIO::kIosVtable,
              "base destructor sees installed ios vtable");
        calls.emplace_back("destroy_ios_base");
        return 0xfeedu;
    }
    void ClearIos(std::uint32_t address, std::uint32_t flags,
                  bool rethrow) override {
        calls.emplace_back("clear");
        clear_address = address;
        clear_flags = flags;
        clear_rethrow = rethrow;
    }
    bool ProcessCharacterInput(std::uint32_t stream,
                                std::uint32_t mode) override {
        Check(stream == 0x6000u && mode == 1u &&
                  pal4::CeguiStreamIoRuntimeStub::ReadDword(0x6004u) == 0u,
              "sentry sees reset read count and mode one");
        calls.emplace_back("sentry");
        return process_result;
    }
    std::uint32_t ReadStreamBuffer(
        std::uint32_t streambuf, std::uint32_t buffer,
        std::uint32_t bytes) override {
        Check(streambuf == 0x8000u && buffer == 0x9000u &&
                  bytes == 11u,
              "streambuf read receives derived pointer and arguments");
        calls.emplace_back("streambuf_read");
        return copied;
    }
    void DestroyCeguiIOStream(std::uint32_t stream) override {
        Check(stream == 0x4008u, "CEGUI base destructor receives stream");
        calls.emplace_back("destroy_cegui");
    }
    void Free(std::uint32_t address) override {
        calls.emplace_back("free");
        writes.push_back(address);
    }
    void InitializeIosBase(std::uint32_t address) override {
        calls.emplace_back("init_ios");
        init_address = address;
        if (init_throws) {
            throw std::runtime_error("ios initialization");
        }
    }
    void AddStandardStream(std::uint32_t address) override {
        calls.emplace_back("add_std");
        standard_address = address;
    }
    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override {
        calls.emplace_back("write_byte");
        Check(address == init_address + 48u || init_address == 0u,
              "fill byte is stored in ios base");
        fill_byte = value;
        CeguiStreamIoRuntimeStub::WriteByte(address, value);
    }
    void Seed(std::uint32_t address, std::uint32_t value) {
        pal4::CeguiStreamIoRuntimeStub::WriteDword(address, value);
    }
    std::uint32_t Peek(std::uint32_t address) {
        return pal4::CeguiStreamIoRuntimeStub::ReadDword(address);
    }
};

void IosDestructorInstallsVtableThenDelegates() {
    Runtime runtime;
    Check(pal4::CeguiStreamIO::DestroyIos(0x4000u, runtime) ==
              0xfeedu &&
              runtime.calls == std::vector<std::string>{
                  "write", "destroy_ios_base"} &&
              runtime.writes == std::vector<std::uint32_t>{0x4000u},
          "ios destructor forwards base result after vtable write");
}

void SetStateHandlesZeroAndMissingStreambuf() {
    Runtime runtime;
    pal4::CeguiStreamIO::SetState(0x4000u, 0u, true, runtime);
    Check(runtime.calls.empty(), "zero state does no reads or clear");
    runtime.Seed(0x4004u, 1u);
    runtime.Seed(0x4028u, 0u);
    pal4::CeguiStreamIO::SetState(0x4000u, 2u, true, runtime);
    Check(runtime.clear_address == 0x4000u &&
              runtime.clear_flags == 7u && runtime.clear_rethrow &&
              runtime.reads == std::vector<std::uint32_t>{
                  0x4004u, 0x4028u},
          "missing streambuf adds badbit to combined flags");
    runtime.Seed(0x4028u, 0x9000u);
    runtime.calls.clear();
    pal4::CeguiStreamIO::SetState(0x4000u, 2u, false, runtime);
    Check(runtime.clear_flags == 3u && !runtime.clear_rethrow &&
              runtime.calls == std::vector<std::string>{
                  "read", "read", "clear"},
          "present streambuf forwards old and requested flags");
}

void SetStateWithoutPreviousFlagsAlwaysClears() {
    Runtime runtime;
    runtime.Seed(0x4028u, 0u);
    runtime.Seed(0x4004u, 0x20u);
    pal4::CeguiStreamIO::SetStateWithoutPreviousFlags(
        0x4000u, 0u, true, runtime);
    Check(runtime.calls == std::vector<std::string>{"read", "clear"} &&
              runtime.reads == std::vector<std::uint32_t>{0x4028u} &&
              runtime.clear_flags == 4u && runtime.clear_rethrow,
          "null streambuf adds badbit and clears even zero input state");
    runtime.Seed(0x4028u, 0x9000u);
    runtime.calls.clear();
    runtime.reads.clear();
    pal4::CeguiStreamIO::SetStateWithoutPreviousFlags(
        0x4000u, 2u, false, runtime);
    Check(runtime.calls == std::vector<std::string>{"read", "clear"} &&
              runtime.clear_flags == 2u && !runtime.clear_rethrow,
          "present streambuf forwards only requested flags");
}

void IstreamSubobjectDestructorUsesVtableOffset() {
    Runtime runtime;
    runtime.Seed(0x4000u, 0x5000u);
    runtime.Seed(0x5004u, 0x20u);
    Check(pal4::CeguiStreamIO::DestroyIstreamSubobject(
              0x4008u, runtime) == 0x5000u &&
              runtime.reads == std::vector<std::uint32_t>{
                  0x4000u, 0x5004u} &&
              runtime.writes == std::vector<std::uint32_t>{0x4020u} &&
              runtime.ReadDword(0x4020u) ==
                  pal4::CeguiStreamIO::kIstreamVtable,
          "subobject destructor writes imported istream vtable at dynamic offset");
}

void SeedReadState(Runtime& runtime) {
    runtime.Seed(0x6000u, 0x7000u);
    runtime.Seed(0x6004u, 99u);
    runtime.Seed(0x7004u, 0x20u);
    runtime.Seed(0x6024u, 0x10u);
    runtime.Seed(0x6048u, 0x8000u);
}

void DeniedInputResetsCountWithoutReading() {
    Runtime runtime;
    SeedReadState(runtime);
    Check(pal4::CeguiStreamIO::Read(
              0x6000u, 0x9000u, 11u, runtime) == 0x6000u &&
              runtime.Peek(0x6004u) == 0u &&
              runtime.calls == std::vector<std::string>{
                  "write", "sentry"},
          "failed sentry skips streambuf and status update");
}

void FullReadPreservesStatus() {
    Runtime runtime;
    SeedReadState(runtime);
    runtime.process_result = true;
    runtime.copied = 11u;
    Check(pal4::CeguiStreamIO::Read(
              0x6000u, 0x9000u, 11u, runtime) == 0x6000u &&
              runtime.Peek(0x6004u) == 11u &&
              runtime.clear_address == 0u &&
              runtime.calls.back() == "write",
          "full read stores count without changing status");
}

void ShortReadSetsEofAndFailBits() {
    Runtime runtime;
    SeedReadState(runtime);
    runtime.process_result = true;
    runtime.copied = 7u;
    Check(pal4::CeguiStreamIO::Read(
              0x6000u, 0x9000u, 11u, runtime) == 0x6000u &&
              runtime.Peek(0x6004u) == 7u &&
              runtime.clear_address == 0x6020u &&
              runtime.clear_flags == 0x13u &&
              !runtime.clear_rethrow &&
              runtime.calls.back() == "clear",
          "short read combines EOF/fail bits with current ios state");
}

void CeguiIstreamDestructorWrappers() {
    Runtime runtime;
    Check(pal4::CeguiStreamIO::DestroyCeguiIstream(
              0x4008u, 2u, runtime) == 0x4008u &&
              runtime.calls == std::vector<std::string>{"destroy_cegui"},
          "direct CEGUI destructor skips free when low flag is clear");
    runtime.calls.clear();
    Check(pal4::CeguiStreamIO::DestroyCeguiIstream(
              0x4008u, 3u, runtime) == 0x4008u &&
              runtime.calls == std::vector<std::string>{
                  "destroy_cegui", "free"} &&
              runtime.writes.back() == 0x4008u,
          "direct CEGUI destructor frees stream when low flag is set");
    runtime.calls.clear();
    runtime.writes.clear();
    runtime.Seed(0x4000u, 0x5000u);
    runtime.Seed(0x5004u, 0x20u);
    Check(pal4::CeguiStreamIO::DestroyCeguiIstreamWrapper(
              0x4008u, 0u, runtime) == 0x4000u &&
              runtime.Peek(0x4020u) ==
                  pal4::CeguiStreamIO::kIstreamVtable &&
              runtime.calls == std::vector<std::string>{
                  "read", "read", "write", "destroy_cegui"},
          "wrapper installs dynamic istream vtable before base destruction");
    runtime.calls.clear();
    Check(pal4::CeguiStreamIO::DestroyCeguiIstreamWrapper(
              0x4008u, 1u, runtime) == 0x4000u &&
              runtime.calls == std::vector<std::string>{
                  "read", "read", "write", "destroy_cegui", "free"} &&
              runtime.writes.back() == 0x4000u,
          "wrapper frees containing object and returns its address");
}

void IstreamConstructorModes() {
    Runtime runtime;
    runtime.Seed(pal4::CeguiStreamIO::kCeguiIstreamBaseVtable + 4u, 8u);
    Check(pal4::CeguiStreamIO::ConstructIstream(
              0x4000u, 0x9000u, true, true, runtime) == 0x4000u &&
              runtime.Peek(0x4000u) ==
                  pal4::CeguiStreamIO::kCeguiIstreamBaseVtable &&
              runtime.Peek(0x4008u) ==
                  pal4::CeguiStreamIO::kIstreamVtable &&
              runtime.Peek(0x402cu) == 0u &&
              runtime.Peek(0x4030u) == 0x9000u &&
              runtime.init_address == 0x4008u &&
              runtime.standard_address == 0x4008u &&
              runtime.fill_byte == 0x20u &&
              runtime.calls.back() == "add_std",
          "base construction installs vtables, streambuf and fill byte");

    Runtime existing;
    existing.Seed(0x4000u, 0xa000u);
    existing.Seed(0xa004u, 0x20u);
    existing.Seed(0x4024u, 0x10u);
    Check(pal4::CeguiStreamIO::ConstructIstream(
              0x4000u, 0u, false, false, existing) == 0x4000u &&
              existing.Peek(0x4000u) == 0xa000u &&
              existing.Peek(0x4020u) ==
                  pal4::CeguiStreamIO::kIstreamVtable &&
              existing.Peek(0x4048u) == 0u &&
              existing.init_address == 0x4020u &&
              existing.clear_flags == 0x14u &&
              !existing.clear_rethrow &&
              existing.standard_address == 0u &&
              existing.calls.back() == "clear",
          "existing-base mode sets badbit for null streambuf");
}

void IstreamConstructorUnwindsBaseOnlyWhenOwned() {
    Runtime owned;
    owned.Seed(pal4::CeguiStreamIO::kCeguiIstreamBaseVtable + 4u, 8u);
    owned.init_throws = true;
    bool threw = false;
    try {
        (void)pal4::CeguiStreamIO::ConstructIstream(
            0x4000u, 0x9000u, false, true, owned);
    } catch (const std::runtime_error&) {
        threw = true;
    }
    Check(threw && owned.calls.size() >= 2u &&
              owned.calls[owned.calls.size() - 2u] == "init_ios" &&
              owned.calls.back() == "destroy_cegui",
          "owned base is destroyed when ios initialization throws");
    Runtime borrowed;
    borrowed.Seed(0x4000u, 0xa000u);
    borrowed.Seed(0xa004u, 0x20u);
    borrowed.init_throws = true;
    threw = false;
    try {
        (void)pal4::CeguiStreamIO::ConstructIstream(
            0x4000u, 0x9000u, false, false, borrowed);
    } catch (const std::runtime_error&) {
        threw = true;
    }
    Check(threw && borrowed.calls.back() == "init_ios",
          "borrowed base remains owned by caller on constructor failure");
}

}  // namespace

int main() {
    IosDestructorInstallsVtableThenDelegates();
    SetStateHandlesZeroAndMissingStreambuf();
    SetStateWithoutPreviousFlagsAlwaysClears();
    IstreamSubobjectDestructorUsesVtableOffset();
    DeniedInputResetsCountWithoutReading();
    FullReadPreservesStatus();
    ShortReadSetsEofAndFailBits();
    CeguiIstreamDestructorWrappers();
    IstreamConstructorModes();
    IstreamConstructorUnwindsBaseOnlyWhenOwned();
    std::cout << "10 CEGUI stream I/O test cases passed\n";
}
