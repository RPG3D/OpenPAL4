#include "pal4/xml_document_record.h"
#include "pal4/range_no_op_callback.h"

#include <new>
#include <array>
#include <stdexcept>
#include <string>

namespace pal4 {

namespace {

void InitializeTree(const std::uint32_t document,
                    XmlDocumentTreeGlobals& globals,
                    IXmlDocumentRecordRuntime& runtime) {
    runtime.WriteByte(document + 28u, 0u);
    (void)XmlDocumentRecord::CreateAttributeTree(document + 20u,
                                                  globals, runtime);
}

void CopyMembers(const std::uint32_t destination, const std::uint32_t source,
                 XmlDocumentTreeGlobals& globals,
                 IXmlDocumentRecordRuntime& runtime) {
    if (static_cast<std::int32_t>(runtime.ReadWord(source + 52u)) > 0) {
        const auto head = runtime.ReadWord(source + 24u);
        auto node = runtime.ReadWord(head);
        while (node != head) {
            std::array<std::uint32_t, 7> storage{};
            const auto temporary =
                runtime.CopyAttributeToTemporary(node + 28u, storage);
            try {
                (void)XmlDocumentRecord::AddAttribute(destination, temporary,
                                                       runtime);
            } catch (...) {
                runtime.DestroyTemporaryAttribute(temporary);
                throw;
            }
            runtime.DestroyTemporaryAttribute(temporary);
            node = runtime.GetNextNode(node);
        }
    }

    if (static_cast<std::int32_t>(runtime.ReadWord(source + 56u)) > 0) {
        for (auto slot = runtime.ReadWord(source + 40u);
             slot != runtime.ReadWord(source + 44u); slot += 4u) {
            const auto child = XmlDocumentRecord::Clone(runtime.ReadWord(slot),
                                                        globals, runtime);
            runtime.AddChild(destination, child);
        }
    }
}

void CopyContents(const std::uint32_t destination, const std::uint32_t source,
                  XmlDocumentTreeGlobals& globals,
                  IXmlDocumentRecordRuntime& runtime) {
    const auto source_name = runtime.ReadWord(source + 4u);
    const auto name = source_name != 0u ? source_name
                                         : runtime.EmptyTextAddress();
    (void)XmlDocumentRecord::SetName(destination, name, 0u, runtime);
    CopyMembers(destination, source, globals, runtime);
}

bool HasName(const std::uint32_t child, const std::uint32_t name,
             const std::uint32_t length,
             IXmlDocumentRecordRuntime& runtime) {
    if (runtime.ReadWord(child + 8u) != length) return false;
    const auto child_name = runtime.ReadWord(child + 4u);
    for (std::uint32_t i = 0u; i < length; ++i) {
        if (runtime.ReadByte(child_name + i) != runtime.ReadByte(name + i))
            return false;
    }
    return true;
}

class XmlTreeMutation final {
public:
    XmlTreeMutation(const std::uint32_t tree,
                    const XmlDocumentTreeGlobals& globals,
                    IXmlDocumentRecordRuntime& runtime)
        : tree_(tree), head_(runtime.ReadWord(tree + 4u)), nil_(globals.nil_node),
          globals_(globals), runtime_(runtime) {}

    [[nodiscard]] std::uint32_t InsertAt(const std::uint32_t parent,
                                         const std::uint32_t child,
                                         const std::uint32_t input) {
        const auto node = runtime_.AllocateMemory(60u);
        if (node == 0u) throw std::bad_alloc{};
        SetParent(node, parent);
        SetColor(node, 0u);
        SetLeft(node, nil_);
        SetRight(node, nil_);
        XmlDocumentRecord::CopyAttributeTreePayload(node + 12u, input,
                                                    runtime_);
        runtime_.WriteWord(tree_ + 12u,
                           runtime_.ReadWord(tree_ + 12u) + 1u);

        if (parent == head_ || child != nil_ ||
            runtime_.CompareAttributeText(input, parent + 12u) < 0) {
            SetLeft(parent, node);
            if (parent == head_) {
                SetParent(head_, node);
                SetRight(head_, node);
            } else if (parent == Left(head_)) {
                SetLeft(head_, node);
            }
        } else {
            SetRight(parent, node);
            if (parent == Right(head_)) SetRight(head_, node);
        }
        FixAfterInsert(node);
        return node;
    }

    [[nodiscard]] std::uint32_t Erase(const std::uint32_t node) {
        const auto left = Left(node);
        const auto right = Right(node);
        std::uint32_t replacement;
        std::uint32_t removed_color;
        if (left == nil_ || right == nil_) {
            replacement = left == nil_ ? right : left;
            removed_color = Color(node);
            Transplant(node, replacement);
            if (Left(head_) == node) {
                SetLeft(head_, right == nil_ ? Parent(node)
                                               : Minimum(replacement));
            }
            if (Right(head_) == node) {
                SetRight(head_, left == nil_ ? Parent(node)
                                                : Maximum(replacement));
            }
        } else {
            const auto successor = Minimum(right);
            replacement = Right(successor);
            removed_color = Color(successor);
            if (Parent(successor) == node) {
                SetParent(replacement, successor);
            } else {
                Transplant(successor, replacement);
                SetRight(successor, right);
                SetParent(right, successor);
            }
            Transplant(node, successor);
            SetLeft(successor, left);
            SetParent(left, successor);
            SetColor(successor, Color(node));
        }
        if (removed_color == 1u) FixAfterErase(replacement);
        return node;
    }

private:
    [[nodiscard]] std::uint32_t Left(const std::uint32_t node) {
        return runtime_.ReadWord(node);
    }
    [[nodiscard]] std::uint32_t Parent(const std::uint32_t node) {
        return runtime_.ReadWord(node + 4u);
    }
    [[nodiscard]] std::uint32_t Right(const std::uint32_t node) {
        return runtime_.ReadWord(node + 8u);
    }
    [[nodiscard]] std::uint32_t Color(const std::uint32_t node) {
        return runtime_.ReadWord(node + 56u);
    }
    void SetLeft(const std::uint32_t node, const std::uint32_t value) {
        runtime_.WriteWord(node, value);
    }
    void SetParent(const std::uint32_t node, const std::uint32_t value) {
        runtime_.WriteWord(node + 4u, value);
    }
    void SetRight(const std::uint32_t node, const std::uint32_t value) {
        runtime_.WriteWord(node + 8u, value);
    }
    void SetColor(const std::uint32_t node, const std::uint32_t value) {
        runtime_.WriteWord(node + 56u, value);
    }
    [[nodiscard]] std::uint32_t Minimum(std::uint32_t node) {
        return XmlDocumentRecord::MinimumAttributeNode(node, globals_, runtime_);
    }
    [[nodiscard]] std::uint32_t Maximum(std::uint32_t node) {
        while (Right(node) != nil_) node = Right(node);
        return node;
    }
    void Transplant(const std::uint32_t old_node,
                    const std::uint32_t replacement) {
        const auto parent = Parent(old_node);
        if (old_node == Parent(head_))
            SetParent(head_, replacement);
        else if (old_node == Left(parent))
            SetLeft(parent, replacement);
        else
            SetRight(parent, replacement);
        SetParent(replacement, parent);
    }
    void RotateLeft(const std::uint32_t node) {
        (void)XmlDocumentRecord::RotateAttributeLeft(tree_, node, globals_, runtime_);
    }
    void RotateRight(const std::uint32_t node) {
        (void)XmlDocumentRecord::RotateAttributeRight(tree_, node, globals_, runtime_);
    }
    void FixAfterInsert(std::uint32_t node) {
        while (node != Parent(head_) && Color(Parent(node)) == 0u) {
            const auto parent = Parent(node);
            const auto grandparent = Parent(parent);
            if (parent == Left(grandparent)) {
                const auto uncle = Right(grandparent);
                if (Color(uncle) == 0u) {
                    SetColor(parent, 1u);
                    SetColor(uncle, 1u);
                    SetColor(grandparent, 0u);
                    node = grandparent;
                } else {
                    if (node == Right(parent)) {
                        node = parent;
                        RotateLeft(node);
                    }
                    SetColor(Parent(node), 1u);
                    SetColor(Parent(Parent(node)), 0u);
                    RotateRight(Parent(Parent(node)));
                }
            } else {
                const auto uncle = Left(grandparent);
                if (Color(uncle) == 0u) {
                    SetColor(parent, 1u);
                    SetColor(uncle, 1u);
                    SetColor(grandparent, 0u);
                    node = grandparent;
                } else {
                    if (node == Left(parent)) {
                        node = parent;
                        RotateRight(node);
                    }
                    SetColor(Parent(node), 1u);
                    SetColor(Parent(Parent(node)), 0u);
                    RotateLeft(Parent(Parent(node)));
                }
            }
        }
        SetColor(Parent(head_), 1u);
    }
    void FixAfterErase(std::uint32_t node) {
        while (node != Parent(head_) && Color(node) == 1u) {
            const auto parent = Parent(node);
            if (node == Left(parent)) {
                auto sibling = Right(parent);
                if (Color(sibling) == 0u) {
                    SetColor(sibling, 1u);
                    SetColor(parent, 0u);
                    RotateLeft(parent);
                    sibling = Right(parent);
                }
                if (Color(Left(sibling)) == 1u &&
                    Color(Right(sibling)) == 1u) {
                    SetColor(sibling, 0u);
                    node = parent;
                } else {
                    if (Color(Right(sibling)) == 1u) {
                        SetColor(Left(sibling), 1u);
                        SetColor(sibling, 0u);
                        RotateRight(sibling);
                        sibling = Right(parent);
                    }
                    SetColor(sibling, Color(parent));
                    SetColor(parent, 1u);
                    SetColor(Right(sibling), 1u);
                    RotateLeft(parent);
                    node = Parent(head_);
                }
            } else {
                auto sibling = Left(parent);
                if (Color(sibling) == 0u) {
                    SetColor(sibling, 1u);
                    SetColor(parent, 0u);
                    RotateRight(parent);
                    sibling = Left(parent);
                }
                if (Color(Right(sibling)) == 1u &&
                    Color(Left(sibling)) == 1u) {
                    SetColor(sibling, 0u);
                    node = parent;
                } else {
                    if (Color(Left(sibling)) == 1u) {
                        SetColor(Right(sibling), 1u);
                        SetColor(sibling, 0u);
                        RotateLeft(sibling);
                        sibling = Left(parent);
                    }
                    SetColor(sibling, Color(parent));
                    SetColor(parent, 1u);
                    SetColor(Left(sibling), 1u);
                    RotateRight(parent);
                    node = Parent(head_);
                }
            }
        }
        SetColor(node, 1u);
    }

    std::uint32_t tree_;
    std::uint32_t head_;
    std::uint32_t nil_;
    const XmlDocumentTreeGlobals& globals_;
    IXmlDocumentRecordRuntime& runtime_;
};

struct TreeAdvance {
    std::uint32_t next;
    std::uint32_t result;
};

TreeAdvance AdvanceTreeNode(std::uint32_t node, const std::uint32_t nil,
                            IXmlDocumentRecordRuntime& runtime) {
    auto right = runtime.ReadWord(node + 8u);
    if (right != nil) {
        while (runtime.ReadWord(right) != nil)
            right = runtime.ReadWord(right);
        return {right, nil};
    }
    auto parent = runtime.ReadWord(node + 4u);
    while (node == runtime.ReadWord(parent + 8u)) {
        node = parent;
        parent = runtime.ReadWord(parent + 4u);
    }
    return {runtime.ReadWord(node + 8u) == parent ? node : parent, parent};
}

TreeAdvance RetreatTreeNode(std::uint32_t node, const std::uint32_t nil,
                            IXmlDocumentRecordRuntime& runtime) {
    if (runtime.ReadWord(node + 56u) == 0u &&
        runtime.ReadWord(runtime.ReadWord(node + 4u) + 4u) == node) {
        const auto right = runtime.ReadWord(node + 8u);
        return {right, right};
    }
    auto left = runtime.ReadWord(node);
    if (left != nil) {
        auto right = runtime.ReadWord(left + 8u);
        while (right != nil) {
            left = right;
            right = runtime.ReadWord(left + 8u);
        }
        return {left, right};
    }
    auto parent = runtime.ReadWord(node + 4u);
    while (node == runtime.ReadWord(parent)) {
        node = parent;
        parent = runtime.ReadWord(parent + 4u);
    }
    return {parent, parent};
}

std::uint32_t EraseAttributeNode(const std::uint32_t tree,
                                  const std::uint32_t node,
                                  XmlDocumentTreeGlobals& globals,
                                  IXmlDocumentRecordRuntime& runtime) {
    const auto next = AdvanceTreeNode(node, globals.nil_node, runtime).next;
    XmlTreeMutation mutation(tree, globals, runtime);
    const auto removed = mutation.Erase(node);
    (void)XmlDocumentRecord::CleanupDocumentValue(removed + 12u, runtime);
    (void)runtime.FreeMemory(removed);
    runtime.WriteWord(tree + 12u, runtime.ReadWord(tree + 12u) - 1u);
    return next;
}

}  // namespace

std::uint32_t XmlDocumentRecord::Initialize(
    const std::uint32_t document, XmlDocumentTreeGlobals& globals,
    IXmlDocumentRecordRuntime& runtime) {
    runtime.WriteWord(document + 4u, 0u);
    runtime.WriteWord(document + 8u, 0u);
    runtime.WriteWord(document + 12u, 0u);
    InitializeTree(document, globals, runtime);
    runtime.WriteWord(document + 40u, 0u);
    runtime.WriteWord(document + 44u, 0u);
    runtime.WriteWord(document + 48u, 0u);
    runtime.WriteWord(document + 16u, 0u);
    runtime.WriteWord(document + 52u, 0u);
    runtime.WriteWord(document + 56u, 0u);
    return document;
}

std::uint32_t XmlDocumentRecord::InitializeFromString(
    const std::uint32_t document, const std::uint32_t source,
    const std::uint32_t parent, XmlDocumentTreeGlobals& globals,
    IXmlDocumentRecordRuntime& runtime) {
    const auto marker = static_cast<std::uint8_t>(source);
    runtime.WriteByte(document, marker);
    runtime.WriteWord(document + 4u, 0u);
    runtime.WriteWord(document + 8u, 0u);
    runtime.WriteWord(document + 12u, 0u);
    runtime.WriteByte(document + 20u, marker);
    runtime.WriteByte(document + 21u, marker);
    InitializeTree(document, globals, runtime);
    runtime.WriteWord(document + 40u, 0u);
    runtime.WriteByte(document + 36u, marker);
    runtime.WriteWord(document + 44u, 0u);
    runtime.WriteWord(document + 48u, 0u);

    std::uint32_t length = 0u;
    if (source != 0u) {
        while (runtime.ReadByte(source + length) != 0u) ++length;
    }
    if (length == 0u) {
        runtime.SetErrorFlag(document, 1u);
    } else {
        runtime.SetErrorFlag(document, 1u);
        runtime.Reallocate(document, length);
        const auto destination = runtime.ReadWord(document + 4u);
        for (std::uint32_t index = 0u; index < length; ++index)
            runtime.WriteByte(destination + index,
                              runtime.ReadByte(source + index));
        runtime.WriteWord(document + 8u, length);
        runtime.WriteByte(destination + length, 0u);
    }
    runtime.WriteWord(document + 16u, parent);
    runtime.WriteWord(document + 52u, 0u);
    runtime.WriteWord(document + 56u, 0u);
    if (parent != 0u) runtime.AddChild(parent, document);
    return document;
}

std::uint32_t XmlDocumentRecord::Destroy(
    const std::uint32_t document, XmlDocumentTreeGlobals& globals,
    IXmlDocumentRecordRuntime& runtime) {
    const auto head = runtime.ReadWord(document + 24u);
    auto node = runtime.ReadWord(head);
    if (runtime.ReadWord(document + 32u) != 0u) {
        runtime.DeleteNode(runtime.ReadWord(head + 4u));
        runtime.WriteWord(head + 4u, globals.nil_node);
        runtime.WriteWord(document + 32u, 0u);
        runtime.WriteWord(head, head);
        runtime.WriteWord(head + 8u, head);
        runtime.GetFirstElement(document + 20u);
    } else {
        while (node != head) {
            const auto current = node;
            node = runtime.GetNextNode(current);
            runtime.RemoveNode(document + 20u, current);
        }
    }
    (void)runtime.FreeMemory(head);
    runtime.WriteWord(document + 24u, 0u);
    runtime.WriteWord(document + 32u, 0u);
    --globals.construction_count;
    runtime.DocumentReferenceRemoved();
    if (globals.construction_count != 0u) return globals.construction_count;

    const auto nil = globals.nil_node;
    globals.nil_node = 0u;
    if (nil != 0u) return runtime.FreeSharedNil(nil);
    return 0u;
}

std::uint32_t XmlDocumentRecord::CopyConstruct(
    const std::uint32_t destination, const std::uint32_t source,
    XmlDocumentTreeGlobals& globals, IXmlDocumentRecordRuntime& runtime) {
    const auto marker = static_cast<std::uint8_t>(source);
    runtime.WriteByte(destination, marker);
    runtime.SetErrorFlag(destination, 0u);
    runtime.WriteByte(destination + 20u, marker);
    runtime.WriteByte(destination + 21u, marker);
    InitializeTree(destination, globals, runtime);
    runtime.WriteWord(destination + 40u, 0u);
    runtime.WriteByte(destination + 36u, marker);
    runtime.WriteWord(destination + 44u, 0u);
    runtime.WriteWord(destination + 48u, 0u);
    runtime.WriteWord(destination + 16u, 0u);
    runtime.WriteWord(destination + 52u, 0u);
    runtime.WriteWord(destination + 56u, 0u);

    CopyContents(destination, source, globals, runtime);
    return destination;
}

std::uint32_t XmlDocumentRecord::CopyAssign(
    const std::uint32_t destination, const std::uint32_t source,
    XmlDocumentTreeGlobals& globals, IXmlDocumentRecordRuntime& runtime) {
    runtime.CleanupNode(destination);
    CopyContents(destination, source, globals, runtime);
    return destination;
}

std::uint32_t XmlDocumentRecord::DestroyNode(
    const std::uint32_t node, XmlDocumentTreeGlobals& globals,
    IXmlDocumentRecordRuntime& runtime) {
    runtime.CleanupNode(node);
    (void)runtime.FreeMemory(runtime.ReadWord(node + 40u));
    runtime.WriteWord(node + 40u, 0u);
    runtime.WriteWord(node + 44u, 0u);
    runtime.WriteWord(node + 48u, 0u);

    if (runtime.ReadWord(node + 32u) != 0u)
        runtime.GetFirstElement(node + 20u);
    (void)Destroy(node, globals, runtime);

    const auto name = runtime.ReadWord(node + 4u);
    std::uint32_t result = 0u;
    if (name != 0u) {
        const auto header = name - 1u;
        const auto references = runtime.ReadByte(header);
        if (references == 0u || references == 0xffu)
            result = runtime.FreeMemory(header);
        else {
            runtime.WriteByte(header,
                              static_cast<std::uint8_t>(references - 1u));
            result = header;
        }
    }
    runtime.WriteWord(node + 4u, 0u);
    runtime.WriteWord(node + 8u, 0u);
    runtime.WriteWord(node + 12u, 0u);
    return result;
}

std::uint8_t XmlDocumentRecord::SetName(
    const std::uint32_t node, const std::uint32_t name,
    const std::uint32_t parent, IXmlDocumentRecordRuntime& runtime) {
    runtime.CleanupNode(node);
    if (name != 0u) {
        std::uint32_t length = 0u;
        while (runtime.ReadByte(name + length) != 0u) {
            if (length == 0xfffffffdu)
                throw std::length_error("XML node name is too long");
            ++length;
        }

        const auto current = runtime.ReadWord(node + 4u);
        const auto references = current != 0u
                                    ? runtime.ReadByte(current - 1u)
                                    : std::uint8_t{0};
        const bool shared = current != 0u && references != 0u &&
                            references != 0xffu;
        if (length == 0u) {
            if (shared) {
                runtime.WriteByte(current - 1u,
                                  static_cast<std::uint8_t>(references - 1u));
                runtime.WriteWord(node + 4u, 0u);
                runtime.WriteWord(node + 8u, 0u);
                runtime.WriteWord(node + 12u, 0u);
            } else {
                runtime.SetErrorFlag(node, 1u);
            }
        } else {
            const auto capacity = runtime.ReadWord(node + 12u);
            if (!shared && (capacity > 31u || capacity < length))
                runtime.SetErrorFlag(node, 1u);
            if (shared || capacity > 31u || capacity < length)
                runtime.Reallocate(node, length);

            const auto destination = runtime.ReadWord(node + 4u);
            for (std::uint32_t index = 0u; index < length; ++index)
                runtime.WriteByte(destination + index,
                                  runtime.ReadByte(name + index));
            runtime.WriteWord(node + 8u, length);
            runtime.WriteByte(destination + length, 0u);
        }
    }
    runtime.WriteWord(node + 16u, parent);
    if (parent != 0u) runtime.AddChild(parent, node);
    return 1u;
}

std::uint32_t XmlDocumentRecord::Clone(
    const std::uint32_t source, XmlDocumentTreeGlobals& globals,
    IXmlDocumentRecordRuntime& runtime) {
    const auto node = runtime.AllocateMemory(60u);
    if (node == 0u) throw std::bad_alloc{};
    const auto source_name = runtime.ReadWord(source + 4u);
    const auto name = source_name != 0u ? source_name
                                         : runtime.EmptyTextAddress();
    try {
        (void)InitializeFromString(node, name, 0u, globals, runtime);
    } catch (...) {
        (void)runtime.FreeMemory(node);
        throw;
    }
    CopyMembers(node, source, globals, runtime);
    return node;
}

std::uint8_t XmlDocumentRecord::AssignWithResult(
    const std::uint32_t destination, const std::uint32_t source,
    XmlDocumentTreeGlobals& globals, IXmlDocumentRecordRuntime& runtime) {
    (void)CopyAssign(destination, source, globals, runtime);
    const auto attribute_count =
        static_cast<std::int32_t>(runtime.ReadWord(source + 52u));
    const auto child_count =
        static_cast<std::int32_t>(runtime.ReadWord(source + 56u));
    return attribute_count > 0 || child_count > 0 ? 1u : 0u;
}

std::uint8_t XmlDocumentRecord::Cleanup(
    const std::uint32_t node, XmlDocumentTreeGlobals& globals,
    IXmlDocumentRecordRuntime& runtime) {
    const auto parent = runtime.ReadWord(node + 16u);
    if (parent != 0u) {
        const auto begin = runtime.ReadWord(parent + 40u);
        const auto end = runtime.ReadWord(parent + 44u);
        for (auto slot = begin; slot != end; slot += 4u) {
            if (runtime.ReadWord(slot) != node) continue;
            for (auto next = slot + 4u; next != end; next += 4u)
                runtime.WriteWord(next - 4u, runtime.ReadWord(next));
            RangeNoOpCallback::Invoke(end - 4u, end);
            runtime.WriteWord(parent + 44u, end - 4u);
            runtime.WriteWord(parent + 56u,
                              runtime.ReadWord(parent + 56u) - 1u);
            break;
        }
        runtime.WriteWord(node + 16u, 0u);
    }

    if (static_cast<std::int32_t>(runtime.ReadWord(node + 52u)) > 0) {
        const auto head = runtime.ReadWord(node + 24u);
        auto current = runtime.ReadWord(head);
        while (current != head) {
            runtime.FreeAttributeValue(current + 28u);
            current = runtime.GetNextNode(current);
        }
        runtime.GetFirstElement(node + 20u);
        runtime.ClearAttributeTree(node + 20u, runtime.ReadWord(head), head);
        runtime.WriteWord(node + 52u, 0u);
    }

    if (static_cast<std::int32_t>(runtime.ReadWord(node + 56u)) > 0) {
        const auto begin = runtime.ReadWord(node + 40u);
        while (begin != runtime.ReadWord(node + 44u)) {
            const auto child = runtime.ReadWord(begin);
            if (child == 0u)
                throw std::invalid_argument("null XML child node");
            (void)Cleanup(child, globals, runtime);
            (void)DestroyNode(child, globals, runtime);
            (void)runtime.FreeMemory(child);
        }
        runtime.WriteWord(node + 44u, begin);
        runtime.WriteWord(node + 56u, 0u);
    }

    auto text = runtime.ReadWord(node + 4u);
    if (text != 0u) {
        const auto references = runtime.ReadByte(text - 1u);
        if (references != 0u && references != 0xffu)
            (void)runtime.AllocateTextBuffer(node, runtime.ReadWord(node + 8u));
    }
    text = runtime.ReadWord(node + 4u);
    if (text != 0u) runtime.WriteByte(text - 1u, 0xffu);
    const auto old_end = text != 0u ? text + runtime.ReadWord(node + 8u) : 0u;

    text = runtime.ReadWord(node + 4u);
    if (text != 0u) {
        const auto references = runtime.ReadByte(text - 1u);
        if (references != 0u && references != 0xffu)
            (void)runtime.AllocateTextBuffer(node, runtime.ReadWord(node + 8u));
    }
    text = runtime.ReadWord(node + 4u);
    if (text != 0u) runtime.WriteByte(text - 1u, 0xffu);
    const auto old_text = text;
    runtime.ReallocateTextIfNeeded(node);
    const auto new_text = runtime.ReadWord(node + 4u);
    const auto offset = old_text != 0u ? old_text - new_text : 0u;
    auto remaining = old_end != 0u ? old_end - old_text : 0u;
    if (runtime.ReadWord(node + 8u) < offset)
        throw std::out_of_range("XML text buffer offset");
    runtime.DetachTextBuffer(node);
    const auto length = runtime.ReadWord(node + 8u);
    if (remaining > length - offset) remaining = length - offset;
    if (remaining == 0u) return static_cast<std::uint8_t>(length);

    const auto destination = runtime.ReadWord(node + 4u) + offset;
    const auto source = destination + remaining;
    for (std::uint32_t index = 0u; index < length - remaining - offset;
         ++index)
        runtime.WriteByte(destination + index,
                          runtime.ReadByte(source + index));
    const auto new_length = length - remaining;
    if (runtime.AllocateTextBuffer(node, new_length) == 0u) return 0u;
    return runtime.SetTextSize(node, new_length);
}

std::uint32_t XmlDocumentRecord::GetAttributeByIndex(
    const std::uint32_t node, const std::uint32_t index_address,
    IXmlDocumentRecordRuntime& runtime) {
    const auto index =
        static_cast<std::int32_t>(runtime.ReadWord(index_address));
    const auto count =
        static_cast<std::int32_t>(runtime.ReadWord(node + 52u));
    if (index < 0 || index >= count) return 0u;

    const auto head = runtime.ReadWord(node + 24u);
    auto current = runtime.ReadWord(head);
    for (std::int32_t step = 0; step < index; ++step)
        current = runtime.GetNextNode(current);
    return current + 28u;
}

std::uint8_t XmlDocumentRecord::AddAttribute(
    const std::uint32_t node, const std::uint32_t attribute,
    IXmlDocumentRecordRuntime& runtime) {
    auto name_address = runtime.ReadWord(attribute + 4u);
    if (name_address == 0u) name_address = runtime.EmptyTextAddress();
    std::string name;
    for (std::uint32_t length = 0u;; ++length) {
        const auto character = runtime.ReadByte(name_address + length);
        if (character == 0u) break;
        if (length == 0xfffffffdu)
            throw std::length_error("XML attribute name is too long");
        name.push_back(static_cast<char>(character));
    }

    const auto head = runtime.ReadWord(node + 24u);
    const auto tree = node + 20u;
    if (runtime.FindAttributeNodeForAddition(tree, name) == head) {
        (void)runtime.InsertAttributeNode(tree, name, attribute);
        runtime.WriteWord(node + 52u,
                          runtime.ReadWord(node + 52u) + 1u);
    }
    return 1u;
}

std::uint32_t XmlDocumentRecord::GetChildByIndex(
    const std::uint32_t node, const std::uint32_t index_address,
    IXmlDocumentRecordRuntime& runtime) {
    const auto index =
        static_cast<std::int32_t>(runtime.ReadWord(index_address));
    if (index < 0 ||
        index >= static_cast<std::int32_t>(runtime.ReadWord(node + 56u)))
        return 0u;
    const auto children = runtime.ReadWord(node + 40u);
    return runtime.ReadWord(children + static_cast<std::uint32_t>(index) * 4u);
}

std::uint32_t XmlDocumentRecord::FindChildByName(
    const std::uint32_t node, const std::uint32_t name_address,
    IXmlDocumentRecordRuntime& runtime) {
    const auto count = runtime.ReadWord(node + 56u);
    if (count == 0u) return 0u;

    std::uint32_t length = 0u;
    while (runtime.ReadByte(name_address + length) != 0u) {
        if (length == 0xfffffffdu)
            throw std::length_error("XML child name is too long");
        ++length;
    }
    const auto children = runtime.ReadWord(node + 40u);
    for (std::uint32_t index = 0u; index < count; ++index) {
        const auto child = runtime.ReadWord(children + index * 4u);
        if (HasName(child, name_address, length, runtime)) return child;
    }
    return 0u;
}

std::uint32_t XmlDocumentRecord::FindNextChildByName(
    const std::uint32_t node, const std::uint32_t previous_child,
    IXmlDocumentRecordRuntime& runtime) {
    const auto count = runtime.ReadWord(node + 56u);
    if (count == 0u) return 0u;
    const auto children = runtime.ReadWord(node + 40u);
    bool found_previous = false;
    for (std::uint32_t index = 0u; index < count; ++index) {
        const auto child = runtime.ReadWord(children + index * 4u);
        if (found_previous) {
            const auto length = runtime.ReadWord(previous_child + 8u);
            auto name = runtime.ReadWord(previous_child + 4u);
            if (name == 0u) name = runtime.EmptyTextAddress();
            if (HasName(child, name, length, runtime)) return child;
        }
        if (child == previous_child) found_previous = true;
    }
    return 0u;
}

std::uint32_t XmlDocumentRecord::AddChild(
    const std::uint32_t parent, const std::uint32_t child,
    IXmlDocumentRecordRuntime& runtime) {
    if (child == 0u) return 0u;

    const auto old_parent = runtime.ReadWord(child + 16u);
    if (old_parent != 0u) {
        const auto first = runtime.ReadWord(old_parent + 40u);
        const auto last = runtime.ReadWord(old_parent + 44u);
        for (auto slot = first; slot != last; slot += 4u) {
            if (runtime.ReadWord(slot) != child) continue;
            for (auto following = slot + 4u; following != last;
                 following += 4u)
                runtime.WriteWord(following - 4u,
                                  runtime.ReadWord(following));
            runtime.WriteWord(old_parent + 44u, last - 4u);
            runtime.WriteWord(old_parent + 56u,
                              runtime.ReadWord(old_parent + 56u) - 1u);
            break;
        }
    }

    const auto first = runtime.ReadWord(parent + 40u);
    const auto last = runtime.ReadWord(parent + 44u);
    const auto capacity_end = runtime.ReadWord(parent + 48u);
    if (last != capacity_end) {
        runtime.WriteWord(last, child);
        runtime.WriteWord(parent + 44u, last + 4u);
    } else {
        const auto size = first == 0u ? 0u : (last - first) / 4u;
        const auto growth = size > 1u ? size : 1u;
        const auto capacity = size + growth;
        const auto new_first = runtime.AllocateMemory(capacity * 4u);
        if (new_first == 0u) throw std::bad_alloc{};
        for (std::uint32_t index = 0u; index < size; ++index)
            runtime.WriteWord(new_first + index * 4u,
                              runtime.ReadWord(first + index * 4u));
        runtime.WriteWord(new_first + size * 4u, child);
        (void)runtime.FreeMemory(first);
        runtime.WriteWord(parent + 48u, new_first + capacity * 4u);
        runtime.WriteWord(parent + 44u, new_first + (size + 1u) * 4u);
        runtime.WriteWord(parent + 40u, new_first);
    }
    runtime.WriteWord(parent + 56u, runtime.ReadWord(parent + 56u) + 1u);
    runtime.WriteWord(child + 16u, parent);
    return child;
}

std::uint32_t XmlDocumentRecord::CreateNode(
    const std::uint32_t parent, const std::uint32_t name,
    XmlDocumentTreeGlobals& globals, IXmlDocumentRecordRuntime& runtime) {
    const auto node = runtime.AllocateMemory(60u);
    if (node == 0u) throw std::bad_alloc{};
    bool initialized = false;
    try {
        (void)Initialize(node, globals, runtime);
        initialized = true;
        if (SetName(node, name, parent, runtime) != 0u) return node;
    } catch (...) {
        if (initialized) (void)DestroyNode(node, globals, runtime);
        (void)runtime.FreeMemory(node);
        throw;
    }
    (void)DestroyNode(node, globals, runtime);
    (void)runtime.FreeMemory(node);
    return 0u;
}

std::uint8_t XmlDocumentRecord::RemoveChild(
    const std::uint32_t parent, const std::uint32_t child,
    XmlDocumentTreeGlobals& globals, IXmlDocumentRecordRuntime& runtime) {
    if (child == 0u) return 0u;
    const auto first = runtime.ReadWord(parent + 40u);
    const auto last = runtime.ReadWord(parent + 44u);
    for (auto slot = first; slot != last; slot += 4u) {
        if (runtime.ReadWord(slot) != child) continue;
        (void)Cleanup(child, globals, runtime);
        (void)DestroyNode(child, globals, runtime);
        (void)runtime.FreeMemory(child);
        return 1u;
    }
    return 0u;
}

std::uint32_t XmlDocumentRecord::CleanupDocumentValue(
    const std::uint32_t document, IXmlDocumentRecordRuntime& runtime) {
    runtime.DestroyEmbeddedAttribute(document + 16u);
    const auto name = runtime.ReadWord(document + 4u);
    std::uint32_t result = 0u;
    if (name != 0u) {
        const auto header = name - 1u;
        const auto references = runtime.ReadByte(header);
        if (references == 0u || references == 0xffu)
            result = runtime.FreeMemory(header);
        else {
            runtime.WriteByte(header,
                              static_cast<std::uint8_t>(references - 1u));
            result = header;
        }
    }
    runtime.WriteWord(document + 4u, 0u);
    runtime.WriteWord(document + 8u, 0u);
    runtime.WriteWord(document + 12u, 0u);
    return result;
}

std::uint32_t XmlDocumentRecord::RemoveAttributeNode(
    const std::uint32_t tree, const std::uint32_t iterator_address,
    const std::uint32_t node, XmlDocumentTreeGlobals& globals,
    IXmlDocumentRecordRuntime& runtime) {
    runtime.WriteWord(iterator_address,
                      EraseAttributeNode(tree, node, globals, runtime));
    return iterator_address;
}

std::uint32_t XmlDocumentRecord::ClearAttributeRange(
    const std::uint32_t tree, const std::uint32_t result_address,
    std::uint32_t first, const std::uint32_t last,
    XmlDocumentTreeGlobals& globals, IXmlDocumentRecordRuntime& runtime) {
    const auto count = runtime.ReadWord(tree + 12u);
    const auto head = count != 0u ? runtime.ReadWord(tree + 4u) : 0u;
    if (count != 0u && first == runtime.ReadWord(head) && last == head) {
        auto node = runtime.ReadWord(head + 4u);
        while (node != globals.nil_node) {
            (void)DeleteAttributeSubtree(runtime.ReadWord(node + 8u),
                                         globals, runtime);
            const auto left = runtime.ReadWord(node);
            (void)CleanupDocumentValue(node + 12u, runtime);
            (void)runtime.FreeMemory(node);
            node = left;
        }
        runtime.WriteWord(head + 4u, globals.nil_node);
        runtime.WriteWord(tree + 12u, 0u);
        runtime.WriteWord(head, head);
        runtime.WriteWord(head + 8u, head);
        first = head;
    } else {
        while (first != last) {
            const auto node = first;
            first = AdvanceTreeNode(node, globals.nil_node, runtime).next;
            (void)EraseAttributeNode(tree, node, globals, runtime);
        }
    }
    runtime.WriteWord(result_address, first);
    return result_address;
}

std::uint32_t XmlDocumentRecord::AdvanceTreeIterator(
    const std::uint32_t iterator_address, XmlDocumentTreeGlobals& globals,
    IXmlDocumentRecordRuntime& runtime) {
    const auto advance = AdvanceTreeNode(runtime.ReadWord(iterator_address),
                                         globals.nil_node, runtime);
    runtime.WriteWord(iterator_address, advance.next);
    return advance.result;
}

std::uint32_t XmlDocumentRecord::InsertAttributeTreeNode(
    const std::uint32_t tree, const std::uint32_t result_address,
    const std::uint32_t input, XmlDocumentTreeGlobals& globals,
    IXmlDocumentRecordRuntime& runtime) {
    const auto head = runtime.ReadWord(tree + 4u);
    auto parent = head;
    auto child = runtime.ReadWord(head + 4u);
    bool went_left = true;
    while (child != globals.nil_node) {
        parent = child;
        went_left = runtime.CompareAttributeRange(input, child + 12u) < 0;
        child = runtime.ReadWord(child + (went_left ? 0u : 8u));
    }

    std::uint32_t selected;
    bool inserted = true;
    if (runtime.ReadByte(tree + 8u) != 0u) {
        (void)InsertAttributeTreeNodeAt(tree, result_address, child, parent,
                                        input, globals, runtime);
        selected = runtime.ReadWord(result_address);
    } else {
        auto candidate = parent;
        if (went_left && parent != runtime.ReadWord(head))
            candidate = RetreatTreeNode(parent, globals.nil_node,
                                        runtime).next;
        if ((went_left && parent == runtime.ReadWord(head)) ||
            runtime.CompareAttributeText(candidate + 12u, input) < 0) {
            (void)InsertAttributeTreeNodeAt(tree, result_address, child, parent,
                                            input, globals, runtime);
            selected = runtime.ReadWord(result_address);
        } else {
            selected = candidate;
            inserted = false;
        }
    }
    runtime.WriteWord(result_address, selected);
    runtime.WriteByte(result_address + 4u, inserted ? 1u : 0u);
    return result_address;
}

std::uint32_t XmlDocumentRecord::DeleteAttributeSubtree(
    std::uint32_t node, XmlDocumentTreeGlobals& globals,
    IXmlDocumentRecordRuntime& runtime) {
    while (node != globals.nil_node) {
        (void)DeleteAttributeSubtree(runtime.ReadWord(node + 8u),
                                     globals, runtime);
        const auto left = runtime.ReadWord(node);
        (void)CleanupDocumentValue(node + 12u, runtime);
        (void)runtime.FreeMemory(node);
        node = left;
    }
    return globals.nil_node;
}

std::uint32_t XmlDocumentRecord::CreateAttributeTree(
    const std::uint32_t tree, XmlDocumentTreeGlobals& globals,
    IXmlDocumentRecordRuntime& runtime) {
    const auto proposed_nil = runtime.AllocateSharedNil();
    if (proposed_nil == 0u) throw std::bad_alloc{};
    runtime.WriteWord(proposed_nil + 4u, 0u);
    runtime.WriteWord(proposed_nil + 56u, 1u);
    const bool first_reference = globals.nil_node == 0u;
    if (first_reference) {
        globals.nil_node = proposed_nil;
        runtime.WriteWord(proposed_nil, 0u);
        runtime.WriteWord(proposed_nil + 8u, 0u);
    }
    ++globals.construction_count;
    runtime.DocumentReferenceAdded();
    if (!first_reference) (void)runtime.FreeSharedNil(proposed_nil);

    std::uint32_t head = 0u;
    try {
        head = AllocateAttributeTreeNode(globals.nil_node, 0u, runtime);
    } catch (...) {
        runtime.DocumentReferenceRemoved();
        if (--globals.construction_count == 0u) {
            const auto nil = globals.nil_node;
            globals.nil_node = 0u;
            (void)runtime.FreeSharedNil(nil);
        }
        throw;
    }
    runtime.WriteWord(tree + 4u, head);
    runtime.WriteWord(tree + 12u, 0u);
    runtime.WriteWord(head, head);
    runtime.WriteWord(head + 8u, head);
    return head;
}

std::uint32_t XmlDocumentRecord::RotateAttributeLeft(
    const std::uint32_t tree, const std::uint32_t node,
    const XmlDocumentTreeGlobals& globals,
    IXmlDocumentRecordRuntime& runtime) {
    const auto pivot = runtime.ReadWord(node + 8u);
    const auto middle = runtime.ReadWord(pivot);
    runtime.WriteWord(node + 8u, middle);
    if (middle != globals.nil_node) runtime.WriteWord(middle + 4u, node);
    const auto parent = runtime.ReadWord(node + 4u);
    runtime.WriteWord(pivot + 4u, parent);
    const auto head = runtime.ReadWord(tree + 4u);
    if (node == runtime.ReadWord(head + 4u))
        runtime.WriteWord(head + 4u, pivot);
    else if (node == runtime.ReadWord(parent))
        runtime.WriteWord(parent, pivot);
    else
        runtime.WriteWord(parent + 8u, pivot);
    runtime.WriteWord(pivot, node);
    runtime.WriteWord(node + 4u, pivot);
    return pivot;
}

std::uint32_t XmlDocumentRecord::MinimumAttributeNode(
    std::uint32_t node, const XmlDocumentTreeGlobals& globals,
    IXmlDocumentRecordRuntime& runtime) {
    while (runtime.ReadWord(node) != globals.nil_node)
        node = runtime.ReadWord(node);
    return node;
}

std::uint32_t XmlDocumentRecord::RotateAttributeRight(
    const std::uint32_t tree, const std::uint32_t node,
    const XmlDocumentTreeGlobals& globals,
    IXmlDocumentRecordRuntime& runtime) {
    const auto pivot = runtime.ReadWord(node);
    const auto middle = runtime.ReadWord(pivot + 8u);
    runtime.WriteWord(node, middle);
    if (middle != globals.nil_node) runtime.WriteWord(middle + 4u, node);
    const auto parent = runtime.ReadWord(node + 4u);
    runtime.WriteWord(pivot + 4u, parent);
    const auto head = runtime.ReadWord(tree + 4u);
    if (node == runtime.ReadWord(head + 4u))
        runtime.WriteWord(head + 4u, pivot);
    else if (node == runtime.ReadWord(parent + 8u))
        runtime.WriteWord(parent + 8u, pivot);
    else
        runtime.WriteWord(parent, pivot);
    runtime.WriteWord(pivot + 8u, node);
    runtime.WriteWord(node + 4u, pivot);
    return pivot;
}

std::uint32_t XmlDocumentRecord::AdvanceAndGetPrevious(
    const std::uint32_t iterator_address,
    const std::uint32_t result_address,
    const XmlDocumentTreeGlobals& globals,
    IXmlDocumentRecordRuntime& runtime) {
    const auto previous = runtime.ReadWord(iterator_address);
    const auto next = AdvanceTreeNode(previous, globals.nil_node, runtime).next;
    runtime.WriteWord(iterator_address, next);
    runtime.WriteWord(result_address, previous);
    return result_address;
}

std::uint32_t XmlDocumentRecord::InsertAttributeTreeNodeAt(
    const std::uint32_t tree, const std::uint32_t result_address,
    const std::uint32_t child, const std::uint32_t parent,
    const std::uint32_t input, const XmlDocumentTreeGlobals& globals,
    IXmlDocumentRecordRuntime& runtime) {
    const auto inserted = XmlTreeMutation(tree, globals, runtime)
                              .InsertAt(parent, child, input);
    runtime.WriteWord(result_address, inserted);
    return result_address;
}

std::uint32_t XmlDocumentRecord::AllocateAttributeTreeNode(
    const std::uint32_t parent, const std::uint32_t flag,
    IXmlDocumentRecordRuntime& runtime) {
    const auto node = runtime.AllocateMemory(60u);
    if (node == 0u) throw std::bad_alloc{};
    runtime.WriteWord(node + 4u, parent);
    runtime.WriteWord(node + 56u, flag);
    return node;
}

std::uint32_t XmlDocumentRecord::RetreatTreeIterator(
    const std::uint32_t iterator_address,
    const XmlDocumentTreeGlobals& globals,
    IXmlDocumentRecordRuntime& runtime) {
    const auto step = RetreatTreeNode(runtime.ReadWord(iterator_address),
                                      globals.nil_node, runtime);
    runtime.WriteWord(iterator_address, step.next);
    return step.result;
}

void XmlDocumentRecord::CopyAttributeTreePayload(
    const std::uint32_t destination, const std::uint32_t source,
    IXmlDocumentRecordRuntime& runtime) {
    if (destination == 0u) return;
    const auto marker = runtime.ReadByte(source);
    runtime.WriteWord(destination + 4u, 0u);
    runtime.WriteByte(destination, marker);
    runtime.WriteWord(destination + 8u, 0u);
    runtime.WriteWord(destination + 12u, 0u);
    const auto length = runtime.ReadWord(source + 8u);

    if (destination == source) {
        runtime.DetachTextBuffer(destination);
        runtime.DetachTextBuffer(destination);
    } else {
        auto text = runtime.ReadWord(source + 4u);
        if (text == 0u) text = runtime.EmptyTextAddress();
        if (length != 0u && runtime.ReadByte(text - 1u) < 0xfeu) {
            runtime.SetErrorFlag(destination, 1u);
            runtime.WriteWord(destination + 4u, text);
            runtime.WriteWord(destination + 8u, length);
            runtime.WriteWord(destination + 12u,
                              runtime.ReadWord(source + 12u));
            runtime.WriteByte(text - 1u,
                              static_cast<std::uint8_t>(
                                  runtime.ReadByte(text - 1u) + 1u));
        } else if (runtime.AllocateTextBuffer(destination, length) != 0u) {
            const auto target = runtime.ReadWord(destination + 4u);
            for (std::uint32_t index = 0u; index < length; ++index)
                runtime.WriteByte(target + index,
                                  runtime.ReadByte(text + index));
            (void)runtime.SetTextSize(destination, length);
        }
    }
    runtime.CopyEmbeddedAttribute(destination + 16u, source + 16u);
}

}  // namespace pal4
