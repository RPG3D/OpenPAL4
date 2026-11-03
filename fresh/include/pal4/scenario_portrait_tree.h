#pragma once

#include <cstdint>

namespace pal4 {

class IScenarioPortraitTreeRuntime {
public:
    virtual ~IScenarioPortraitTreeRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(
        std::uint32_t address) = 0;
    virtual void WriteByte(std::uint32_t address,
                           std::uint8_t value) = 0;
    [[nodiscard]] virtual std::uint32_t SharedNil() = 0;
    virtual void SetSharedNil(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t SharedNilRefCount() = 0;
    virtual void SetSharedNilRefCount(std::uint32_t count) = 0;
    [[nodiscard]] virtual std::uint32_t AllocateMemory(
        std::uint32_t size) = 0;
    [[nodiscard]] virtual int CompareSearch(
        std::uint32_t value, std::uint32_t node_data) = 0;
    [[nodiscard]] virtual int CompareData(
        std::uint32_t left, std::uint32_t right) = 0;
    virtual void CopyData(std::uint32_t destination,
                          std::uint32_t source) = 0;
    [[nodiscard]] virtual std::uint32_t EmptyData() = 0;
    virtual void AssignFirstBuffer(std::uint32_t destination,
                                   std::uint32_t source,
                                   std::uint32_t start,
                                   std::uint32_t count) = 0;
    virtual void EraseBuffer(std::uint32_t buffer,
                             std::uint32_t start,
                             std::uint32_t count) = 0;
    [[nodiscard]] virtual bool AllocateText(
        std::uint32_t buffer, std::uint32_t length,
        std::uint8_t mode) = 0;
    virtual void CopyBytes(std::uint32_t destination,
                           std::uint32_t source,
                           std::uint32_t length) = 0;
    virtual void SetTextSize(std::uint32_t buffer,
                             std::uint32_t length) = 0;
    [[nodiscard]] virtual std::uint32_t Predecessor(
        std::uint32_t node) = 0;
    [[nodiscard]] virtual std::uint32_t Successor(
        std::uint32_t node) = 0;
    virtual void RotateLeft(std::uint32_t tree,
                            std::uint32_t node) = 0;
    virtual void RotateRight(std::uint32_t tree,
                             std::uint32_t node) = 0;
    virtual void SetErrorFlag(std::uint32_t address,
                              std::uint32_t value) = 0;
    virtual void FreeMemory(std::uint32_t address) = 0;
    virtual void CleanupProperties(std::uint32_t address) = 0;
};

class ScenarioPortraitTree final {
public:
    // IDA 0x4b84d0.
    [[nodiscard]] static std::uint32_t Insert(
        std::uint32_t tree, std::uint32_t output,
        std::uint32_t value,
        IScenarioPortraitTreeRuntime& runtime);

    // IDA 0x4b8730.
    [[nodiscard]] static std::uint32_t Find(
        std::uint32_t tree, std::uint32_t output,
        std::uint32_t value,
        IScenarioPortraitTreeRuntime& runtime);

    // IDA 0x4b87e0.
    [[nodiscard]] static std::uint32_t Remove(
        std::uint32_t tree, std::uint32_t output,
        std::uint32_t node,
        IScenarioPortraitTreeRuntime& runtime);

    // IDA 0x4b8bf0.
    [[nodiscard]] static std::uint32_t FreeNodes(
        std::uint32_t node,
        IScenarioPortraitTreeRuntime& runtime);

    // IDA 0x4b8c40.
    [[nodiscard]] static std::uint32_t Initialize(
        std::uint32_t tree,
        IScenarioPortraitTreeRuntime& runtime);

    // IDA 0x4b8cc0.
    [[nodiscard]] static std::uint32_t InsertUnique(
        std::uint32_t tree, std::uint32_t output,
        std::uint32_t child, std::uint32_t parent,
        std::uint32_t value,
        IScenarioPortraitTreeRuntime& runtime);

    // IDA 0x4b8f30: lifts the right child.
    [[nodiscard]] static std::uint32_t RotateRight(
        std::uint32_t tree, std::uint32_t node,
        IScenarioPortraitTreeRuntime& runtime);

    // IDA 0x4b8f90: lifts the left child.
    [[nodiscard]] static std::uint32_t RotateLeft(
        std::uint32_t tree, std::uint32_t node,
        IScenarioPortraitTreeRuntime& runtime);

    // IDA 0x4b8ff0.
    [[nodiscard]] static std::uint32_t AllocateNode(
        std::uint32_t parent, std::uint32_t colour,
        IScenarioPortraitTreeRuntime& runtime);

    // IDA 0x4b9010.
    [[nodiscard]] static std::uint32_t Predecessor(
        std::uint32_t node,
        IScenarioPortraitTreeRuntime& runtime);

    // IDA 0x4b9070.
    [[nodiscard]] static std::uint32_t Successor(
        std::uint32_t node,
        IScenarioPortraitTreeRuntime& runtime);

    // IDA 0x4b90c0.
    static void CopyData(std::uint32_t destination,
                         std::uint32_t source,
                         IScenarioPortraitTreeRuntime& runtime);
};

}  // namespace pal4
