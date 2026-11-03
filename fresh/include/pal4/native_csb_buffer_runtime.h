#pragma once

#include "pal4/csb_buffer_reader.h"
#include "pal4/native_pk_file_manager_runtime.h"

#include <cstdint>

namespace pal4 {

class NativeCsbBufferRuntime final : public ICsbBufferRuntime {
public:
    explicit NativeCsbBufferRuntime(NativePkFileManagerRuntime& packages)
        : packages_(packages) {}
    ~NativeCsbBufferRuntime() override;
    NativeCsbBufferRuntime(const NativeCsbBufferRuntime&) = delete;
    NativeCsbBufferRuntime& operator=(const NativeCsbBufferRuntime&) = delete;

    [[nodiscard]] std::uint32_t CreateContext(
        std::uint32_t type, std::uint32_t access,
        const char* source) override;
    void ReadSize(std::uint32_t context, std::uint32_t& size) override;
    [[nodiscard]] std::uint32_t Allocate(std::uint32_t size) override;
    void ReadBytes(std::uint32_t context, std::uint32_t destination,
                   std::uint32_t size) override;
    void CloseContext(std::uint32_t context, std::uint32_t output) override;

    static void FreePayload(std::uint32_t data);

private:
    NativePkFileManagerRuntime& packages_;
    std::uint32_t handle_ = 0u;
    std::uint32_t member_size_ = 0u;
    std::uint32_t payload_size_ = 0u;
    std::uint32_t allocation_ = 0u;
};

}  // namespace pal4
