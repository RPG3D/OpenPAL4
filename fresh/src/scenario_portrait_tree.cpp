#include "pal4/scenario_portrait_tree.h"

#include <cstdint>

namespace pal4 {

namespace {

std::uint32_t Read(const std::uint32_t address,
                   IScenarioPortraitTreeRuntime& runtime) {
    return runtime.ReadWord(address);
}

void Finish(const std::uint32_t output, const std::uint32_t node,
            const bool inserted,
            IScenarioPortraitTreeRuntime& runtime) {
    runtime.WriteWord(output, node);
    runtime.WriteByte(output + 4u, inserted ? 1u : 0u);
}

void RestoreColours(const std::uint32_t tree,
                    const std::uint32_t header,
                    std::uint32_t node,
                    IScenarioPortraitTreeRuntime& runtime) {
    while (node != Read(header + 4u, runtime)) {
        const auto parent = Read(node + 4u, runtime);
        if (Read(parent + 44u, runtime) != 0u) break;
        const auto grandparent = Read(parent + 4u, runtime);
        const auto grand_left = Read(grandparent, runtime);
        if (parent == grand_left) {
            const auto uncle = Read(grandparent + 8u, runtime);
            if (Read(uncle + 44u, runtime) == 0u) {
                runtime.WriteWord(parent + 44u, 1u);
                runtime.WriteWord(uncle + 44u, 1u);
                runtime.WriteWord(grandparent + 44u, 0u);
                node = grandparent;
                continue;
            }
            if (node == Read(parent + 8u, runtime)) {
                node = parent;
                runtime.RotateRight(tree, parent);
            }
            const auto new_parent = Read(node + 4u, runtime);
            runtime.WriteWord(new_parent + 44u, 1u);
            const auto new_grandparent = Read(new_parent + 4u, runtime);
            runtime.WriteWord(new_grandparent + 44u, 0u);
            runtime.RotateLeft(tree, new_grandparent);
        } else {
            const auto uncle = grand_left;
            if (Read(uncle + 44u, runtime) == 0u) {
                runtime.WriteWord(parent + 44u, 1u);
                runtime.WriteWord(uncle + 44u, 1u);
                runtime.WriteWord(grandparent + 44u, 0u);
                node = grandparent;
                continue;
            }
            if (node == Read(parent, runtime)) {
                node = parent;
                runtime.RotateLeft(tree, parent);
            }
            const auto new_parent = Read(node + 4u, runtime);
            runtime.WriteWord(new_parent + 44u, 1u);
            const auto new_grandparent = Read(new_parent + 4u, runtime);
            runtime.WriteWord(new_grandparent + 44u, 0u);
            runtime.RotateRight(tree, new_grandparent);
        }
    }
    runtime.WriteWord(Read(header + 4u, runtime) + 44u, 1u);
}

void LinkNode(const std::uint32_t tree,
              const std::uint32_t header,
              const std::uint32_t child,
              const std::uint32_t parent,
              const std::uint32_t value,
              const std::uint32_t node,
              IScenarioPortraitTreeRuntime& runtime) {
    const auto nil = runtime.SharedNil();
    if (parent == header || child != nil ||
        runtime.CompareData(value, parent + 12u) < 0) {
        runtime.WriteWord(parent, node);
        if (parent == header) {
            runtime.WriteWord(header + 4u, node);
            runtime.WriteWord(header + 8u, node);
        } else if (parent == Read(header, runtime)) {
            runtime.WriteWord(header, node);
        }
    } else {
        runtime.WriteWord(parent + 8u, node);
        if (parent == Read(header + 8u, runtime))
            runtime.WriteWord(header + 8u, node);
    }
    RestoreColours(tree, header, node, runtime);
}

void RepairAfterRemove(const std::uint32_t tree,
                       const std::uint32_t header,
                       std::uint32_t child,
                       IScenarioPortraitTreeRuntime& runtime) {
    while (child != Read(header + 4u, runtime) &&
           Read(child + 44u, runtime) == 1u) {
        auto parent = Read(child + 4u, runtime);
        if (child == Read(parent, runtime)) {
            auto sibling = Read(parent + 8u, runtime);
            if (Read(sibling + 44u, runtime) == 0u) {
                runtime.WriteWord(sibling + 44u, 1u);
                runtime.WriteWord(parent + 44u, 0u);
                runtime.RotateRight(tree, parent);
                parent = Read(child + 4u, runtime);
                sibling = Read(parent + 8u, runtime);
            }
            if (Read(Read(sibling, runtime) + 44u, runtime) == 1u &&
                Read(Read(sibling + 8u, runtime) + 44u,
                     runtime) == 1u) {
                runtime.WriteWord(sibling + 44u, 0u);
                child = parent;
            } else {
                if (Read(Read(sibling + 8u, runtime) + 44u,
                         runtime) == 1u) {
                    runtime.WriteWord(Read(sibling, runtime) + 44u,
                                      1u);
                    runtime.WriteWord(sibling + 44u, 0u);
                    runtime.RotateLeft(tree, sibling);
                    parent = Read(child + 4u, runtime);
                    sibling = Read(parent + 8u, runtime);
                }
                runtime.WriteWord(sibling + 44u,
                                  Read(parent + 44u, runtime));
                runtime.WriteWord(parent + 44u, 1u);
                runtime.WriteWord(Read(sibling + 8u, runtime) + 44u,
                                  1u);
                runtime.RotateRight(tree, parent);
                break;
            }
        } else {
            auto sibling = Read(parent, runtime);
            if (Read(sibling + 44u, runtime) == 0u) {
                runtime.WriteWord(sibling + 44u, 1u);
                runtime.WriteWord(parent + 44u, 0u);
                runtime.RotateLeft(tree, parent);
                parent = Read(child + 4u, runtime);
                sibling = Read(parent, runtime);
            }
            if (Read(Read(sibling + 8u, runtime) + 44u,
                     runtime) == 1u &&
                Read(Read(sibling, runtime) + 44u, runtime) == 1u) {
                runtime.WriteWord(sibling + 44u, 0u);
                child = parent;
            } else {
                if (Read(Read(sibling, runtime) + 44u,
                         runtime) == 1u) {
                    runtime.WriteWord(Read(sibling + 8u, runtime) + 44u,
                                      1u);
                    runtime.WriteWord(sibling + 44u, 0u);
                    runtime.RotateRight(tree, sibling);
                    parent = Read(child + 4u, runtime);
                    sibling = Read(parent, runtime);
                }
                runtime.WriteWord(sibling + 44u,
                                  Read(parent + 44u, runtime));
                runtime.WriteWord(parent + 44u, 1u);
                runtime.WriteWord(Read(sibling, runtime) + 44u, 1u);
                runtime.RotateLeft(tree, parent);
                break;
            }
        }
    }
    runtime.WriteWord(child + 44u, 1u);
}

}  // namespace

std::uint32_t ScenarioPortraitTree::Insert(
    const std::uint32_t tree, const std::uint32_t output,
    const std::uint32_t value,
    IScenarioPortraitTreeRuntime& runtime) {
    const auto nil = runtime.SharedNil();
    const auto header = Read(tree + 4u, runtime);
    auto parent = header;
    auto current = Read(header + 4u, runtime);
    bool left = true;
    while (current != nil) {
        parent = current;
        left = runtime.CompareSearch(value, current + 12u) < 0;
        current = Read(current + (left ? 0u : 8u), runtime);
    }

    if (runtime.ReadByte(tree + 8u) == 0u) {
        auto candidate = parent;
        if (left) {
            if (parent == Read(header, runtime)) {
                (void)InsertUnique(tree, output, current, parent,
                                   value, runtime);
                const auto inserted = Read(output, runtime);
                Finish(output, inserted, true, runtime);
                return output;
            }
            candidate = runtime.Predecessor(parent);
        }
        if (runtime.CompareData(candidate + 12u, value) >= 0) {
            Finish(output, candidate, false, runtime);
            return output;
        }
        (void)InsertUnique(tree, output, current, parent,
                           value, runtime);
        const auto inserted = Read(output, runtime);
        Finish(output, inserted, true, runtime);
        return output;
    }

    const auto node = AllocateNode(parent, 0u, runtime);
    runtime.WriteWord(node, nil);
    runtime.WriteWord(node + 8u, nil);
    runtime.CopyData(node + 12u, value);
    runtime.WriteWord(tree + 12u,
                      Read(tree + 12u, runtime) + 1u);
    LinkNode(tree, header, current, parent, value, node,
             runtime);
    Finish(output, node, true, runtime);
    return output;
}

std::uint32_t ScenarioPortraitTree::Find(
    const std::uint32_t tree, const std::uint32_t output,
    const std::uint32_t value,
    IScenarioPortraitTreeRuntime& runtime) {
    const auto header = Read(tree + 4u, runtime);
    const auto nil = runtime.SharedNil();
    auto candidate = header;
    auto current = Read(header + 4u, runtime);
    while (current != nil) {
        if (runtime.CompareData(current + 12u, value) >= 0) {
            candidate = current;
            current = Read(current, runtime);
        } else {
            current = Read(current + 8u, runtime);
        }
    }
    // The final byte comparison in IDA is query versus candidate.
    if (candidate != header &&
        runtime.CompareData(value, candidate + 12u) < 0)
        candidate = header;
    runtime.WriteWord(output, candidate);
    return output;
}

std::uint32_t ScenarioPortraitTree::Remove(
    const std::uint32_t tree, const std::uint32_t output,
    const std::uint32_t node,
    IScenarioPortraitTreeRuntime& runtime) {
    const auto nil = runtime.SharedNil();
    const auto header = Read(tree + 4u, runtime);
    const auto successor = runtime.Successor(node);
    auto to_free = node;
    auto child = nil;
    if (Read(node, runtime) == nil) {
        child = Read(node + 8u, runtime);
    } else if (Read(node + 8u, runtime) == nil) {
        child = Read(node, runtime);
    } else {
        to_free = Read(node + 8u, runtime);
        while (Read(to_free, runtime) != nil)
            to_free = Read(to_free, runtime);
        child = Read(to_free + 8u, runtime);
    }

    if (to_free != node) {
        const auto left = Read(node, runtime);
        runtime.WriteWord(left + 4u, to_free);
        runtime.WriteWord(to_free, left);
        if (to_free == Read(node + 8u, runtime)) {
            runtime.WriteWord(child + 4u, to_free);
        } else {
            const auto old_parent = Read(to_free + 4u, runtime);
            runtime.WriteWord(child + 4u, old_parent);
            runtime.WriteWord(old_parent, child);
            const auto right = Read(node + 8u, runtime);
            runtime.WriteWord(to_free + 8u, right);
            runtime.WriteWord(right + 4u, to_free);
        }
        const auto parent = Read(node + 4u, runtime);
        if (Read(header + 4u, runtime) == node) {
            runtime.WriteWord(header + 4u, to_free);
        } else if (Read(parent, runtime) == node) {
            runtime.WriteWord(parent, to_free);
        } else {
            runtime.WriteWord(parent + 8u, to_free);
        }
        runtime.WriteWord(to_free + 4u, parent);
        const auto old_colour = Read(to_free + 44u, runtime);
        runtime.WriteWord(to_free + 44u,
                          Read(node + 44u, runtime));
        runtime.WriteWord(node + 44u, old_colour);
        to_free = node;
    } else {
        const auto parent = Read(node + 4u, runtime);
        runtime.WriteWord(child + 4u, parent);
        if (Read(header + 4u, runtime) == node) {
            runtime.WriteWord(header + 4u, child);
        } else if (Read(parent, runtime) == node) {
            runtime.WriteWord(parent, child);
        } else {
            runtime.WriteWord(parent + 8u, child);
        }
        if (Read(header, runtime) == node) {
            if (Read(node + 8u, runtime) == nil) {
                runtime.WriteWord(header, parent);
            } else {
                auto first = child;
                while (Read(first, runtime) != nil)
                    first = Read(first, runtime);
                runtime.WriteWord(header, first);
            }
        }
        if (Read(header + 8u, runtime) == node) {
            if (Read(node, runtime) == nil) {
                runtime.WriteWord(header + 8u, parent);
            } else {
                auto last = child;
                while (Read(last + 8u, runtime) != nil)
                    last = Read(last + 8u, runtime);
                runtime.WriteWord(header + 8u, last);
            }
        }
    }

    if (Read(to_free + 44u, runtime) == 1u)
        RepairAfterRemove(tree, header, child, runtime);
    runtime.SetErrorFlag(to_free + 28u, 1u);
    const auto buffer = Read(to_free + 16u, runtime);
    if (buffer != 0u) {
        const auto count = runtime.ReadByte(buffer - 1u);
        if (count == 0u || count == 0xffu)
            runtime.FreeMemory(buffer - 1u);
        else
            runtime.WriteByte(buffer - 1u,
                              static_cast<std::uint8_t>(count - 1u));
    }
    runtime.WriteWord(to_free + 16u, 0u);
    runtime.WriteWord(to_free + 20u, 0u);
    runtime.WriteWord(to_free + 24u, 0u);
    runtime.FreeMemory(to_free);
    runtime.WriteWord(tree + 12u,
                      Read(tree + 12u, runtime) - 1u);
    runtime.WriteWord(output, successor);
    return output;
}

std::uint32_t ScenarioPortraitTree::FreeNodes(
    std::uint32_t node,
    IScenarioPortraitTreeRuntime& runtime) {
    const auto nil = runtime.SharedNil();
    while (node != nil) {
        (void)FreeNodes(Read(node + 8u, runtime), runtime);
        const auto left = Read(node, runtime);
        runtime.CleanupProperties(node + 12u);
        runtime.FreeMemory(node);
        node = left;
    }
    return nil;
}

std::uint32_t ScenarioPortraitTree::Initialize(
    const std::uint32_t tree,
    IScenarioPortraitTreeRuntime& runtime) {
    const auto candidate = runtime.AllocateMemory(0x30u);
    runtime.WriteWord(candidate + 44u, 1u);
    runtime.WriteWord(candidate + 4u, 0u);
    if (runtime.SharedNil() == 0u) {
        runtime.SetSharedNil(candidate);
        runtime.WriteWord(candidate, 0u);
        runtime.WriteWord(candidate + 8u, 0u);
    } else {
        runtime.FreeMemory(candidate);
    }
    runtime.SetSharedNilRefCount(
        runtime.SharedNilRefCount() + 1u);
    const auto header = runtime.AllocateMemory(0x30u);
    runtime.WriteWord(header + 4u, runtime.SharedNil());
    runtime.WriteWord(header + 44u, 0u);
    runtime.WriteWord(tree + 4u, header);
    runtime.WriteWord(tree + 12u, 0u);
    runtime.WriteWord(header, header);
    runtime.WriteWord(header + 8u, header);
    return header;
}

std::uint32_t ScenarioPortraitTree::InsertUnique(
    const std::uint32_t tree, const std::uint32_t output,
    const std::uint32_t child, const std::uint32_t parent,
    const std::uint32_t value,
    IScenarioPortraitTreeRuntime& runtime) {
    const auto nil = runtime.SharedNil();
    const auto node = runtime.AllocateMemory(0x30u);
    runtime.WriteWord(node + 4u, parent);
    runtime.WriteWord(node + 44u, 0u);
    runtime.WriteWord(node, nil);
    runtime.WriteWord(node + 8u, nil);
    runtime.CopyData(node + 12u, value);
    const auto header = Read(tree + 4u, runtime);
    runtime.WriteWord(tree + 12u,
                      Read(tree + 12u, runtime) + 1u);
    LinkNode(tree, header, child, parent, value, node,
             runtime);
    runtime.WriteWord(output, node);
    return output;
}

std::uint32_t ScenarioPortraitTree::RotateRight(
    const std::uint32_t tree, const std::uint32_t node,
    IScenarioPortraitTreeRuntime& runtime) {
    const auto pivot = Read(node + 8u, runtime);
    const auto middle = Read(pivot, runtime);
    runtime.WriteWord(node + 8u, middle);
    if (middle != runtime.SharedNil())
        runtime.WriteWord(middle + 4u, node);
    const auto parent = Read(node + 4u, runtime);
    runtime.WriteWord(pivot + 4u, parent);
    const auto header = Read(tree + 4u, runtime);
    if (node == Read(header + 4u, runtime)) {
        runtime.WriteWord(header + 4u, pivot);
    } else if (node == Read(parent, runtime)) {
        runtime.WriteWord(parent, pivot);
    } else {
        runtime.WriteWord(parent + 8u, pivot);
    }
    runtime.WriteWord(pivot, node);
    runtime.WriteWord(node + 4u, pivot);
    return pivot;
}

std::uint32_t ScenarioPortraitTree::RotateLeft(
    const std::uint32_t tree, const std::uint32_t node,
    IScenarioPortraitTreeRuntime& runtime) {
    const auto pivot = Read(node, runtime);
    const auto middle = Read(pivot + 8u, runtime);
    runtime.WriteWord(node, middle);
    if (middle != runtime.SharedNil())
        runtime.WriteWord(middle + 4u, node);
    const auto parent = Read(node + 4u, runtime);
    runtime.WriteWord(pivot + 4u, parent);
    const auto header = Read(tree + 4u, runtime);
    if (node == Read(header + 4u, runtime)) {
        runtime.WriteWord(header + 4u, pivot);
    } else if (node == Read(parent + 8u, runtime)) {
        runtime.WriteWord(parent + 8u, pivot);
    } else {
        runtime.WriteWord(parent, pivot);
    }
    runtime.WriteWord(pivot + 8u, node);
    runtime.WriteWord(node + 4u, pivot);
    return pivot;
}

std::uint32_t ScenarioPortraitTree::AllocateNode(
    const std::uint32_t parent, const std::uint32_t colour,
    IScenarioPortraitTreeRuntime& runtime) {
    const auto node = runtime.AllocateMemory(0x30u);
    runtime.WriteWord(node + 4u, parent);
    runtime.WriteWord(node + 44u, colour);
    return node;
}

std::uint32_t ScenarioPortraitTree::Predecessor(
    std::uint32_t node,
    IScenarioPortraitTreeRuntime& runtime) {
    const auto parent = Read(node + 4u, runtime);
    if (Read(node + 44u, runtime) == 0u &&
        Read(parent + 4u, runtime) == node)
        return Read(node + 8u, runtime);

    auto left = Read(node, runtime);
    const auto nil = runtime.SharedNil();
    if (left != nil) {
        while (Read(left + 8u, runtime) != nil)
            left = Read(left + 8u, runtime);
        return left;
    }
    auto previous = node;
    auto ancestor = parent;
    while (previous == Read(ancestor, runtime)) {
        previous = ancestor;
        ancestor = Read(ancestor + 4u, runtime);
    }
    return ancestor;
}

std::uint32_t ScenarioPortraitTree::Successor(
    std::uint32_t node,
    IScenarioPortraitTreeRuntime& runtime) {
    const auto nil = runtime.SharedNil();
    auto right = Read(node + 8u, runtime);
    if (right != nil) {
        while (Read(right, runtime) != nil)
            right = Read(right, runtime);
        return right;
    }
    auto ancestor = Read(node + 4u, runtime);
    while (node == Read(ancestor + 8u, runtime)) {
        node = ancestor;
        ancestor = Read(ancestor + 4u, runtime);
    }
    if (Read(node + 8u, runtime) != ancestor)
        node = ancestor;
    return node;
}

void ScenarioPortraitTree::CopyData(
    const std::uint32_t destination,
    const std::uint32_t source,
    IScenarioPortraitTreeRuntime& runtime) {
    if (destination == 0u) return;
    runtime.WriteByte(destination, runtime.ReadByte(source));
    runtime.WriteWord(destination + 4u, 0u);
    runtime.WriteWord(destination + 8u, 0u);
    runtime.WriteWord(destination + 12u, 0u);
    runtime.AssignFirstBuffer(destination, source, 0u, 0xffffffffu);
    runtime.WriteByte(destination + 16u,
                      runtime.ReadByte(source + 16u));
    runtime.WriteWord(destination + 20u, 0u);
    runtime.WriteWord(destination + 24u, 0u);
    runtime.WriteWord(destination + 28u, 0u);
    const auto length = Read(source + 24u, runtime);
    const auto second = destination + 16u;
    if (second == source + 16u) {
        runtime.EraseBuffer(second, length, 0xffffffffu);
        runtime.EraseBuffer(second, 0u, 0u);
        return;
    }
    auto source_data = Read(source + 20u, runtime);
    if (source_data == 0u)
        source_data = runtime.EmptyData();
    if (length != 0u &&
        runtime.ReadByte(source_data - 1u) < 0xfeu) {
        runtime.WriteWord(second + 4u, source_data);
        runtime.WriteWord(second + 8u, length);
        runtime.WriteWord(second + 12u,
                          Read(source + 28u, runtime));
        runtime.WriteByte(source_data - 1u,
                          static_cast<std::uint8_t>(
                              runtime.ReadByte(source_data - 1u) + 1u));
        return;
    }
    if (runtime.AllocateText(second, length, 1u)) {
        runtime.CopyBytes(Read(second + 4u, runtime),
                          source_data, length);
        runtime.SetTextSize(second, length);
    }
}

}  // namespace pal4
