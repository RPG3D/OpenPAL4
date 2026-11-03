#include "pal4/exception_info.h"

#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

class Runtime final : public pal4::IExceptionInfoRuntime,
                      public pal4::IExceptionInfoCopyRuntime {
public:
    explicit Runtime(const pal4::ExceptionInfo& info) : info_(info) {}

    void FreeBaseBuffer(std::uint32_t address) override {
        Check(info_.Vtable() == pal4::ExceptionInfo::kBaseVtable,
              "base vtable is installed before free");
        freed.push_back(address);
    }

    std::uint32_t DuplicateBaseBuffer(std::uint32_t address) override {
        Check(info_.Vtable() == pal4::ExceptionInfo::kBaseVtable,
              "base vtable is selected while duplicating base string");
        duplicated.push_back(address);
        return duplicate_result;
    }

    std::vector<std::uint32_t> freed;
    std::vector<std::uint32_t> duplicated;
    std::uint32_t duplicate_result = 0x9999u;

private:
    const pal4::ExceptionInfo& info_;
};

void CheckKnownFields() {
    pal4::ExceptionInfo info;
    Check(&info.Format("prefix\n", "Update", "main.cpp", 29) == &info &&
              info.Vtable() == pal4::ExceptionInfo::kDerivedVtable &&
              info.BaseBuffer() == 0u && info.BaseBufferOwned() == 0u &&
              info.Text() == "prefix\nFunc:  Update\nFile:  main.cpp\nLine:  29\n",
          "format initializes base fields, selects derived vtable and appends text");
    Check(std::string(info.TextPointer()) == info.Text(),
          "offset-pointer accessor exposes the formatted text");
    Check(info.BaseWhatAddress() == 0x841964u,
          "null std::exception message uses the shared empty-text address");
    info.SetBaseBuffer(0x2345u, 0u);
    Check(info.BaseWhatAddress() == 0x2345u,
          "non-null std::exception message pointer is returned unchanged");
}

void CheckUnknownAndSignedLine() {
    pal4::ExceptionInfo info;
    info.SetBaseBuffer(0x1111u, 1u);
    info.Format("", "", "", 0);
    Check(info.Text() ==
              "Func:  <Unknown>\nFile:  <Unknown>\nLine:  <Unknown>\n" &&
              info.BaseBuffer() == 0u && info.BaseBufferOwned() == 0u,
          "empty strings and zero line use exact fallback text");
    info.Format("start", "F", "file", -7);
    Check(info.Text() == "startFunc:  F\nFile:  file\nLine:  -7\n",
          "nonzero signed line is rendered as decimal");
}

void CheckDestructor() {
    pal4::ExceptionInfo info;
    info.Format("", "", "", 0);
    Runtime runtime(info);
    info.SetBaseBuffer(0x1234u, 0u);
    info.Destroy(runtime);
    Check(runtime.freed.empty() &&
              info.Vtable() == pal4::ExceptionInfo::kBaseVtable,
          "unowned base pointer is left intact");
    info.SetBaseBuffer(0x5678u, 3u);
    info.Destroy(runtime);
    Check(runtime.freed == std::vector<std::uint32_t>{0x5678u} &&
              info.BaseBuffer() == 0x5678u && info.BaseBufferOwned() == 3u,
          "owned base pointer is freed without clearing base fields");
}

void CheckCopyConstructor() {
    pal4::ExceptionInfo source;
    source.Format("prefix", "Function", "file.cpp", 12);
    source.SetExtraField(0x42u);
    source.SetBaseBuffer(0x1234u, 0u);
    pal4::ExceptionInfo copy;
    Runtime runtime(copy);
    Check(&copy.CopyFrom(source, runtime) == &copy &&
              copy.Vtable() == pal4::ExceptionInfo::kDerivedVtable &&
              copy.BaseBuffer() == 0x1234u && copy.BaseBufferOwned() == 0u &&
              copy.ExtraField() == 0x42u && copy.Text() == source.Text() &&
              runtime.duplicated.empty(),
          "unowned base buffer is borrowed and derived fields are copied");

    source.SetBaseBuffer(0x5678u, 1u);
    Check(&copy.CopyFrom(source, runtime) == &copy &&
              copy.BaseBuffer() == 0x9999u && copy.BaseBufferOwned() == 1u &&
              runtime.duplicated == std::vector<std::uint32_t>{0x5678u},
          "owned base buffer is duplicated before derived copy");
    runtime.duplicate_result = 0u;
    copy.CopyFrom(source, runtime);
    Check(copy.BaseBuffer() == 0u && copy.BaseBufferOwned() == 1u &&
              copy.Vtable() == pal4::ExceptionInfo::kDerivedVtable,
          "allocation failure retains the ownership flag and derived vtable");
}

}  // namespace

int main() {
    CheckKnownFields();
    CheckUnknownAndSignedLine();
    CheckDestructor();
    CheckCopyConstructor();
    std::cout << "4 exception info test cases passed\n";
}
