#pragma once

#include <cstdint>

namespace pal4 {

class IUiRedBlackTreeStorageRuntime {
public:
    virtual ~IUiRedBlackTreeStorageRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t AllocateMemory(std::uint32_t size) = 0;
    virtual void FreeMemory(std::uint32_t address) = 0;
    virtual void CleanupData(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t SharedNil() = 0;
    virtual void SetSharedNil(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t SharedNilUsers() = 0;
    virtual void SetSharedNilUsers(std::uint32_t users) = 0;
};

class IUiCeguiRedBlackTreeStorageRuntime
    : public IUiRedBlackTreeStorageRuntime {
public:
    virtual void FreeCeguiString(std::uint32_t address) = 0;
};

class UiRedBlackTreeStorage final {
public:
    // IDA 0x4c0c60.
    [[nodiscard]] static std::uint32_t FreeNodes(
        std::uint32_t node, IUiRedBlackTreeStorageRuntime& runtime);

    // IDA 0x4c14e0. Same traversal, CEGUI string payload.
    [[nodiscard]] static std::uint32_t FreeCeguiNodes(
        std::uint32_t node, IUiCeguiRedBlackTreeStorageRuntime& runtime);

    // IDA 0x4c0cb0.
    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t tree, IUiRedBlackTreeStorageRuntime& runtime);
};

}  // namespace pal4
