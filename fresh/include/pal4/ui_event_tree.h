#pragma once

#include <cstdint>

namespace pal4 {

struct UiEventInsertResult {
    std::uint32_t node{};
    bool inserted{};
};

class IUiEventTreeRuntime {
public:
    virtual ~IUiEventTreeRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    [[nodiscard]] virtual std::uint32_t Free(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t SharedNil() = 0;
    virtual void SetSharedNil(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t ReferenceCount() = 0;
    virtual void SetReferenceCount(std::uint32_t count) = 0;
    [[nodiscard]] virtual std::uint32_t FirstElement(
        std::uint32_t tree) = 0;
    [[nodiscard]] virtual std::uint32_t CloneHandler(
        std::uint32_t handler) = 0;
    [[nodiscard]] virtual std::uint32_t CreateNode(
        std::uint32_t parent) = 0;
    [[nodiscard]] virtual std::uint32_t DeleteHandler(
        std::uint32_t handler, std::uint32_t flags) = 0;
    [[nodiscard]] virtual std::uint8_t InvokeHandler(
        std::uint32_t handler, std::uint32_t argument) = 0;
};

class UiEventTree final {
public:
    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t tree, IUiEventTreeRuntime& runtime);
    [[nodiscard]] static std::uint32_t DestroyAlt(
        std::uint32_t tree, IUiEventTreeRuntime& runtime);
    [[nodiscard]] static std::uint32_t ClearAndRelease(
        std::uint32_t tree, IUiEventTreeRuntime& runtime);
    [[nodiscard]] static bool InsertOrReplace(
        std::uint32_t tree, std::uint32_t key_address,
        std::uint32_t handler_slot, IUiEventTreeRuntime& runtime);
    [[nodiscard]] static UiEventInsertResult Insert(
        std::uint32_t tree, std::uint32_t key,
        std::uint32_t handler, IUiEventTreeRuntime& runtime);
    [[nodiscard]] static std::uint32_t InsertNode(
        std::uint32_t tree, std::uint32_t child,
        std::uint32_t parent, std::uint32_t key,
        std::uint32_t handler, IUiEventTreeRuntime& runtime);
    [[nodiscard]] static std::uint32_t Erase(
        std::uint32_t tree, std::uint32_t node,
        IUiEventTreeRuntime& runtime);
    [[nodiscard]] static std::uint32_t FindNode(
        std::uint32_t tree, std::uint32_t key,
        IUiEventTreeRuntime& runtime);
    [[nodiscard]] static std::uint32_t FreeNodes(
        std::uint32_t root, IUiEventTreeRuntime& runtime);
    [[nodiscard]] static std::uint32_t RotateRight(
        std::uint32_t tree, std::uint32_t node,
        IUiEventTreeRuntime& runtime);
    [[nodiscard]] static std::uint32_t RotateLeft(
        std::uint32_t tree, std::uint32_t node,
        IUiEventTreeRuntime& runtime);
    [[nodiscard]] static std::uint32_t Successor(
        std::uint32_t& iterator, IUiEventTreeRuntime& runtime);
    [[nodiscard]] static std::uint32_t Find(
        std::uint32_t tree, std::uint32_t key,
        IUiEventTreeRuntime& runtime);
    [[nodiscard]] static std::uint32_t Predecessor(
        std::uint32_t& iterator, IUiEventTreeRuntime& runtime);
    static void SuccessorAlt(
        std::uint32_t& iterator, IUiEventTreeRuntime& runtime);
    static void CopyValue(
        std::uint32_t destination, std::uint32_t key,
        std::uint32_t handler, IUiEventTreeRuntime& runtime);
    [[nodiscard]] static std::uint32_t DestroyValue(
        std::uint32_t record, std::uint8_t flags,
        IUiEventTreeRuntime& runtime);
    [[nodiscard]] static std::uint32_t ReleaseClone(
        std::uint32_t record, IUiEventTreeRuntime& runtime);
    [[nodiscard]] static std::uint8_t Process(
        std::uint32_t tree, std::uint32_t key_address,
        std::uint32_t argument, IUiEventTreeRuntime& runtime);
};

}  // namespace pal4
