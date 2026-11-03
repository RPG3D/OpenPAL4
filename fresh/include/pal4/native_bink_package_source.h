#pragma once

#include "pal4/native_bink_decoder.h"

#include <string>

namespace pal4 {

class NativeBinkPackageSource final {
public:
    explicit NativeBinkPackageSource(NativeBinkDecoder& decoder)
        : decoder_(decoder) {}
    ~NativeBinkPackageSource();
    NativeBinkPackageSource(const NativeBinkPackageSource&) = delete;
    NativeBinkPackageSource& operator=(const NativeBinkPackageSource&) = delete;

    [[nodiscard]] bool Open(const char* archive_path, const char* member_name);
    void Close();

private:
    NativeBinkDecoder& decoder_;
    std::string temporary_path_;
};

}  // namespace pal4
