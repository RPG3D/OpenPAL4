#include "pal4/exception_info.h"

#include <string>

namespace pal4 {

ExceptionInfo& ExceptionInfo::Format(const char* initial, const char* function,
                                     const char* file, const int line) {
    base_buffer_ = 0u;
    base_buffer_owned_ = 0u;
    vtable_ = kDerivedVtable;

    text_ = initial;
    text_ += *function != '\0' ? "Func:  " + std::string(function) + '\n'
                              : "Func:  <Unknown>\n";
    text_ += *file != '\0' ? "File:  " + std::string(file) + '\n'
                          : "File:  <Unknown>\n";
    text_ += line != 0 ? "Line:  " + std::to_string(line) + '\n'
                       : "Line:  <Unknown>\n";
    return *this;
}

void ExceptionInfo::SetBaseBuffer(const std::uint32_t address,
                                  const std::uint32_t owned) {
    base_buffer_ = address;
    base_buffer_owned_ = owned;
}

std::uint32_t ExceptionInfo::BaseWhatAddress() const {
    return base_buffer_ ? base_buffer_ : 0x841964u;
}

void ExceptionInfo::Destroy(IExceptionInfoRuntime& runtime) {
    vtable_ = kDerivedVtable;
    vtable_ = kBaseVtable;
    if (base_buffer_owned_ != 0u) {
        runtime.FreeBaseBuffer(base_buffer_);
    }
}

ExceptionInfo& ExceptionInfo::CopyFrom(
    const ExceptionInfo& source, IExceptionInfoCopyRuntime& runtime) {
    vtable_ = kBaseVtable;
    base_buffer_owned_ = source.base_buffer_owned_;
    base_buffer_ = base_buffer_owned_ != 0u
        ? runtime.DuplicateBaseBuffer(source.base_buffer_)
        : source.base_buffer_;
    extra_field_ = source.extra_field_;
    text_ = source.text_;
    vtable_ = kDerivedVtable;
    return *this;
}

}  // namespace pal4
