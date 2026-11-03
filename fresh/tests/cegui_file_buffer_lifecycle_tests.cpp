#include "pal4/cegui_file_buffer_lifecycle.h"

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

class Runtime final : public pal4::CeguiFileBufferRuntimeStub {
public:
    std::vector<std::string> calls;
    std::vector<std::uint32_t> destroyed;
    std::vector<std::uint32_t> freed;
    std::int32_t fclose_result = 0;
    std::uint32_t locale_release = 0u;

    std::uint32_t ReadDword(std::uint32_t address) override {
        calls.emplace_back("read_dword");
        return pal4::CeguiFileBufferRuntimeStub::ReadDword(address);
    }
    void WriteDword(std::uint32_t address,
                    std::uint32_t value) override {
        calls.emplace_back("write_dword");
        if (address == 0x4028u && value ==
            pal4::CeguiFileBufferLifecycle::kIstreamSubobjectVtable) {
            calls.emplace_back("istream_aux");
        }
        if (address == 0x4028u && value ==
            pal4::CeguiFileBufferLifecycle::kIstreamVtable) {
            calls.emplace_back("istream_final");
        }
        pal4::CeguiFileBufferRuntimeStub::WriteDword(address, value);
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        calls.emplace_back("read_byte");
        return pal4::CeguiFileBufferRuntimeStub::ReadByte(address);
    }
    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override {
        calls.emplace_back("write_byte");
        pal4::CeguiFileBufferRuntimeStub::WriteByte(address, value);
    }
    std::int32_t CloseCrtFile(std::uint32_t file) override {
        Check(file == 0x7000u, "fclose uses file pointer at +80");
        calls.emplace_back("fclose");
        return fclose_result;
    }
    void InitializeWideStreambuf(std::uint32_t object) override {
        Check(object == 0x4000u && Peek(0x4000u) ==
                  pal4::CeguiFileBufferLifecycle::kFilebufVtable,
              "wide streambuf reset runs under filebuf vtable");
        calls.emplace_back("init_wide");
    }
    std::uint32_t GlobalLocaleValue() override {
        calls.emplace_back("locale");
        return 0xdeadu;
    }
    void SetErrorFlag(std::uint32_t object,
                      std::uint8_t value) override {
        Check(object == 0xa000u && value == 1u &&
                  Peek(0x4000u) ==
                      pal4::CeguiFileBufferLifecycle::kFilebufVtable,
              "error-buffer flag set precedes final vtable transition");
        calls.emplace_back("error_flag");
    }
    void Free(std::uint32_t address) override {
        Check(address == 0xa000u || address == 0x4000u,
              "filebuf frees auxiliary storage or owned streambuf");
        calls.emplace_back("free");
        freed.push_back(address);
    }
    std::uint32_t DestroyReference(
        std::uint32_t object, std::uint8_t flags) override {
        Check(flags == 1u && Peek(object + 4u) == 0u,
              "reference destructor sees zero count and flag one");
        calls.emplace_back("destroy_ref");
        destroyed.push_back(object);
        return 0xbeefu;
    }
    void InitializeFileBuffer(std::uint32_t filebuf,
                              std::uint32_t first,
                              std::uint32_t mode) override {
        Check(filebuf == 0x4000u && first == 0u && mode == 2u &&
                  Peek(filebuf) ==
                      pal4::CeguiFileBufferLifecycle::kFilebufVtable,
              "filebuf initializer receives zero, two and filebuf vtable");
        calls.emplace_back("init_filebuf");
    }
    std::uint32_t DecrementLocaleFacet(
        std::uint32_t facet) override {
        Check(facet == 0xc000u && Peek(0x4000u) ==
                  pal4::CeguiFileBufferLifecycle::kStreambufVtable,
              "locale facet is released after streambuf vtable");
        calls.emplace_back("decref_facet");
        if (locale_release != 0u) {
            Seed(locale_release + 4u, 0u);
        }
        return locale_release;
    }
    void Seed(std::uint32_t address, std::uint32_t value) {
        pal4::CeguiFileBufferRuntimeStub::WriteDword(address, value);
    }
    void SeedByte(std::uint32_t address, std::uint8_t value) {
        pal4::CeguiFileBufferRuntimeStub::WriteByte(address, value);
    }
    std::uint32_t Peek(std::uint32_t address) {
        return pal4::CeguiFileBufferRuntimeStub::ReadDword(address);
    }
    std::uint8_t PeekByte(std::uint32_t address) {
        return pal4::CeguiFileBufferRuntimeStub::ReadByte(address);
    }
};

void SeedFileBuffer(Runtime& runtime) {
    runtime.SeedByte(0x4048u, 1u);
    runtime.Seed(0x4050u, 0x7000u);
    runtime.Seed(0x4044u, 0xa000u);
    runtime.Seed(0x404cu, 0x8000u);
    runtime.Seed(0x4034u, 0x9000u);
    runtime.Seed(0x8004u, 1u);
    runtime.Seed(0x9004u, 2u);
    runtime.Seed(0x4038u, 0x3333u);
    runtime.Seed(0x403cu, 0x4444u);
    runtime.Seed(0x4040u, 0x5555u);
}

void FileBufferDestructorClosesAndReleasesInOrder() {
    Runtime runtime;
    SeedFileBuffer(runtime);
    Check(pal4::CeguiFileBufferLifecycle::DestroyFileBuffer(
              0x4000u, runtime) == 0x9000u &&
              runtime.Peek(0x4000u) ==
                  pal4::CeguiFileBufferLifecycle::kStreambufVtable &&
              runtime.PeekByte(0x4048u) == 0u &&
              runtime.Peek(0x4050u) == 0u &&
              runtime.Peek(0x4038u) == 0u &&
              runtime.Peek(0x403cu) == 0xdeadu &&
              runtime.Peek(0x4040u) == 0xdeadu &&
              runtime.Peek(0x8004u) == 0u &&
              runtime.Peek(0x9004u) == 1u &&
              runtime.destroyed == std::vector<std::uint32_t>{0x8000u},
          "successful close resets file state and releases references");
    const auto fclose = std::find(runtime.calls.begin(),
                                  runtime.calls.end(), "fclose");
    const auto init = std::find(runtime.calls.begin(),
                                runtime.calls.end(), "init_wide");
    const auto flag = std::find(runtime.calls.begin(),
                                runtime.calls.end(), "error_flag");
    Check(std::count(runtime.calls.begin(), runtime.calls.end(),
                     "locale") == 2,
          "filebuf rereads the cached locale value for both fields");
    const auto freed = std::find(runtime.calls.begin(),
                                 runtime.calls.end(), "free");
    const auto destroy = std::find(runtime.calls.begin(),
                                   runtime.calls.end(), "destroy_ref");
    Check(fclose < init && init < flag && flag < freed && freed < destroy,
          "file close, reset, auxiliary free and reference cleanup are ordered");
}

void FailedFcloseLeavesFileFieldsUntouched() {
    Runtime runtime;
    SeedFileBuffer(runtime);
    runtime.fclose_result = -1;
    static_cast<void>(pal4::CeguiFileBufferLifecycle::DestroyFileBuffer(
        0x4000u, runtime));
    Check(runtime.PeekByte(0x4048u) == 1u &&
              runtime.Peek(0x4050u) == 0x7000u &&
              runtime.Peek(0x4038u) == 0x3333u &&
              std::find(runtime.calls.begin(), runtime.calls.end(),
                        "init_wide") == runtime.calls.end() &&
              runtime.destroyed == std::vector<std::uint32_t>{0x8000u},
          "failed fclose skips file-field reset but continues destruction");
}

void StreambufDestructorHandlesRefcountBranches() {
    for (const auto count : {0u, 1u, 2u, 0xffffffffu}) {
        Runtime runtime;
        runtime.Seed(0x4034u, 0x9000u);
        runtime.Seed(0x9004u, count);
        const auto result =
            pal4::CeguiFileBufferLifecycle::DestroyStreambuf(
                0x4000u, runtime);
        Check(runtime.Peek(0x4000u) ==
                  pal4::CeguiFileBufferLifecycle::kStreambufVtable,
              "streambuf destructor installs vtable first");
        if (count <= 1u) {
            Check(result == 0xbeefu &&
                      runtime.destroyed ==
                          std::vector<std::uint32_t>{0x9000u} &&
                      runtime.Peek(0x9004u) == 0u,
                  "zero/one count invokes virtual reference destructor");
        } else {
            Check(result == 0x9000u && runtime.destroyed.empty() &&
                      runtime.Peek(0x9004u) ==
                          (count == 2u ? 1u : 0xffffffffu),
                  "positive count decrements and sentinel count is retained");
        }
    }
}

void NullReferenceAndSentinelFunctions() {
    Runtime runtime;
    runtime.Seed(0x4034u, 0u);
    Check(pal4::CeguiFileBufferLifecycle::DestroyStreambuf(
              0x4000u, runtime) == 0u &&
              runtime.destroyed.empty() &&
              pal4::CeguiFileBufferLifecycle::ReturnNegativeOne(123u) == -1 &&
              pal4::CeguiFileBufferLifecycle::Underflow() == -1,
          "null reference is skipped and both sentinel methods return -1");
}

void SeedFileStream(Runtime& runtime) {
    SeedFileBuffer(runtime);
    runtime.Seed(0x3ff8u, 0xb000u);
    runtime.Seed(0xb004u, 0x30u);
    runtime.Seed(0x4034u, 0xc000u);
    runtime.Seed(0xc004u, 1u);
}

void FileStreamSubobjectDestructor() {
    Runtime runtime;
    SeedFileStream(runtime);
    runtime.locale_release = 0xc000u;
    Check(pal4::CeguiFileBufferLifecycle::DestroyFileStreamSubobjects(
              0x4054u, runtime) == 0xb000u &&
              runtime.Peek(0x4028u) ==
                  pal4::CeguiFileBufferLifecycle::kIstreamVtable &&
              runtime.Peek(0x4000u) ==
                  pal4::CeguiFileBufferLifecycle::kStreambufVtable &&
              runtime.Peek(0x8004u) == 0u &&
              runtime.destroyed ==
                  std::vector<std::uint32_t>{0x8000u, 0xc000u},
          "file stream destroys filebuf, streambuf and locale references");
    const auto aux = std::find(runtime.calls.begin(), runtime.calls.end(),
                               "istream_aux");
    const auto close = std::find(runtime.calls.begin(), runtime.calls.end(),
                                 "fclose");
    const auto init = std::find(runtime.calls.begin(), runtime.calls.end(),
                                "init_filebuf");
    const auto free = std::find(runtime.calls.begin(), runtime.calls.end(),
                                "free");
    const auto facet = std::find(runtime.calls.begin(), runtime.calls.end(),
                                 "decref_facet");
    const auto final = std::find(runtime.calls.begin(), runtime.calls.end(),
                                 "istream_final");
    Check(aux < close && close < init && init < free &&
              free < facet && facet < final,
          "subobject vtables, close, storage and facet release are ordered");
}

void FileStreamSubobjectCloseAndFacetBranches() {
    Runtime runtime;
    SeedFileStream(runtime);
    runtime.fclose_result = -1;
    runtime.locale_release = 0u;
    (void)pal4::CeguiFileBufferLifecycle::DestroyFileStreamSubobjects(
        0x4054u, runtime);
    Check(std::find(runtime.calls.begin(), runtime.calls.end(),
                    "init_filebuf") == runtime.calls.end() &&
              runtime.destroyed ==
                  std::vector<std::uint32_t>{0x8000u} &&
              runtime.Peek(0x4028u) ==
                  pal4::CeguiFileBufferLifecycle::kIstreamVtable,
          "failed fclose skips init and unreleased facet skips destruction");
    Runtime closed;
    SeedFileStream(closed);
    closed.SeedByte(0x4048u, 0u);
    closed.Seed(0x4034u, 0u);
    (void)pal4::CeguiFileBufferLifecycle::DestroyFileStreamSubobjects(
        0x4054u, closed);
    Check(std::find(closed.calls.begin(), closed.calls.end(),
                    "fclose") == closed.calls.end() &&
              std::find(closed.calls.begin(), closed.calls.end(),
                        "decref_facet") == closed.calls.end(),
          "closed stream and null facet skip CRT and locale calls");
}

void StreambufFlaggedDestructor() {
    for (const auto flags : {0u, 2u, 1u, 3u}) {
        Runtime runtime;
        runtime.Seed(0x4034u, 0x9000u);
        runtime.Seed(0x9004u, 1u);
        Check(pal4::CeguiFileBufferLifecycle::DestroyStreambufWithFlag(
                  0x4000u, static_cast<std::uint8_t>(flags),
                  runtime) == 0x4000u &&
                  runtime.Peek(0x4000u) ==
                      pal4::CeguiFileBufferLifecycle::kStreambufVtable &&
                  runtime.destroyed ==
                      std::vector<std::uint32_t>{0x9000u} &&
                  (flags & 1u ? runtime.freed ==
                      std::vector<std::uint32_t>{0x4000u}
                              : runtime.freed.empty()),
              "wrapper destroys streambuf then conditionally frees object");
    }
}

void FileBufferFlaggedDestructor() {
    for (const auto flags : {0u, 2u, 1u, 3u}) {
        Runtime runtime;
        SeedFileBuffer(runtime);
        Check(pal4::CeguiFileBufferLifecycle::DestroyFileBufferWithFlag(
                  0x4000u, static_cast<std::uint8_t>(flags),
                  runtime) == 0x4000u &&
                  runtime.Peek(0x4000u) ==
                      pal4::CeguiFileBufferLifecycle::kStreambufVtable &&
                  runtime.destroyed ==
                      std::vector<std::uint32_t>{0x8000u} &&
                  runtime.freed == (flags & 1u
                      ? std::vector<std::uint32_t>{0xa000u, 0x4000u}
                      : std::vector<std::uint32_t>{0xa000u}),
              "filebuf wrapper conditionally frees object after destruction");
    }
}

}  // namespace

int main() {
    FileBufferDestructorClosesAndReleasesInOrder();
    FailedFcloseLeavesFileFieldsUntouched();
    StreambufDestructorHandlesRefcountBranches();
    NullReferenceAndSentinelFunctions();
    FileStreamSubobjectDestructor();
    FileStreamSubobjectCloseAndFacetBranches();
    StreambufFlaggedDestructor();
    FileBufferFlaggedDestructor();
    std::cout << "8 CEGUI file-buffer lifecycle test cases passed\n";
}
