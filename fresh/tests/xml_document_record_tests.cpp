#include "pal4/xml_document_record.h"

#include <cstdlib>
#include <functional>
#include <iostream>
#include <map>
#include <new>
#include <set>
#include <stdexcept>
#include <string>
#include <tuple>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IXmlDocumentRecordRuntime {
    std::uint32_t AllocateSharedNil() override { return AllocateMemory(60u); }
    std::uint32_t FreeSharedNil(std::uint32_t address) override { return FreeMemory(address); }
    void DocumentReferenceAdded() noexcept override {}
    void DocumentReferenceRemoved() noexcept override {}
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::uint32_t> word_reads;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::uint32_t> allocations;
    std::vector<std::uint32_t> frees;
    std::vector<std::uint8_t> error_flags;
    std::vector<std::uint32_t> reallocation_lengths;
    std::vector<std::pair<std::uint32_t, std::uint32_t>> children;
    std::vector<std::uint32_t> deleted_nodes;
    std::vector<std::uint32_t> next_requests;
    std::vector<std::uint32_t> removed_nodes;
    std::vector<std::uint32_t> first_element_requests;
    std::vector<std::uint32_t> copied_attributes;
    std::vector<std::uint32_t> destroyed_attributes;
    std::vector<std::uint32_t> cleaned_nodes;
    std::vector<std::string> copy_events;
    std::vector<std::uint32_t> freed_attributes;
    std::vector<std::tuple<std::uint32_t, std::uint32_t,
                           std::uint32_t>> cleared_trees;
    std::vector<std::pair<std::uint32_t, std::uint32_t>> text_allocations;
    std::vector<std::uint32_t> text_reallocations;
    std::vector<std::uint32_t> text_detachments;
    std::vector<std::pair<std::uint32_t, std::uint32_t>> text_sizes;
    std::vector<std::pair<std::uint32_t, std::string>> attribute_queries;
    std::vector<std::pair<std::uint32_t, std::string>> attribute_insertions;
    std::vector<std::pair<std::uint32_t, std::uint32_t>> attribute_assignments;
    std::vector<std::uint32_t> destroyed_embedded_attributes;
    std::vector<std::pair<std::uint32_t, std::uint32_t>> tree_comparisons;
    std::vector<std::pair<std::uint32_t, std::uint32_t>> embedded_attribute_copies;
    std::vector<std::string> attribute_events;
    std::map<std::uint32_t, std::uint32_t> successors;
    std::uint32_t allocation_attempts = 0u;
    std::uint32_t fail_attempt = 0u;
    bool fail_reallocation = false;
    std::uint8_t text_allocation_result = 1u;
    std::uint8_t text_size_result = 7u;
    std::uint32_t reallocated_text_pointer = 0u;
    std::uint32_t allocated_text_pointer = 0u;
    std::uint32_t detached_length_override = 0xffffffffu;
    std::uint32_t attribute_find_result = 0u;
    std::uint32_t attribute_insert_result = 0x9000u;
    bool fail_attribute_insert = false;
    bool fail_attribute_assign = false;

    std::uint32_t AllocateMemory(std::uint32_t size) override {
        Check(size == 60u || (size != 0u && size % 4u == 0u),
              "document nodes and child-pointer vectors use expected sizes");
        ++allocation_attempts;
        if (allocation_attempts == fail_attempt) return 0u;
        const auto result = allocation_attempts == 1u
                                ? 0x7000u
                                : allocation_attempts == 2u
                                      ? 0x8000u
                                      : 0x100000u + allocation_attempts * 0x1000u;
        allocations.push_back(result);
        return result;
    }
    std::uint32_t FreeMemory(std::uint32_t address) override {
        frees.push_back(address);
        return 0xfaceu;
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        word_reads.push_back(address);
        return words[address];
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        bytes[address] = value;
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        return bytes[address];
    }
    void SetErrorFlag(std::uint32_t record, std::uint8_t flag) override {
        Check(record != 0u, "error flag targets a live document");
        copy_events.push_back("reset");
        error_flags.push_back(flag);
        words[record + 4u] = 0u;
        words[record + 8u] = 0u;
        words[record + 12u] = 0u;
    }
    void Reallocate(std::uint32_t record, std::uint32_t length) override {
        Check(record != 0u, "reallocation targets a live document");
        copy_events.push_back("reallocate");
        reallocation_lengths.push_back(length);
        if (fail_reallocation) throw std::bad_alloc{};
        words[record + 4u] =
            record == 0x3000u ? 0xa000u : record + 0x200000u;
        words[record + 12u] = length;
    }
    void AddChild(std::uint32_t parent, std::uint32_t child) override {
        copy_events.push_back("add_child");
        children.emplace_back(parent, child);
    }
    void DeleteNode(std::uint32_t node) override {
        deleted_nodes.push_back(node);
    }
    std::uint32_t GetNextNode(std::uint32_t node) override {
        next_requests.push_back(node);
        return successors.at(node);
    }
    void RemoveNode(std::uint32_t document, std::uint32_t node) override {
        Check(document == 0x3014u, "node removal targets the tree subobject");
        removed_nodes.push_back(node);
    }
    void GetFirstElement(std::uint32_t document) override {
        first_element_requests.push_back(document);
    }
    std::uint32_t EmptyTextAddress() override { return 0xe000u; }
    std::uint32_t CopyAttributeToTemporary(std::uint32_t source,
        std::span<std::uint32_t, 7>) override {
        copied_attributes.push_back(source);
        return 0xd000u;
    }
    void DestroyTemporaryAttribute(std::uint32_t temporary) override {
        destroyed_attributes.push_back(temporary);
    }
    void CleanupNode(std::uint32_t node) override {
        copy_events.push_back("cleanup");
        cleaned_nodes.push_back(node);
    }
    void FreeAttributeValue(std::uint32_t attribute) override {
        freed_attributes.push_back(attribute);
    }
    void ClearAttributeTree(std::uint32_t tree, std::uint32_t first,
                            std::uint32_t head) override {
        cleared_trees.emplace_back(tree, first, head);
        words[head] = head;
    }
    std::uint8_t AllocateTextBuffer(std::uint32_t node,
                                    std::uint32_t length) override {
        text_allocations.emplace_back(node, length);
        if (text_allocation_result != 0u && allocated_text_pointer != 0u)
            words[node + 4u] = allocated_text_pointer;
        return text_allocation_result;
    }
    void ReallocateTextIfNeeded(std::uint32_t node) override {
        text_reallocations.push_back(node);
        if (reallocated_text_pointer != 0u)
            words[node + 4u] = reallocated_text_pointer;
    }
    void DetachTextBuffer(std::uint32_t node) override {
        text_detachments.push_back(node);
        if (detached_length_override != 0xffffffffu)
            words[node + 8u] = detached_length_override;
    }
    std::uint8_t SetTextSize(std::uint32_t node,
                             std::uint32_t length) override {
        text_sizes.emplace_back(node, length);
        words[node + 8u] = length;
        return text_size_result;
    }
    std::uint32_t FindAttributeNode(std::uint32_t tree,
                                    std::string_view name) override {
        attribute_events.push_back("find");
        attribute_queries.emplace_back(tree, name);
        return attribute_find_result != 0u
                   ? attribute_find_result
                   : words[tree + 4u];
    }
    std::uint32_t FindAttributeNodeForAddition(std::uint32_t tree,
                                      std::string_view name) override {
        return FindAttributeNode(tree, name);
    }
    std::uint32_t InsertAttributeNode(std::uint32_t tree,
        std::string_view name, std::uint32_t source) override {
        attribute_events.push_back("insert");
        attribute_insertions.emplace_back(tree, name);
        if (fail_attribute_insert) throw std::bad_alloc{};
        AssignAttributeValue(attribute_insert_result + 28u, source);
        return attribute_insert_result;
    }
    void AssignAttributeValue(std::uint32_t destination,
                              std::uint32_t source) {
        attribute_events.push_back("assign");
        attribute_assignments.emplace_back(destination, source);
        if (fail_attribute_assign) throw std::bad_alloc{};
    }
    void DestroyEmbeddedAttribute(std::uint32_t attribute) override {
        destroyed_embedded_attributes.push_back(attribute);
    }
    std::int32_t CompareNames(std::uint32_t first, std::uint32_t second) {
        const auto first_length = ReadWord(first + 8u);
        const auto second_length = ReadWord(second + 8u);
        auto first_text = ReadWord(first + 4u);
        auto second_text = ReadWord(second + 4u);
        if (first_text == 0u) first_text = EmptyTextAddress();
        if (second_text == 0u) second_text = EmptyTextAddress();
        const auto common = first_length < second_length
                                ? first_length : second_length;
        for (std::uint32_t index = 0u; index < common; ++index) {
            const auto left = ReadByte(first_text + index);
            const auto right = ReadByte(second_text + index);
            if (left != right) return left < right ? -1 : 1;
        }
        return first_length == second_length
                   ? 0 : first_length < second_length ? -1 : 1;
    }
    std::int32_t CompareAttributeRange(std::uint32_t input,
                                       std::uint32_t node_text) override {
        tree_comparisons.emplace_back(input, node_text);
        return CompareNames(input, node_text);
    }
    std::int32_t CompareAttributeText(std::uint32_t first,
                                      std::uint32_t second) override {
        tree_comparisons.emplace_back(first, second);
        return CompareNames(first, second);
    }
    void CopyEmbeddedAttribute(std::uint32_t destination,
                               std::uint32_t source) override {
        embedded_attribute_copies.emplace_back(destination, source);
    }
};

void CheckDocumentFields(Runtime& runtime) {
    Check(runtime.words[0x3004u] == 0u &&
              runtime.words[0x3008u] == 0u &&
              runtime.words[0x300cu] == 0u &&
              runtime.words[0x3010u] == 0u &&
              runtime.words[0x3018u] == 0x8000u &&
              runtime.words[0x3020u] == 0u &&
              runtime.words[0x3028u] == 0u &&
              runtime.words[0x302cu] == 0u &&
              runtime.words[0x3030u] == 0u &&
              runtime.words[0x3034u] == 0u &&
              runtime.words[0x3038u] == 0u &&
              runtime.bytes[0x301cu] == 0u,
          "document fields match IDA's 60-byte layout");
}

void TestFirstDocumentEstablishesNilNode() {
    Runtime runtime;
    pal4::XmlDocumentTreeGlobals globals;
    runtime.bytes[0x3000u] = 0x7bu;
    runtime.bytes[0x3014u] = 0x11u;
    runtime.bytes[0x3015u] = 0x22u;
    runtime.bytes[0x3024u] = 0x33u;
    Check(pal4::XmlDocumentRecord::Initialize(0x3000u, globals, runtime) ==
              0x3000u &&
              globals.nil_node == 0x7000u &&
              globals.construction_count == 1u &&
              runtime.allocations ==
                  (std::vector<std::uint32_t>{0x7000u, 0x8000u}) &&
              runtime.frees.empty() &&
              runtime.words[0x7000u] == 0u &&
              runtime.words[0x7004u] == 0u &&
              runtime.words[0x7008u] == 0u &&
              runtime.words[0x7038u] == 1u &&
              runtime.words[0x8000u] == 0x8000u &&
              runtime.words[0x8004u] == 0x7000u &&
              runtime.words[0x8008u] == 0x8000u &&
              runtime.words[0x8038u] == 0u &&
              runtime.bytes[0x3000u] == 0x7bu &&
              runtime.bytes[0x3014u] == 0x11u &&
              runtime.bytes[0x3015u] == 0x22u &&
              runtime.bytes[0x3024u] == 0x33u,
          "first document installs the nil node and retains indeterminate bytes");
    CheckDocumentFields(runtime);
}

void TestLaterDocumentDiscardsProposedNil() {
    Runtime runtime;
    pal4::XmlDocumentTreeGlobals globals{0x9000u, 7u};
    Check(pal4::XmlDocumentRecord::Initialize(0x3000u, globals, runtime) ==
              0x3000u &&
              globals.nil_node == 0x9000u &&
              globals.construction_count == 8u &&
              runtime.frees == std::vector<std::uint32_t>{0x7000u} &&
              runtime.words[0x7004u] == 0u &&
              runtime.words[0x7038u] == 1u &&
              runtime.words[0x8004u] == 0x9000u &&
              runtime.words[0x8000u] == 0x8000u &&
              runtime.words[0x8008u] == 0x8000u,
          "later document frees the proposed nil and uses shared nil");
    CheckDocumentFields(runtime);
}

void TestAllocationFailures() {
    Runtime first_failure;
    first_failure.fail_attempt = 1u;
    pal4::XmlDocumentTreeGlobals first_globals;
    bool first_caught = false;
    try {
        (void)pal4::XmlDocumentRecord::Initialize(
            0x3000u, first_globals, first_failure);
    } catch (const std::bad_alloc&) {
        first_caught = true;
    }
    Check(first_caught && first_globals.nil_node == 0u &&
              first_globals.construction_count == 0u &&
              first_failure.allocation_attempts == 1u,
          "first allocation failure stops before global state changes");

    Runtime second_failure;
    second_failure.fail_attempt = 2u;
    pal4::XmlDocumentTreeGlobals second_globals;
    bool second_caught = false;
    try {
        (void)pal4::XmlDocumentRecord::Initialize(
            0x3000u, second_globals, second_failure);
    } catch (const std::bad_alloc&) {
        second_caught = true;
    }
    Check(second_caught && second_globals.nil_node == 0u &&
              second_globals.construction_count == 0u &&
              second_failure.allocation_attempts == 2u &&
              second_failure.frees == std::vector<std::uint32_t>{0x7000u},
          "second allocation failure releases newly initialized shared nil");
}

void TestStringAndParentInitialization() {
    Runtime runtime;
    pal4::XmlDocumentTreeGlobals globals;
    runtime.bytes[0x9102u] = 'r';
    runtime.bytes[0x9103u] = 'o';
    runtime.bytes[0x9104u] = 'o';
    runtime.bytes[0x9105u] = 't';
    runtime.bytes[0x9106u] = 0u;
    Check(pal4::XmlDocumentRecord::InitializeFromString(
              0x3000u, 0x9102u, 0x4000u, globals, runtime) == 0x3000u,
          "string constructor returns its document");
    Check(runtime.bytes[0x3000u] == 0x02u &&
              runtime.bytes[0x3014u] == 0x02u &&
              runtime.bytes[0x3015u] == 0x02u &&
              runtime.bytes[0x3024u] == 0x02u &&
              runtime.words[0x3004u] == 0xa000u &&
              runtime.words[0x3008u] == 4u &&
              runtime.words[0x300cu] == 4u &&
              runtime.words[0x3010u] == 0x4000u &&
              runtime.words[0x3018u] == 0x8000u &&
              runtime.words[0x3034u] == 0u &&
              runtime.words[0x3038u] == 0u &&
              runtime.bytes[0xa000u] == 'r' &&
              runtime.bytes[0xa001u] == 'o' &&
              runtime.bytes[0xa002u] == 'o' &&
              runtime.bytes[0xa003u] == 't' &&
              runtime.bytes[0xa004u] == 0u &&
              runtime.error_flags == std::vector<std::uint8_t>{1u} &&
              runtime.reallocation_lengths == std::vector<std::uint32_t>{4u} &&
              runtime.children ==
                  (std::vector<std::pair<std::uint32_t, std::uint32_t>>{
                      {0x4000u, 0x3000u}}),
          "nonempty name and parent follow the IDA initialization path");
}

void TestEmptyAndNullSource() {
    for (const auto source : {0u, 0x9200u}) {
        Runtime runtime;
        pal4::XmlDocumentTreeGlobals globals{0x9000u, 7u};
        runtime.bytes[0x9200u] = 0u;
        Check(pal4::XmlDocumentRecord::InitializeFromString(
                  0x3000u, source, 0u, globals, runtime) == 0x3000u &&
                  globals.nil_node == 0x9000u &&
                  globals.construction_count == 8u &&
                  runtime.frees == std::vector<std::uint32_t>{0x7000u} &&
                  runtime.words[0x8004u] == 0x9000u &&
                  runtime.words[0x3004u] == 0u &&
                  runtime.words[0x3008u] == 0u &&
                  runtime.words[0x300cu] == 0u &&
                  runtime.words[0x3010u] == 0u &&
                  runtime.error_flags == std::vector<std::uint8_t>{1u} &&
                  runtime.reallocation_lengths.empty() &&
                  runtime.children.empty(),
              "empty source leaves name unallocated and skips parent callback");
        Check(runtime.bytes[0x3000u] == static_cast<std::uint8_t>(source) &&
                  runtime.bytes[0x3014u] == static_cast<std::uint8_t>(source) &&
                  runtime.bytes[0x3015u] == static_cast<std::uint8_t>(source) &&
                  runtime.bytes[0x3024u] == static_cast<std::uint8_t>(source),
              "source low byte is repeated at four document offsets");
    }
}

void TestStringConstructorFailures() {
    for (const auto failed_attempt : {1u, 2u}) {
        Runtime runtime;
        pal4::XmlDocumentTreeGlobals globals;
        runtime.fail_attempt = failed_attempt;
        bool caught = false;
        try {
            (void)pal4::XmlDocumentRecord::InitializeFromString(
                0x3000u, 0x9200u, 0x4000u, globals, runtime);
        } catch (const std::bad_alloc&) {
            caught = true;
        }
        Check(caught && runtime.allocation_attempts == failed_attempt &&
                  runtime.children.empty() && runtime.error_flags.empty(),
              "node allocation failure stops before text and parent updates");
    }

    Runtime runtime;
    pal4::XmlDocumentTreeGlobals globals;
    runtime.bytes[0x9200u] = 'x';
    runtime.bytes[0x9201u] = 0u;
    runtime.fail_reallocation = true;
    bool caught = false;
    try {
        (void)pal4::XmlDocumentRecord::InitializeFromString(
            0x3000u, 0x9200u, 0x4000u, globals, runtime);
    } catch (const std::bad_alloc&) {
        caught = true;
    }
    Check(caught && runtime.error_flags == std::vector<std::uint8_t>{1u} &&
              runtime.reallocation_lengths == std::vector<std::uint32_t>{1u} &&
              runtime.children.empty() && runtime.words[0x3010u] == 0u,
          "text allocation failure stops before parent attachment");
}

void TestDestructorReleasesSharedNilOnLastDocument() {
    Runtime runtime;
    pal4::XmlDocumentTreeGlobals globals;
    (void)pal4::XmlDocumentRecord::Initialize(0x3000u, globals, runtime);
    Check(pal4::XmlDocumentRecord::Destroy(0x3000u, globals, runtime) ==
              0xfaceu &&
              globals.nil_node == 0u &&
              globals.construction_count == 0u &&
              runtime.frees ==
                  (std::vector<std::uint32_t>{0x8000u, 0x7000u}) &&
              runtime.words[0x3018u] == 0u &&
              runtime.words[0x3020u] == 0u &&
              runtime.deleted_nodes.empty() &&
              runtime.next_requests.empty() &&
              runtime.removed_nodes.empty(),
          "last empty document releases head and shared nil in order");
}

void TestDestructorRetainsSharedNilWithOtherDocuments() {
    Runtime runtime;
    pal4::XmlDocumentTreeGlobals globals{0x9000u, 2u};
    runtime.words[0x3018u] = 0x8000u;
    runtime.words[0x8000u] = 0x8000u;
    Check(pal4::XmlDocumentRecord::Destroy(0x3000u, globals, runtime) == 1u &&
              globals.nil_node == 0x9000u &&
              globals.construction_count == 1u &&
              runtime.frees == std::vector<std::uint32_t>{0x8000u},
          "shared nil survives until the final document is destroyed");
}

void TestDestructorWholeTreePath() {
    Runtime runtime;
    pal4::XmlDocumentTreeGlobals globals{0x9000u, 1u};
    runtime.words[0x3018u] = 0x8000u;
    runtime.words[0x3020u] = 3u;
    runtime.words[0x8000u] = 0xb000u;
    runtime.words[0x8004u] = 0xa000u;
    runtime.words[0x8008u] = 0xc000u;
    Check(pal4::XmlDocumentRecord::Destroy(0x3000u, globals, runtime) ==
              0xfaceu &&
              runtime.deleted_nodes == std::vector<std::uint32_t>{0xa000u} &&
              runtime.first_element_requests ==
                  std::vector<std::uint32_t>{0x3014u} &&
              runtime.removed_nodes.empty() &&
              runtime.words[0x8000u] == 0x8000u &&
              runtime.words[0x8004u] == 0x9000u &&
              runtime.words[0x8008u] == 0x8000u &&
              runtime.words[0x3020u] == 0u &&
              runtime.frees ==
                  (std::vector<std::uint32_t>{0x8000u, 0x9000u}),
          "nonzero tree count deletes root then resets and frees header");
}

void TestDestructorIteratesUncountedNodes() {
    Runtime runtime;
    pal4::XmlDocumentTreeGlobals globals{0x9000u, 2u};
    runtime.words[0x3018u] = 0x8000u;
    runtime.words[0x8000u] = 0xa000u;
    runtime.successors[0xa000u] = 0xb000u;
    runtime.successors[0xb000u] = 0x8000u;
    Check(pal4::XmlDocumentRecord::Destroy(0x3000u, globals, runtime) == 1u &&
              runtime.next_requests ==
                  (std::vector<std::uint32_t>{0xa000u, 0xb000u}) &&
              runtime.removed_nodes ==
                  (std::vector<std::uint32_t>{0xa000u, 0xb000u}) &&
              runtime.deleted_nodes.empty() &&
              runtime.first_element_requests.empty() &&
              runtime.frees == std::vector<std::uint32_t>{0x8000u},
          "zero count walks nodes, computes successor before removal");
}

void TestCopyConstructorCopiesAttributesAndChildren() {
    Runtime runtime;
    pal4::XmlDocumentTreeGlobals globals;
    runtime.words[0x5007u] = 0x9900u;
    runtime.words[0x501bu] = 0xc000u;
    runtime.words[0x5037u] = 2u;
    runtime.words[0x503bu] = 2u;
    runtime.words[0xc000u] = 0xc100u;
    runtime.successors[0xc100u] = 0xc200u;
    runtime.successors[0xc200u] = 0xc000u;
    runtime.words[0x502bu] = 0xf000u;
    runtime.words[0x502fu] = 0xf008u;
    runtime.words[0xf000u] = 0x6200u;
    runtime.words[0xf004u] = 0x6300u;
    runtime.bytes[0x9900u] = 'r';
    runtime.bytes[0x9901u] = 'o';
    runtime.bytes[0x9902u] = 'o';
    runtime.bytes[0x9903u] = 't';
    runtime.bytes[0x9904u] = 0u;
    Check(pal4::XmlDocumentRecord::CopyConstruct(
              0x3000u, 0x5003u, globals, runtime) == 0x3000u &&
              globals.construction_count == 3u &&
              runtime.bytes[0x3000u] == 3u &&
              runtime.bytes[0x3014u] == 3u &&
              runtime.bytes[0x3015u] == 3u &&
              runtime.bytes[0x3024u] == 3u &&
              runtime.error_flags ==
                  (std::vector<std::uint8_t>{0u, 1u, 1u, 1u}) &&
              runtime.cleaned_nodes ==
                  std::vector<std::uint32_t>{0x3000u} &&
              runtime.words[0x3004u] == 0xa000u &&
              runtime.words[0x3008u] == 4u &&
              runtime.bytes[0xa000u] == 'r' &&
              runtime.bytes[0xa004u] == 0u &&
              runtime.copied_attributes ==
                  (std::vector<std::uint32_t>{0xc11cu, 0xc21cu}) &&
              runtime.attribute_assignments ==
                  (std::vector<std::pair<std::uint32_t,
                                         std::uint32_t>>{
                      {0x901cu, 0xd000u}, {0x901cu, 0xd000u}}) &&
              runtime.words[0x3034u] == 2u &&
              runtime.destroyed_attributes ==
                  (std::vector<std::uint32_t>{0xd000u, 0xd000u}) &&
              runtime.next_requests ==
                  (std::vector<std::uint32_t>{0xc100u, 0xc200u}) &&
              runtime.children ==
                  (std::vector<std::pair<std::uint32_t, std::uint32_t>>{
                      {0x3000u, 0x103000u},
                      {0x3000u, 0x106000u}}) &&
              runtime.allocations.size() == 8u &&
              runtime.frees ==
                  (std::vector<std::uint32_t>{0x104000u, 0x107000u}),
          "copy constructor traverses attributes before child slots");
}

void TestCopyConstructorNameFallback() {
    Runtime runtime;
    pal4::XmlDocumentTreeGlobals globals{0x9000u, 3u};
    runtime.words[0x6004u] = 0u;
    Check(pal4::XmlDocumentRecord::CopyConstruct(
              0x3000u, 0x6000u, globals, runtime) == 0x3000u &&
              globals.nil_node == 0x9000u &&
              globals.construction_count == 4u &&
              runtime.error_flags ==
                  (std::vector<std::uint8_t>{0u, 1u}) &&
              runtime.words[0x3004u] == 0u &&
              runtime.reallocation_lengths.empty() &&
              runtime.copied_attributes.empty() &&
              runtime.children.empty(),
          "null source name selects the native empty literal");
}

void TestCopyConstructorCleansTemporaryAfterInsertFailure() {
    Runtime runtime;
    pal4::XmlDocumentTreeGlobals globals;
    runtime.fail_attribute_insert = true;
    runtime.words[0x6010u] = 0u;
    runtime.words[0x6018u] = 0xc000u;
    runtime.words[0x6034u] = 1u;
    runtime.words[0xc000u] = 0xc100u;
    bool caught = false;
    try {
        (void)pal4::XmlDocumentRecord::CopyConstruct(
            0x3000u, 0x6000u, globals, runtime);
    } catch (const std::bad_alloc&) {
        caught = true;
    }
    Check(caught && runtime.copied_attributes ==
                        std::vector<std::uint32_t>{0xc11cu} &&
              runtime.attribute_events ==
                  (std::vector<std::string>{"find", "insert"}) &&
              runtime.destroyed_attributes ==
                  std::vector<std::uint32_t>{0xd000u} &&
              runtime.children.empty(),
          "temporary attribute is destroyed when insertion throws");
}

void TestCopyAssignmentReusesCopyFlow() {
    Runtime runtime;
    pal4::XmlDocumentTreeGlobals globals{0x9000u, 1u};
    runtime.words[0x6004u] = 0x9900u;
    runtime.words[0x6018u] = 0xc000u;
    runtime.words[0x6034u] = 1u;
    runtime.words[0xc000u] = 0xc100u;
    runtime.successors[0xc100u] = 0xc000u;
    runtime.words[0x6038u] = 1u;
    runtime.words[0x6028u] = 0xf000u;
    runtime.words[0x602cu] = 0xf004u;
    runtime.words[0xf000u] = 0x6200u;
    runtime.bytes[0x9900u] = 'r';
    runtime.bytes[0x9901u] = 'o';
    runtime.bytes[0x9902u] = 'o';
    runtime.bytes[0x9903u] = 't';
    runtime.bytes[0x9904u] = 0u;
    Check(pal4::XmlDocumentRecord::CopyAssign(
              0x3000u, 0x6000u, globals, runtime) == 0x3000u &&
              runtime.cleaned_nodes ==
                  (std::vector<std::uint32_t>{0x3000u, 0x3000u}) &&
              runtime.copy_events ==
                  (std::vector<std::string>{
                      "cleanup", "cleanup", "reset", "reallocate",
                      "reset", "add_child"}) &&
              runtime.words[0x3004u] == 0xa000u &&
              runtime.words[0x3008u] == 4u &&
              runtime.copied_attributes ==
                  std::vector<std::uint32_t>{0xc11cu} &&
              runtime.destroyed_attributes ==
                  std::vector<std::uint32_t>{0xd000u} &&
              runtime.next_requests ==
                  std::vector<std::uint32_t>{0xc100u} &&
              runtime.children ==
                  (std::vector<std::pair<std::uint32_t, std::uint32_t>>{
                      {0x3000u, 0x7000u}}) &&
              globals.construction_count == 2u &&
              runtime.allocations ==
                  (std::vector<std::uint32_t>{
                      0x7000u, 0x8000u, 0x103000u}) &&
              runtime.frees == std::vector<std::uint32_t>{0x8000u},
          "assignment cleans destination then copies name, attributes and child");
}

void TestCopyAssignmentWithEmptyName() {
    Runtime runtime;
    pal4::XmlDocumentTreeGlobals globals{0x9000u, 1u};
    runtime.words[0x6004u] = 0u;
    Check(pal4::XmlDocumentRecord::CopyAssign(
              0x3000u, 0x6000u, globals, runtime) == 0x3000u &&
              runtime.cleaned_nodes ==
                  (std::vector<std::uint32_t>{0x3000u, 0x3000u}) &&
              runtime.error_flags == std::vector<std::uint8_t>{1u} &&
              runtime.words[0x3004u] == 0u &&
              runtime.copied_attributes.empty() &&
              runtime.children.empty(),
          "assignment clears the name when the source name is absent");
}

void TestSetNameNullAndEmptySources() {
    Runtime null_runtime;
    null_runtime.words[0x3004u] = 0xa001u;
    null_runtime.words[0x3008u] = 3u;
    null_runtime.words[0x300cu] = 8u;
    Check(pal4::XmlDocumentRecord::SetName(
              0x3000u, 0u, 0x4000u, null_runtime) == 1u &&
              null_runtime.cleaned_nodes ==
                  std::vector<std::uint32_t>{0x3000u} &&
              null_runtime.words[0x3004u] == 0xa001u &&
              null_runtime.words[0x3008u] == 3u &&
              null_runtime.words[0x300cu] == 8u &&
              null_runtime.words[0x3010u] == 0x4000u &&
              null_runtime.error_flags.empty() &&
              null_runtime.children ==
                  (std::vector<std::pair<std::uint32_t, std::uint32_t>>{
                      {0x4000u, 0x3000u}}),
          "null name preserves name storage and attaches optional parent");

    Runtime empty_runtime;
    empty_runtime.bytes[0x9100u] = 0u;
    empty_runtime.words[0x3004u] = 0xa001u;
    empty_runtime.bytes[0xa000u] = 2u;
    Check(pal4::XmlDocumentRecord::SetName(
              0x3000u, 0x9100u, 0u, empty_runtime) == 1u &&
              empty_runtime.bytes[0xa000u] == 1u &&
              empty_runtime.words[0x3004u] == 0u &&
              empty_runtime.words[0x3008u] == 0u &&
              empty_runtime.words[0x300cu] == 0u &&
              empty_runtime.error_flags.empty() &&
              empty_runtime.reallocation_lengths.empty(),
          "empty name decrements a shared buffer without freeing it");

    Runtime unique_runtime;
    unique_runtime.bytes[0x9100u] = 0u;
    unique_runtime.words[0x3004u] = 0xa001u;
    unique_runtime.bytes[0xa000u] = 0xffu;
    Check(pal4::XmlDocumentRecord::SetName(
              0x3000u, 0x9100u, 0u, unique_runtime) == 1u &&
              unique_runtime.error_flags ==
                  std::vector<std::uint8_t>{1u} &&
              unique_runtime.words[0x3004u] == 0u &&
              unique_runtime.children.empty(),
          "empty name resets an exclusive name buffer");
}

void TestSetNameCapacityAndSharing() {
    for (const auto capacity : {4u, 32u}) {
        Runtime runtime;
        runtime.words[0x3004u] = 0xa001u;
        runtime.words[0x300cu] = capacity;
        runtime.bytes[0xa000u] = 0xffu;
        runtime.bytes[0x9100u] = 'n';
        runtime.bytes[0x9101u] = 'a';
        runtime.bytes[0x9102u] = 'm';
        runtime.bytes[0x9103u] = 'e';
        runtime.bytes[0x9104u] = 0u;
        const auto expected_destination =
            capacity == 4u ? 0xa001u : 0xa000u;
        Check(pal4::XmlDocumentRecord::SetName(
                  0x3000u, 0x9100u, 0x4000u, runtime) == 1u &&
                  runtime.words[0x3004u] == expected_destination &&
                  runtime.words[0x3008u] == 4u &&
                  runtime.bytes[expected_destination] == 'n' &&
                  runtime.bytes[expected_destination + 1u] == 'a' &&
                  runtime.bytes[expected_destination + 2u] == 'm' &&
                  runtime.bytes[expected_destination + 3u] == 'e' &&
                  runtime.bytes[expected_destination + 4u] == 0u &&
                  runtime.children ==
                      (std::vector<std::pair<std::uint32_t,
                                             std::uint32_t>>{
                          {0x4000u, 0x3000u}}),
              "nonempty name copies bytes and attaches parent");
        if (capacity == 4u) {
            Check(runtime.reallocation_lengths.empty() &&
                      runtime.error_flags.empty(),
                  "adequate small exclusive capacity is reused");
        } else {
            Check(runtime.reallocation_lengths ==
                          std::vector<std::uint32_t>{4u} &&
                      runtime.error_flags ==
                          std::vector<std::uint8_t>{1u},
                  "capacity above 31 resets and reallocates");
        }
    }

    Runtime shared_runtime;
    shared_runtime.words[0x3004u] = 0xb001u;
    shared_runtime.words[0x300cu] = 10u;
    shared_runtime.bytes[0xb000u] = 2u;
    shared_runtime.bytes[0x9100u] = 'x';
    shared_runtime.bytes[0x9101u] = 0u;
    Check(pal4::XmlDocumentRecord::SetName(
              0x3000u, 0x9100u, 0u, shared_runtime) == 1u &&
              shared_runtime.reallocation_lengths ==
                  std::vector<std::uint32_t>{1u} &&
              shared_runtime.error_flags.empty() &&
              shared_runtime.words[0x3004u] == 0xa000u &&
              shared_runtime.bytes[0xa000u] == 'x' &&
              shared_runtime.bytes[0xa001u] == 0u,
          "shared name storage reallocates without the exclusive reset");
}

void TestSetNameAllocationFailureStopsBeforeParent() {
    Runtime runtime;
    runtime.fail_reallocation = true;
    runtime.bytes[0x9100u] = 'x';
    runtime.bytes[0x9101u] = 0u;
    bool caught = false;
    try {
        (void)pal4::XmlDocumentRecord::SetName(
            0x3000u, 0x9100u, 0x4000u, runtime);
    } catch (const std::bad_alloc&) {
        caught = true;
    }
    Check(caught && runtime.cleaned_nodes ==
                        std::vector<std::uint32_t>{0x3000u} &&
              runtime.error_flags == std::vector<std::uint8_t>{1u} &&
              runtime.reallocation_lengths ==
                  std::vector<std::uint32_t>{1u} &&
              runtime.children.empty() &&
              runtime.words[0x3010u] == 0u,
          "name allocation failure leaves parent assignment pending");
}

void TestCloneCopiesAttributesAndNestedChild() {
    Runtime runtime;
    pal4::XmlDocumentTreeGlobals globals;
    runtime.words[0x6004u] = 0x9900u;
    runtime.bytes[0x9900u] = 'p';
    runtime.bytes[0x9901u] = 0u;
    runtime.words[0x6018u] = 0xc000u;
    runtime.words[0x6034u] = 1u;
    runtime.words[0xc000u] = 0xc100u;
    runtime.successors[0xc100u] = 0xc000u;
    runtime.words[0x6038u] = 1u;
    runtime.words[0x6028u] = 0xf000u;
    runtime.words[0x602cu] = 0xf004u;
    runtime.words[0xf000u] = 0x6200u;
    runtime.words[0x6204u] = 0x9a00u;
    runtime.bytes[0x9a00u] = 'q';
    runtime.bytes[0x9a01u] = 0u;
    Check(pal4::XmlDocumentRecord::Clone(
              0x6000u, globals, runtime) == 0x7000u &&
              globals.nil_node == 0x8000u &&
              globals.construction_count == 2u &&
              runtime.allocations.size() == 6u &&
              runtime.frees == std::vector<std::uint32_t>{0x105000u} &&
              runtime.words[0x7004u] == 0x207000u &&
              runtime.words[0x7008u] == 1u &&
              runtime.bytes[0x207000u] == 'p' &&
              runtime.bytes[0x207001u] == 0u &&
              runtime.words[0x104004u] == 0x304000u &&
              runtime.words[0x104008u] == 1u &&
              runtime.bytes[0x304000u] == 'q' &&
              runtime.bytes[0x304001u] == 0u &&
              runtime.copied_attributes ==
                  std::vector<std::uint32_t>{0xc11cu} &&
              runtime.attribute_queries ==
                  (std::vector<std::pair<std::uint32_t,
                                         std::string>>{
                      {0x7014u, ""}}) &&
              runtime.words[0x7034u] == 1u &&
              runtime.destroyed_attributes ==
                  std::vector<std::uint32_t>{0xd000u} &&
              runtime.children ==
                  (std::vector<std::pair<std::uint32_t,
                                         std::uint32_t>>{
                      {0x7000u, 0x104000u}}),
          "clone constructs an independent tree with copied attributes and child");
}

void TestCloneAllocationFailures() {
    for (const auto failed_attempt : {1u, 2u, 3u}) {
        Runtime runtime;
        pal4::XmlDocumentTreeGlobals globals;
        runtime.fail_attempt = failed_attempt;
        bool caught = false;
        try {
            (void)pal4::XmlDocumentRecord::Clone(
                0x6000u, globals, runtime);
        } catch (const std::bad_alloc&) {
            caught = true;
        }
        Check(caught && runtime.allocation_attempts == failed_attempt &&
                  runtime.frees ==
                      (failed_attempt == 1u
                           ? std::vector<std::uint32_t>{}
                            : failed_attempt == 2u
                                ? std::vector<std::uint32_t>{0x7000u}
                                : std::vector<std::uint32_t>{0x8000u,
                                                              0x7000u}) &&
                   globals.construction_count == 0u &&
                   globals.nil_node == 0u,
              "clone reports allocation failure and frees raw node after construction failure");
    }
}

void TestAssignmentResultFollowsSourceCounts() {
    struct Counts {
        std::uint32_t attributes;
        std::uint32_t children;
        std::uint8_t expected;
    };
    for (const auto counts : {
             Counts{0u, 0u, 0u},
             Counts{1u, 0u, 1u},
             Counts{0u, 1u, 1u},
             Counts{0xffffffffu, 0xffffffffu, 0u}}) {
        Runtime runtime;
        pal4::XmlDocumentTreeGlobals globals{0x9000u, 1u};
        runtime.words[0x6034u] = counts.attributes;
        runtime.words[0x6038u] = counts.children;
        Check(pal4::XmlDocumentRecord::AssignWithResult(
                  0x3000u, 0x6000u, globals, runtime) == counts.expected &&
                  runtime.cleaned_nodes ==
                      (std::vector<std::uint32_t>{0x3000u, 0x3000u}) &&
                  runtime.error_flags ==
                      std::vector<std::uint8_t>{1u} &&
                  runtime.allocations.empty() &&
                  globals.construction_count == 1u,
              "assignment returns whether source has positive attribute or child count");
    }
}

void TestCleanupDetachesFromParent() {
    Runtime runtime;
    pal4::XmlDocumentTreeGlobals globals{0x9000u, 2u};
    runtime.words[0x3010u] = 0x4000u;
    runtime.words[0x4028u] = 0xa000u;
    runtime.words[0x402cu] = 0xa00cu;
    runtime.words[0x4038u] = 3u;
    runtime.words[0xa000u] = 0x5100u;
    runtime.words[0xa004u] = 0x3000u;
    runtime.words[0xa008u] = 0x5200u;
    Check(pal4::XmlDocumentRecord::Cleanup(
              0x3000u, globals, runtime) == 0u &&
              runtime.words[0x3010u] == 0u &&
              runtime.words[0x402cu] == 0xa008u &&
              runtime.words[0x4038u] == 2u &&
              runtime.words[0xa000u] == 0x5100u &&
              runtime.words[0xa004u] == 0x5200u &&
              runtime.text_reallocations ==
                  std::vector<std::uint32_t>{0x3000u} &&
              runtime.text_detachments ==
                  std::vector<std::uint32_t>{0x3000u},
          "cleanup removes child slot, compacts vector and clears parent");
}

void TestCleanupReleasesAttributeTree() {
    Runtime runtime;
    pal4::XmlDocumentTreeGlobals globals{0x9000u, 1u};
    runtime.words[0x3034u] = 2u;
    runtime.words[0x3018u] = 0x8000u;
    runtime.words[0x8000u] = 0x8100u;
    runtime.successors[0x8100u] = 0x8200u;
    runtime.successors[0x8200u] = 0x8000u;
    Check(pal4::XmlDocumentRecord::Cleanup(
              0x3000u, globals, runtime) == 0u &&
              runtime.freed_attributes ==
                  (std::vector<std::uint32_t>{0x811cu, 0x821cu}) &&
              runtime.next_requests ==
                  (std::vector<std::uint32_t>{0x8100u, 0x8200u}) &&
              runtime.first_element_requests ==
                  std::vector<std::uint32_t>{0x3014u} &&
              runtime.cleared_trees ==
                  (std::vector<std::tuple<std::uint32_t,
                                          std::uint32_t,
                                          std::uint32_t>>{
                      {0x3014u, 0x8100u, 0x8000u}}) &&
              runtime.words[0x3034u] == 0u &&
              runtime.words[0x8000u] == 0x8000u,
          "cleanup frees attribute values before clearing their tree");
}

void TestCleanupRecursivelyDestroysChildren() {
    Runtime runtime;
    pal4::XmlDocumentTreeGlobals globals{0x9000u, 2u};
    runtime.words[0x3028u] = 0xa000u;
    runtime.words[0x302cu] = 0xa004u;
    runtime.words[0x3038u] = 1u;
    runtime.words[0xa000u] = 0x4000u;
    runtime.words[0x4010u] = 0x3000u;
    runtime.words[0x4018u] = 0xb000u;
    runtime.words[0xb000u] = 0xb000u;
    Check(pal4::XmlDocumentRecord::Cleanup(
              0x3000u, globals, runtime) == 0u &&
              runtime.words[0x302cu] == 0xa000u &&
              runtime.words[0x3038u] == 0u &&
              runtime.words[0x4010u] == 0u &&
              runtime.cleaned_nodes ==
                  std::vector<std::uint32_t>{0x4000u} &&
              runtime.frees ==
                  (std::vector<std::uint32_t>{0u, 0xb000u, 0x4000u}) &&
              globals.construction_count == 1u &&
              globals.nil_node == 0x9000u &&
              runtime.text_reallocations ==
                  (std::vector<std::uint32_t>{0x4000u, 0x3000u}),
          "cleanup detaches and destroys each child before clearing vector count");
}

void TestCleanupNormalizesNameBuffer() {
    Runtime runtime;
    pal4::XmlDocumentTreeGlobals globals;
    runtime.words[0x3004u] = 0xa001u;
    runtime.words[0x3008u] = 3u;
    runtime.bytes[0xa000u] = 2u;
    Check(pal4::XmlDocumentRecord::Cleanup(
              0x3000u, globals, runtime) == 7u &&
              runtime.text_allocations ==
                  (std::vector<std::pair<std::uint32_t,
                                         std::uint32_t>>{
                      {0x3000u, 3u}, {0x3000u, 0u}}) &&
              runtime.bytes[0xa000u] == 0xffu &&
              runtime.text_sizes ==
                  (std::vector<std::pair<std::uint32_t,
                                         std::uint32_t>>{
                      {0x3000u, 0u}}) &&
              runtime.words[0x3008u] == 0u,
          "cleanup detaches shared text and applies final size update");

    Runtime invalid_runtime;
    invalid_runtime.words[0x3004u] = 0xa001u;
    invalid_runtime.words[0x3008u] = 3u;
    invalid_runtime.bytes[0xa000u] = 0xffu;
    invalid_runtime.reallocated_text_pointer = 0x9ff0u;
    bool caught = false;
    try {
        (void)pal4::XmlDocumentRecord::Cleanup(
            0x3000u, globals, invalid_runtime);
    } catch (const std::out_of_range&) {
        caught = true;
    }
    Check(caught && invalid_runtime.text_detachments.empty(),
          "out-of-range text offset stops before detach");

    Runtime copy_runtime;
    copy_runtime.words[0x3004u] = 0xa001u;
    copy_runtime.words[0x3008u] = 3u;
    copy_runtime.bytes[0xa000u] = 0xffu;
    copy_runtime.bytes[0xa004u] = 'x';
    copy_runtime.bytes[0xa005u] = 'y';
    copy_runtime.detached_length_override = 5u;
    copy_runtime.text_allocation_result = 0u;
    Check(pal4::XmlDocumentRecord::Cleanup(
              0x3000u, globals, copy_runtime) == 0u &&
              copy_runtime.bytes[0xa001u] == 'x' &&
              copy_runtime.bytes[0xa002u] == 'y' &&
              copy_runtime.text_allocations ==
                  (std::vector<std::pair<std::uint32_t,
                                         std::uint32_t>>{
                      {0x3000u, 2u}}) &&
              copy_runtime.text_sizes.empty(),
          "text compaction copies trailing bytes before failed size allocation");
}

void TestGetAttributeByIndex() {
    struct Case {
        std::uint32_t index;
        std::uint32_t count;
        std::uint32_t expected;
        std::vector<std::uint32_t> traversal;
    };
    for (const auto& test : {
             Case{0u, 3u, 0x811cu, {}},
             Case{1u, 3u, 0x821cu, {0x8100u}},
             Case{2u, 3u, 0x831cu, {0x8100u, 0x8200u}},
             Case{3u, 3u, 0u, {}},
             Case{0xffffffffu, 3u, 0u, {}},
             Case{0u, 0u, 0u, {}},
             Case{0u, 0xffffffffu, 0u, {}}}) {
        Runtime runtime;
        runtime.words[0x5000u] = test.index;
        runtime.words[0x3034u] = test.count;
        runtime.words[0x3018u] = 0x8000u;
        runtime.words[0x8000u] = 0x8100u;
        runtime.successors[0x8100u] = 0x8200u;
        runtime.successors[0x8200u] = 0x8300u;
        Check(pal4::XmlDocumentRecord::GetAttributeByIndex(
                  0x3000u, 0x5000u, runtime) == test.expected &&
                  runtime.next_requests == test.traversal,
              "attribute index checks signed bounds and advances in tree order");
    }
}

void TestGetChildByIndex() {
    struct Case {
        std::uint32_t index;
        std::uint32_t count;
        std::uint32_t expected;
        std::vector<std::uint32_t> reads;
    };
    for (const auto& test : {
             Case{0u, 3u, 0x9100u, {0x5000u, 0x3038u, 0x3028u, 0xa000u}},
             Case{1u, 3u, 0u, {0x5000u, 0x3038u, 0x3028u, 0xa004u}},
             Case{2u, 3u, 0x9300u, {0x5000u, 0x3038u, 0x3028u, 0xa008u}},
             Case{3u, 3u, 0u, {0x5000u, 0x3038u}},
             Case{0u, 0u, 0u, {0x5000u, 0x3038u}},
             Case{0xffffffffu, 3u, 0u, {0x5000u}},
             Case{0x80000000u, 3u, 0u, {0x5000u}},
             Case{0u, 0xffffffffu, 0u, {0x5000u, 0x3038u}}}) {
        Runtime runtime;
        runtime.words[0x5000u] = test.index;
        runtime.words[0x3038u] = test.count;
        runtime.words[0x3028u] = 0xa000u;
        runtime.words[0xa000u] = 0x9100u;
        runtime.words[0xa004u] = 0u;
        runtime.words[0xa008u] = 0x9300u;
        Check(pal4::XmlDocumentRecord::GetChildByIndex(
                  0x3000u, 0x5000u, runtime) == test.expected &&
                  runtime.word_reads == test.reads,
              "child index checks signed bounds before reading pointer slot");
    }
}

void TestFindChildByName() {
    Runtime runtime;
    runtime.words[0x3038u] = 4u;
    runtime.words[0x3028u] = 0xa000u;
    for (std::uint32_t i = 0u; i < 4u; ++i) {
        const auto child = 0x8100u + i * 0x100u;
        const auto text = 0x9100u + i * 0x100u;
        runtime.words[0xa000u + i * 4u] = child;
        runtime.words[child + 4u] = text;
    }
    runtime.words[0x8108u] = 2u;
    runtime.bytes[0x9100u] = 'a';
    runtime.bytes[0x9101u] = 'b';
    runtime.words[0x8208u] = 3u;
    runtime.bytes[0x9200u] = 'a';
    runtime.bytes[0x9201u] = 'b';
    runtime.bytes[0x9202u] = 'c';
    runtime.words[0x8308u] = 2u;
    runtime.bytes[0x9300u] = 'a';
    runtime.bytes[0x9301u] = 'b';
    runtime.words[0x8408u] = 0u;
    const auto query = 0xb000u;
    runtime.bytes[query] = 'a';
    runtime.bytes[query + 1u] = 'b';
    runtime.bytes[query + 2u] = 0u;
    Check(pal4::XmlDocumentRecord::FindChildByName(
              0x3000u, query, runtime) == 0x8100u,
          "name lookup returns first exact match among duplicate children");
    runtime.bytes[query + 2u] = 'c';
    runtime.bytes[query + 3u] = 0u;
    Check(pal4::XmlDocumentRecord::FindChildByName(
              0x3000u, query, runtime) == 0x8200u,
          "longer name skips a matching prefix");
    runtime.bytes[query + 2u] = 'd';
    Check(pal4::XmlDocumentRecord::FindChildByName(
              0x3000u, query, runtime) == 0u,
          "missing name returns null");
    runtime.bytes[query] = 'A';
    runtime.bytes[query + 1u] = 'b';
    runtime.bytes[query + 2u] = 0u;
    Check(pal4::XmlDocumentRecord::FindChildByName(
              0x3000u, query, runtime) == 0u,
          "name matching is case sensitive");
    runtime.bytes[query] = 0u;
    Check(pal4::XmlDocumentRecord::FindChildByName(
              0x3000u, query, runtime) == 0x8400u,
          "empty name matches a zero-length child name");
    Runtime empty;
    Check(pal4::XmlDocumentRecord::FindChildByName(
              0x3000u, 0u, empty) == 0u &&
              empty.word_reads == std::vector<std::uint32_t>{0x3038u},
          "empty vector returns before reading name or child storage");
}

void TestFindNextChildByName() {
    Runtime runtime;
    runtime.words[0x3038u] = 4u;
    runtime.words[0x3028u] = 0xa000u;
    runtime.words[0xa000u] = 0x8100u;
    runtime.words[0xa004u] = 0x8200u;
    runtime.words[0xa008u] = 0x8300u;
    runtime.words[0xa00cu] = 0x8400u;
    runtime.words[0x8108u] = 2u;
    runtime.words[0x8104u] = 0x9100u;
    runtime.bytes[0x9100u] = 'a';
    runtime.bytes[0x9101u] = 'b';
    runtime.words[0x8208u] = 3u;
    runtime.words[0x8204u] = 0x9200u;
    runtime.bytes[0x9200u] = 'a';
    runtime.bytes[0x9201u] = 'b';
    runtime.bytes[0x9202u] = 'c';
    runtime.words[0x8308u] = 2u;
    runtime.words[0x8304u] = 0x9300u;
    runtime.bytes[0x9300u] = 'a';
    runtime.bytes[0x9301u] = 'b';
    runtime.words[0x8408u] = 2u;
    runtime.words[0x8404u] = 0x9400u;
    runtime.bytes[0x9400u] = 'a';
    runtime.bytes[0x9401u] = 'b';
    Check(pal4::XmlDocumentRecord::FindNextChildByName(
              0x3000u, 0x8100u, runtime) == 0x8300u &&
              pal4::XmlDocumentRecord::FindNextChildByName(
                  0x3000u, 0x8300u, runtime) == 0x8400u &&
              pal4::XmlDocumentRecord::FindNextChildByName(
                  0x3000u, 0x8400u, runtime) == 0u,
          "next-name lookup skips unequal siblings and preserves order");
    Check(pal4::XmlDocumentRecord::FindNextChildByName(
              0x3000u, 0x8500u, runtime) == 0u,
          "unknown previous child does not trigger name comparison");
    runtime.words[0x8304u] = 0u;
    runtime.words[0x8308u] = 0u;
    runtime.words[0x8404u] = 0u;
    runtime.words[0x8408u] = 0u;
    Check(pal4::XmlDocumentRecord::FindNextChildByName(
              0x3000u, 0x8300u, runtime) == 0x8400u,
          "null name pointers use the empty-name fallback");
    Runtime empty;
    Check(pal4::XmlDocumentRecord::FindNextChildByName(
              0x3000u, 0x8100u, empty) == 0u &&
              empty.word_reads == std::vector<std::uint32_t>{0x3038u},
          "empty vector returns before accessing previous child");
}

void TestAddChildNullAndInPlace() {
    Runtime runtime;
    Check(pal4::XmlDocumentRecord::AddChild(
              0x3000u, 0u, runtime) == 0u && runtime.word_reads.empty(),
          "null child returns before touching parent or allocator");
    runtime.words[0x3028u] = 0xa000u;
    runtime.words[0x302cu] = 0xa004u;
    runtime.words[0x3030u] = 0xa00cu;
    runtime.words[0x3038u] = 1u;
    runtime.words[0xa000u] = 0x8100u;
    Check(pal4::XmlDocumentRecord::AddChild(
              0x3000u, 0x8200u, runtime) == 0x8200u &&
              runtime.words[0xa004u] == 0x8200u &&
              runtime.words[0x302cu] == 0xa008u &&
              runtime.words[0x3038u] == 2u &&
              runtime.words[0x8210u] == 0x3000u &&
              runtime.allocations.empty(),
          "spare vector slot appends child and sets parent");
}

void TestAddChildGrowsVector() {
    for (const std::uint32_t count : {0u, 1u, 2u}) {
        Runtime runtime;
        const auto first = count == 0u ? 0u : 0xa000u;
        runtime.words[0x3028u] = first;
        runtime.words[0x302cu] = first + count * 4u;
        runtime.words[0x3030u] = first + count * 4u;
        runtime.words[0x3038u] = count;
        for (std::uint32_t index = 0u; index < count; ++index)
            runtime.words[first + index * 4u] = 0x8100u + index * 0x100u;
        Check(pal4::XmlDocumentRecord::AddChild(
                  0x3000u, 0x9000u, runtime) == 0x9000u &&
                  runtime.allocations ==
                      std::vector<std::uint32_t>{0x7000u} &&
                  runtime.frees == std::vector<std::uint32_t>{first} &&
                  runtime.words[0x3028u] == 0x7000u &&
                  runtime.words[0x302cu] == 0x7000u + (count + 1u) * 4u &&
                  runtime.words[0x3030u] ==
                      0x7000u + (count + (count > 1u ? count : 1u)) * 4u &&
                  runtime.words[0x3038u] == count + 1u &&
                  runtime.words[0x9010u] == 0x3000u &&
                  runtime.words[0x7000u + count * 4u] == 0x9000u,
              "full vector grows and preserves existing child order");
        for (std::uint32_t index = 0u; index < count; ++index)
            Check(runtime.words[0x7000u + index * 4u] ==
                      0x8100u + index * 0x100u,
                  "vector reallocation copies earlier children");
    }
}

void TestAddChildMovesBetweenParents() {
    Runtime runtime;
    runtime.words[0x8210u] = 0x4000u;
    runtime.words[0x4028u] = 0xb000u;
    runtime.words[0x402cu] = 0xb00cu;
    runtime.words[0x4038u] = 3u;
    runtime.words[0xb000u] = 0x8100u;
    runtime.words[0xb004u] = 0x8200u;
    runtime.words[0xb008u] = 0x8300u;
    Check(pal4::XmlDocumentRecord::AddChild(
              0x3000u, 0x8200u, runtime) == 0x8200u &&
              runtime.words[0xb000u] == 0x8100u &&
              runtime.words[0xb004u] == 0x8300u &&
              runtime.words[0x402cu] == 0xb008u &&
              runtime.words[0x4038u] == 2u &&
              runtime.words[0x7000u] == 0x8200u &&
              runtime.words[0x3038u] == 1u &&
              runtime.words[0x8210u] == 0x3000u,
          "reparenting compacts old sibling vector before append");
}

void TestAddChildReordersWithinParent() {
    Runtime runtime;
    runtime.words[0x8210u] = 0x3000u;
    runtime.words[0x3028u] = 0xa000u;
    runtime.words[0x302cu] = 0xa00cu;
    runtime.words[0x3030u] = 0xa00cu;
    runtime.words[0x3038u] = 3u;
    runtime.words[0xa000u] = 0x8100u;
    runtime.words[0xa004u] = 0x8200u;
    runtime.words[0xa008u] = 0x8300u;
    Check(pal4::XmlDocumentRecord::AddChild(
              0x3000u, 0x8200u, runtime) == 0x8200u &&
              runtime.words[0xa000u] == 0x8100u &&
              runtime.words[0xa004u] == 0x8300u &&
              runtime.words[0xa008u] == 0x8200u &&
              runtime.words[0x302cu] == 0xa00cu &&
              runtime.words[0x3038u] == 3u &&
              runtime.allocations.empty(),
          "same-parent append moves child to end without growing vector");
}

void TestAddChildAllocationFailureAfterDetach() {
    Runtime runtime;
    runtime.fail_attempt = 1u;
    runtime.words[0x8210u] = 0x4000u;
    runtime.words[0x4028u] = 0xb000u;
    runtime.words[0x402cu] = 0xb004u;
    runtime.words[0x4038u] = 1u;
    runtime.words[0xb000u] = 0x8200u;
    bool caught = false;
    try {
        (void)pal4::XmlDocumentRecord::AddChild(
            0x3000u, 0x8200u, runtime);
    } catch (const std::bad_alloc&) {
        caught = true;
    }
    Check(caught && runtime.words[0x402cu] == 0xb000u &&
              runtime.words[0x4038u] == 0u &&
              runtime.words[0x8210u] == 0x4000u &&
              runtime.words[0x3038u] == 0u &&
              runtime.frees.empty(),
          "allocation failure follows native detach-before-append order");
}

void TestCreateNodeInitializesAndAttaches() {
    Runtime runtime;
    pal4::XmlDocumentTreeGlobals globals;
    runtime.bytes[0xb000u] = 'x';
    runtime.bytes[0xb001u] = 0u;
    Check(pal4::XmlDocumentRecord::CreateNode(
              0x3000u, 0xb000u, globals, runtime) == 0x7000u &&
              runtime.allocations ==
                  (std::vector<std::uint32_t>{
                      0x7000u, 0x8000u, 0x103000u}) &&
              globals.nil_node == 0x8000u &&
              globals.construction_count == 1u &&
              runtime.words[0x7018u] == 0x103000u &&
              runtime.words[0x7004u] == 0x207000u &&
              runtime.words[0x7008u] == 1u &&
              runtime.bytes[0x207000u] == 'x' &&
              runtime.bytes[0x207001u] == 0u &&
              runtime.words[0x7010u] == 0x3000u &&
              runtime.children ==
                  (std::vector<std::pair<std::uint32_t,
                                         std::uint32_t>>{
                      {0x3000u, 0x7000u}}),
          "create-node allocates, initializes, names and attaches child");

    Runtime unnamed;
    pal4::XmlDocumentTreeGlobals unnamed_globals;
    Check(pal4::XmlDocumentRecord::CreateNode(
              0u, 0u, unnamed_globals, unnamed) == 0x7000u &&
              unnamed.words[0x7010u] == 0u &&
              unnamed.children.empty() &&
              unnamed.cleaned_nodes ==
                  std::vector<std::uint32_t>{0x7000u},
          "null name and parent still produce initialized node");
}

void TestCreateNodeAllocationFailures() {
    for (const auto failed_attempt : {1u, 2u, 3u}) {
        Runtime runtime;
        pal4::XmlDocumentTreeGlobals globals;
        runtime.fail_attempt = failed_attempt;
        bool caught = false;
        try {
            (void)pal4::XmlDocumentRecord::CreateNode(
                0x3000u, 0u, globals, runtime);
        } catch (const std::bad_alloc&) {
            caught = true;
        }
        Check(caught && runtime.allocation_attempts == failed_attempt &&
                  runtime.children.empty() &&
                  runtime.cleaned_nodes.empty() &&
                   runtime.frees ==
                       (failed_attempt == 1u
                            ? std::vector<std::uint32_t>{}
                            : failed_attempt == 2u
                                ? std::vector<std::uint32_t>{0x7000u}
                                : std::vector<std::uint32_t>{0x8000u,
                                                              0x7000u}) &&
                   globals.construction_count == 0u &&
                   globals.nil_node == 0u,
              "create-node releases outer storage on allocation failure");
    }
}

void TestRemoveChildMembershipAndDestruction() {
    Runtime runtime;
    pal4::XmlDocumentTreeGlobals globals{0x9000u, 2u};
    runtime.words[0x3028u] = 0xa000u;
    runtime.words[0x302cu] = 0xa008u;
    runtime.words[0x3038u] = 2u;
    runtime.words[0xa000u] = 0x4000u;
    runtime.words[0xa004u] = 0x4100u;
    runtime.words[0x4010u] = 0x3000u;
    runtime.words[0x4018u] = 0x8000u;
    runtime.words[0x8000u] = 0x8000u;
    Check(pal4::XmlDocumentRecord::RemoveChild(
              0x3000u, 0u, globals, runtime) == 0u &&
              runtime.word_reads.empty(),
          "null remove request returns before reading parent vector");
    Check(pal4::XmlDocumentRecord::RemoveChild(
              0x3000u, 0x4200u, globals, runtime) == 0u &&
              runtime.cleaned_nodes.empty() && runtime.frees.empty() &&
              runtime.words[0x3038u] == 2u,
          "non-member child is not cleaned or destroyed");
    Check(pal4::XmlDocumentRecord::RemoveChild(
              0x3000u, 0x4000u, globals, runtime) == 1u &&
              runtime.words[0xa000u] == 0x4100u &&
              runtime.words[0x302cu] == 0xa004u &&
              runtime.words[0x3038u] == 1u &&
              runtime.words[0x4010u] == 0u &&
              runtime.cleaned_nodes ==
                  std::vector<std::uint32_t>{0x4000u} &&
              runtime.frees ==
                  (std::vector<std::uint32_t>{0u, 0x8000u, 0x4000u}) &&
              globals.construction_count == 1u,
          "member removal cleans, destroys and frees after vector detachment");
}

void TestCleanupDocumentValue() {
    for (const auto header : {0u, 0xffu, 2u}) {
        Runtime runtime;
        runtime.words[0x3004u] = 0xa001u;
        runtime.words[0x3008u] = 3u;
        runtime.words[0x300cu] = 8u;
        runtime.bytes[0xa000u] = static_cast<std::uint8_t>(header);
        const auto result =
            pal4::XmlDocumentRecord::CleanupDocumentValue(0x3000u, runtime);
        Check(result == (header == 2u ? 0xa000u : 0xfaceu) &&
                  runtime.destroyed_embedded_attributes ==
                      std::vector<std::uint32_t>{0x3010u} &&
                  runtime.frees ==
                      (header == 2u
                           ? std::vector<std::uint32_t>{}
                           : std::vector<std::uint32_t>{0xa000u}) &&
                  runtime.bytes[0xa000u] ==
                      static_cast<std::uint8_t>(header == 2u ? 1u : header) &&
                  runtime.words[0x3004u] == 0u &&
                  runtime.words[0x3008u] == 0u &&
                  runtime.words[0x300cu] == 0u,
              "document value cleanup releases unique or decrements shared name");
    }
    Runtime absent;
    Check(pal4::XmlDocumentRecord::CleanupDocumentValue(
              0x3000u, absent) == 0u &&
              absent.destroyed_embedded_attributes ==
                  std::vector<std::uint32_t>{0x3010u} &&
              absent.frees.empty(),
          "absent name still destroys embedded attribute");
}

void TestRemoveAttributeNodeTreeCases() {
    constexpr std::uint32_t nil = 0x9000u;
    constexpr std::uint32_t head = 0x8000u;
    constexpr std::uint32_t a = 0xa000u;
    constexpr std::uint32_t b = 0xb000u;
    constexpr std::uint32_t c = 0xc000u;
    constexpr std::uint32_t d = 0xd000u;
    constexpr std::uint32_t e = 0xe000u;
    constexpr std::uint32_t f = 0xf000u;
    constexpr std::uint32_t g = 0x10000u;
    struct Node {
        std::uint32_t address, left, parent, right, color;
    };
    struct Case {
        const char* label;
        std::vector<Node> nodes;
        std::uint32_t root, first, last, removed, next;
        std::vector<std::uint32_t> remaining;
        std::uint32_t new_root, new_first, new_last;
    };
    for (const auto& test : {
             Case{"singleton", {{a, nil, head, nil, 1u}},
                  a, a, a, a, head, {}, nil, head, head},
             Case{"red leaf", {{b, a, head, c, 1u},
                                {a, nil, b, nil, 0u},
                                {c, nil, b, nil, 0u}},
                  b, a, c, a, b, {b, c}, b, b, c},
             Case{"red sibling fixup", {{b, a, head, d, 1u},
                                        {a, nil, b, nil, 1u},
                                        {d, c, b, e, 0u},
                                        {c, nil, d, nil, 1u},
                                        {e, nil, d, nil, 1u}},
                  b, a, e, a, b, {b, c, d, e}, d, b, e},
             Case{"near red fixup", {{b, a, head, d, 1u},
                                     {a, nil, b, nil, 1u},
                                     {d, c, b, nil, 1u},
                                     {c, nil, d, nil, 0u}},
                  b, a, d, a, b, {b, c, d}, c, b, d},
             Case{"far red fixup", {{b, a, head, d, 1u},
                                    {a, nil, b, nil, 1u},
                                    {d, nil, b, e, 1u},
                                    {e, nil, d, nil, 0u}},
                  b, a, e, a, b, {b, d, e}, d, b, e},
             Case{"right side fixup", {{d, b, head, e, 1u},
                                        {b, a, d, c, 1u},
                                        {a, nil, b, nil, 0u},
                                        {c, nil, b, nil, 0u},
                                        {e, nil, d, nil, 1u}},
                  d, a, e, e, head, {a, b, c, d}, b, a, d},
             Case{"right near red fixup", {{d, b, head, e, 1u},
                                            {b, nil, d, c, 1u},
                                            {c, nil, b, nil, 0u},
                                            {e, nil, d, nil, 1u}},
                  d, b, e, e, head, {b, c, d}, c, b, d},
             Case{"direct successor", {{b, a, head, c, 1u},
                                       {a, nil, b, nil, 1u},
                                       {c, nil, b, nil, 1u}},
                  b, a, c, b, c, {a, c}, c, a, c},
             Case{"deep successor", {{d, b, head, f, 1u},
                                     {b, a, d, c, 1u},
                                     {a, nil, b, nil, 1u},
                                     {c, nil, b, nil, 1u},
                                     {f, e, d, g, 1u},
                                     {e, nil, f, nil, 1u},
                                     {g, nil, f, nil, 1u}},
                  d, a, g, d, e, {a, b, c, e, f, g}, e, a, g}}) {
        Runtime runtime;
        pal4::XmlDocumentTreeGlobals globals{nil, 1u};
        runtime.words[0x3018u] = head;
        runtime.words[0x3020u] =
            static_cast<std::uint32_t>(test.nodes.size());
        runtime.words[head] = test.first;
        runtime.words[head + 4u] = test.root;
        runtime.words[head + 8u] = test.last;
        runtime.words[nil] = nil;
        runtime.words[nil + 8u] = nil;
        runtime.words[nil + 56u] = 1u;
        for (const auto& node : test.nodes) {
            runtime.words[node.address] = node.left;
            runtime.words[node.address + 4u] = node.parent;
            runtime.words[node.address + 8u] = node.right;
            runtime.words[node.address + 56u] = node.color;
        }
        Check(pal4::XmlDocumentRecord::RemoveAttributeNode(
                  0x3014u, 0x5000u, test.removed, globals, runtime) ==
                  0x5000u &&
                  runtime.words[0x5000u] == test.next &&
                  runtime.words[0x3020u] == test.nodes.size() - 1u &&
                  runtime.words[head] == test.new_first &&
                  runtime.words[head + 4u] == test.new_root &&
                  runtime.words[head + 8u] == test.new_last &&
                  runtime.next_requests.empty() &&
                  runtime.destroyed_embedded_attributes ==
                      std::vector<std::uint32_t>{test.removed + 28u} &&
                  runtime.frees ==
                      std::vector<std::uint32_t>{test.removed},
              test.label);

        std::vector<std::uint32_t> inorder;
        std::set<std::uint32_t> visited;
        std::function<int(std::uint32_t, std::uint32_t)> validate =
            [&](const std::uint32_t node, const std::uint32_t parent) -> int {
            if (node == nil) return 1;
            Check(visited.insert(node).second &&
                      runtime.words[node + 4u] == parent &&
                      runtime.words[node + 56u] <= 1u,
                  "tree has unique nodes, correct parents and valid colors");
            const auto left = runtime.words[node];
            const auto right = runtime.words[node + 8u];
            if (runtime.words[node + 56u] == 0u)
                Check(runtime.words[left + 56u] == 1u &&
                          runtime.words[right + 56u] == 1u,
                      "red tree node has black children");
            const auto left_depth = validate(left, node);
            inorder.push_back(node);
            const auto right_depth = validate(right, node);
            Check(left_depth == right_depth,
                  "tree paths have equal black height");
            return left_depth +
                   static_cast<int>(runtime.words[node + 56u]);
        };
        if (test.new_root != nil)
            Check(runtime.words[test.new_root + 56u] == 1u,
                  "remaining tree root is black");
        (void)validate(test.new_root, head);
        Check(inorder == test.remaining,
              "tree deletion preserves sorted node order");
    }
}

void TestRemoveAttributeNodeReleasesName() {
    Runtime runtime;
    pal4::XmlDocumentTreeGlobals globals{0x9000u, 1u};
    runtime.words[0x3018u] = 0x8000u;
    runtime.words[0x3020u] = 1u;
    runtime.words[0x8000u] = 0xa000u;
    runtime.words[0x8004u] = 0xa000u;
    runtime.words[0x8008u] = 0xa000u;
    runtime.words[0xa000u] = 0x9000u;
    runtime.words[0xa004u] = 0x8000u;
    runtime.words[0xa008u] = 0x9000u;
    runtime.words[0xa038u] = 1u;
    runtime.words[0xa010u] = 0xb001u;
    runtime.words[0xa014u] = 4u;
    runtime.words[0xa018u] = 8u;
    runtime.bytes[0xb000u] = 0u;
    runtime.words[0x9038u] = 1u;
    Check(pal4::XmlDocumentRecord::RemoveAttributeNode(
              0x3014u, 0x5000u, 0xa000u, globals, runtime) == 0x5000u &&
              runtime.destroyed_embedded_attributes ==
                  std::vector<std::uint32_t>{0xa01cu} &&
              runtime.frees ==
                  (std::vector<std::uint32_t>{0xb000u, 0xa000u}) &&
              runtime.words[0xa010u] == 0u &&
              runtime.words[0xa014u] == 0u &&
              runtime.words[0xa018u] == 0u,
          "tree removal cleans attribute and name before freeing tree node");
}

void TestClearAttributeRange() {
    Runtime full;
    pal4::XmlDocumentTreeGlobals globals{0x9000u, 1u};
    full.words[0x3018u] = 0x8000u;
    full.words[0x3020u] = 3u;
    full.words[0x8000u] = 0xa000u;
    full.words[0x8004u] = 0xb000u;
    full.words[0x8008u] = 0xc000u;
    full.words[0xb000u] = 0xa000u;
    full.words[0xb008u] = 0xc000u;
    full.words[0xa000u] = 0x9000u;
    full.words[0xa008u] = 0x9000u;
    full.words[0xc000u] = 0x9000u;
    full.words[0xc008u] = 0x9000u;
    Check(pal4::XmlDocumentRecord::ClearAttributeRange(
              0x3014u, 0x5000u, 0xa000u, 0x8000u,
              globals, full) == 0x5000u &&
              full.deleted_nodes.empty() &&
              full.destroyed_embedded_attributes ==
                  (std::vector<std::uint32_t>{
                      0xc01cu, 0xb01cu, 0xa01cu}) &&
              full.frees ==
                  (std::vector<std::uint32_t>{
                      0xc000u, 0xb000u, 0xa000u}) &&
              full.words[0x8000u] == 0x8000u &&
              full.words[0x8004u] == 0x9000u &&
              full.words[0x8008u] == 0x8000u &&
              full.words[0x3020u] == 0u &&
              full.words[0x5000u] == 0x8000u,
          "whole-tree clear releases left spine and resets head");

    Runtime partial;
    partial.words[0x3018u] = 0x8000u;
    partial.words[0x3020u] = 3u;
    partial.words[0x8000u] = 0xa000u;
    partial.words[0x8004u] = 0xb000u;
    partial.words[0x8008u] = 0xc000u;
    partial.words[0xa000u] = 0x9000u;
    partial.words[0xa004u] = 0xb000u;
    partial.words[0xa008u] = 0x9000u;
    partial.words[0xa038u] = 0u;
    partial.words[0xb000u] = 0xa000u;
    partial.words[0xb004u] = 0x8000u;
    partial.words[0xb008u] = 0xc000u;
    partial.words[0xb038u] = 1u;
    partial.words[0xc000u] = 0x9000u;
    partial.words[0xc004u] = 0xb000u;
    partial.words[0xc008u] = 0x9000u;
    partial.words[0xc038u] = 0u;
    partial.words[0x9000u] = 0x9000u;
    partial.words[0x9008u] = 0x9000u;
    partial.words[0x9038u] = 1u;
    Check(pal4::XmlDocumentRecord::ClearAttributeRange(
              0x3014u, 0x5000u, 0xa000u, 0xc000u,
              globals, partial) == 0x5000u &&
              partial.next_requests.empty() &&
              partial.frees ==
                  (std::vector<std::uint32_t>{0xa000u, 0xb000u}) &&
              partial.words[0x8000u] == 0xc000u &&
              partial.words[0x8004u] == 0xc000u &&
              partial.words[0x8008u] == 0xc000u &&
              partial.words[0x3020u] == 1u &&
              partial.words[0x5000u] == 0xc000u,
          "partial clear deletes half-open range and returns endpoint");

    Runtime empty;
    empty.words[0x5000u] = 0xdeadbeefu;
    Check(pal4::XmlDocumentRecord::ClearAttributeRange(
              0x3014u, 0x5000u, 0x8000u, 0x8000u,
              globals, empty) == 0x5000u &&
              empty.words[0x5000u] == 0x8000u &&
              empty.word_reads ==
                  std::vector<std::uint32_t>{0x3020u} &&
              empty.frees.empty(),
          "empty range returns its endpoint without reading tree head");
}

void TestDeleteAttributeSubtree() {
    Runtime runtime;
    pal4::XmlDocumentTreeGlobals globals{0x9000u, 1u};
    Check(pal4::XmlDocumentRecord::DeleteAttributeSubtree(
              0x9000u, globals, runtime) == 0x9000u &&
              runtime.word_reads.empty() && runtime.frees.empty(),
          "nil subtree returns without reading nodes");
    runtime.words[0xb000u] = 0xa000u;
    runtime.words[0xb008u] = 0xd000u;
    runtime.words[0xa000u] = 0x9000u;
    runtime.words[0xa008u] = 0x9000u;
    runtime.words[0xd000u] = 0xc000u;
    runtime.words[0xd008u] = 0xe000u;
    runtime.words[0xc000u] = 0x9000u;
    runtime.words[0xc008u] = 0x9000u;
    runtime.words[0xe000u] = 0x9000u;
    runtime.words[0xe008u] = 0x9000u;
    runtime.words[0xe010u] = 0x20001u;
    runtime.bytes[0x20000u] = 0u;
    Check(pal4::XmlDocumentRecord::DeleteAttributeSubtree(
              0xb000u, globals, runtime) == 0x9000u &&
              runtime.destroyed_embedded_attributes ==
                  (std::vector<std::uint32_t>{
                      0xe01cu, 0xd01cu, 0xc01cu, 0xb01cu, 0xa01cu}) &&
              runtime.frees ==
                  (std::vector<std::uint32_t>{
                      0x20000u, 0xe000u, 0xd000u,
                      0xc000u, 0xb000u, 0xa000u}) &&
              runtime.words[0xe010u] == 0u,
          "subtree deletion visits each right branch before left spine");
}

void TestCreateAttributeTree() {
    Runtime first;
    pal4::XmlDocumentTreeGlobals globals;
    first.bytes[0x3008u] = 0x5au;
    Check(pal4::XmlDocumentRecord::CreateAttributeTree(
              0x3000u, globals, first) == 0x8000u &&
              globals.nil_node == 0x7000u &&
              globals.construction_count == 1u &&
              first.words[0x7000u] == 0u &&
              first.words[0x7004u] == 0u &&
              first.words[0x7008u] == 0u &&
              first.words[0x7038u] == 1u &&
              first.words[0x3004u] == 0x8000u &&
              first.words[0x300cu] == 0u &&
              first.words[0x8000u] == 0x8000u &&
              first.words[0x8004u] == 0x7000u &&
              first.words[0x8008u] == 0x8000u &&
              first.bytes[0x3008u] == 0x5au,
          "new attribute tree installs shared nil and self-linked head");

    Runtime subsequent;
    subsequent.bytes[0x3008u] = 0x6bu;
    pal4::XmlDocumentTreeGlobals shared{0x9000u, 4u};
    Check(pal4::XmlDocumentRecord::CreateAttributeTree(
              0x3000u, shared, subsequent) == 0x8000u &&
              shared.nil_node == 0x9000u &&
              shared.construction_count == 5u &&
              subsequent.frees ==
                  std::vector<std::uint32_t>{0x7000u} &&
              subsequent.words[0x8004u] == 0x9000u &&
              subsequent.bytes[0x3008u] == 0x6bu,
          "later attribute tree frees proposed nil and retains mode byte");

    for (const auto attempt : {1u, 2u}) {
        Runtime failed;
        failed.fail_attempt = attempt;
        pal4::XmlDocumentTreeGlobals failure_globals;
        bool caught = false;
        try {
            (void)pal4::XmlDocumentRecord::CreateAttributeTree(
                0x3000u, failure_globals, failed);
        } catch (const std::bad_alloc&) {
            caught = true;
        }
        Check(caught && failed.allocation_attempts == attempt &&
                  failure_globals.construction_count == 0u &&
                  failure_globals.nil_node == 0u &&
                  failed.frees == (attempt == 2u
                      ? std::vector<std::uint32_t>{0x7000u}
                      : std::vector<std::uint32_t>{}),
              "tree creation rolls back both allocation failure states");
    }

    Runtime failed_shared;
    failed_shared.fail_attempt = 2u;
    pal4::XmlDocumentTreeGlobals existing{0x9000u, 4u};
    bool caught = false;
    try {
        (void)pal4::XmlDocumentRecord::CreateAttributeTree(
            0x3000u, existing, failed_shared);
    } catch (const std::bad_alloc&) {
        caught = true;
    }
    Check(caught && existing.nil_node == 0x9000u &&
              existing.construction_count == 4u &&
              failed_shared.frees ==
                  std::vector<std::uint32_t>{0x7000u},
          "failed later tree retains the preexisting shared nil");
}

void TestAttributeTreeRotationsAndMinimum() {
    pal4::XmlDocumentTreeGlobals globals{0x6000u, 1u};
    Runtime root;
    root.words[0x1004u] = 0x5000u;
    root.words[0x5004u] = 0x7000u;
    root.words[0x7000u] = globals.nil_node;
    root.words[0x7004u] = 0x5000u;
    root.words[0x7008u] = 0x8000u;
    root.words[0x8000u] = 0x9000u;
    root.words[0x8004u] = 0x7000u;
    root.words[0x8008u] = globals.nil_node;
    root.words[0x9000u] = globals.nil_node;
    root.words[0x9004u] = 0x8000u;
    Check(pal4::XmlDocumentRecord::MinimumAttributeNode(
              0x8000u, globals, root) == 0x9000u &&
              pal4::XmlDocumentRecord::MinimumAttributeNode(
                  0x9000u, globals, root) == 0x9000u,
          "tree minimum follows left links and accepts an existing minimum");
    Check(pal4::XmlDocumentRecord::RotateAttributeLeft(
              0x1000u, 0x7000u, globals, root) == 0x8000u &&
              root.words[0x5004u] == 0x8000u &&
              root.words[0x8004u] == 0x5000u &&
              root.words[0x8000u] == 0x7000u &&
              root.words[0x7004u] == 0x8000u &&
              root.words[0x7008u] == 0x9000u &&
              root.words[0x9004u] == 0x7000u,
          "left root rotation reparents its middle subtree");
    Check(pal4::XmlDocumentRecord::RotateAttributeRight(
              0x1000u, 0x8000u, globals, root) == 0x7000u &&
              root.words[0x5004u] == 0x7000u &&
              root.words[0x7004u] == 0x5000u &&
              root.words[0x7008u] == 0x8000u &&
              root.words[0x8004u] == 0x7000u &&
              root.words[0x8000u] == 0x9000u &&
              root.words[0x9004u] == 0x8000u,
          "right root rotation reverses all left-rotation links");

    Runtime left_child;
    left_child.words[0x1004u] = 0x5000u;
    left_child.words[0x5004u] = 0xa000u;
    left_child.words[0xa000u] = 0x7000u;
    left_child.words[0x7004u] = 0xa000u;
    left_child.words[0x7008u] = 0x8000u;
    left_child.words[0x8000u] = globals.nil_node;
    Check(pal4::XmlDocumentRecord::RotateAttributeLeft(
              0x1000u, 0x7000u, globals, left_child) == 0x8000u &&
              left_child.words[0xa000u] == 0x8000u &&
              left_child.words[0x8004u] == 0xa000u &&
              left_child.words[0x8000u] == 0x7000u &&
              left_child.words[0x7008u] == globals.nil_node &&
              left_child.words.count(globals.nil_node + 4u) == 0u,
          "left rotation replaces a parent's left link without parenting nil");

    Runtime right_child;
    right_child.words[0x1004u] = 0x5000u;
    right_child.words[0x5004u] = 0xa000u;
    right_child.words[0xa008u] = 0x7000u;
    right_child.words[0x7004u] = 0xa000u;
    right_child.words[0x7000u] = 0x8000u;
    right_child.words[0x8008u] = globals.nil_node;
    Check(pal4::XmlDocumentRecord::RotateAttributeRight(
              0x1000u, 0x7000u, globals, right_child) == 0x8000u &&
              right_child.words[0xa008u] == 0x8000u &&
              right_child.words[0x8004u] == 0xa000u &&
              right_child.words[0x8008u] == 0x7000u &&
              right_child.words[0x7000u] == globals.nil_node &&
              right_child.words.count(globals.nil_node + 4u) == 0u,
          "right rotation replaces a parent's right link without parenting nil");
}

void TestAdvanceTreeIterator() {
    constexpr std::uint32_t nil = 0x9000u;
    pal4::XmlDocumentTreeGlobals globals{nil, 1u};
    Runtime runtime;
    runtime.words[0x8004u] = 0xb000u;
    runtime.words[0x8008u] = 0xc000u;
    runtime.words[0xa004u] = 0xb000u;
    runtime.words[0xa008u] = nil;
    runtime.words[0xb004u] = 0x8000u;
    runtime.words[0xb008u] = 0xc000u;
    runtime.words[0xc000u] = nil;
    runtime.words[0xc004u] = 0xb000u;
    runtime.words[0xc008u] = nil;

    runtime.words[0x5000u] = 0xa000u;
    Check(pal4::XmlDocumentRecord::AdvanceTreeIterator(
              0x5000u, globals, runtime) == 0xb000u &&
              runtime.words[0x5000u] == 0xb000u,
          "iterator ascends from left child to parent");
    Check(pal4::XmlDocumentRecord::AdvanceTreeIterator(
              0x5000u, globals, runtime) == nil &&
              runtime.words[0x5000u] == 0xc000u,
          "iterator descends into right subtree minimum");
    Check(pal4::XmlDocumentRecord::AdvanceTreeIterator(
              0x5000u, globals, runtime) == 0x8000u &&
              runtime.words[0x5000u] == 0x8000u,
          "iterator moves from rightmost node to head");

    Runtime singleton;
    singleton.words[0x5000u] = 0xa000u;
    singleton.words[0xa004u] = 0x8000u;
    singleton.words[0xa008u] = nil;
    singleton.words[0x8004u] = 0xa000u;
    singleton.words[0x8008u] = 0xa000u;
    Check(pal4::XmlDocumentRecord::AdvanceTreeIterator(
              0x5000u, globals, singleton) == 0xa000u &&
              singleton.words[0x5000u] == 0x8000u,
          "single-node iterator retains native return register while advancing to head");
}

void TestAdvanceAndGetPrevious() {
    pal4::XmlDocumentTreeGlobals globals{0x9000u, 1u};
    Runtime runtime;
    runtime.words[0x5000u] = 0xb000u;
    runtime.words[0x8004u] = 0xb000u;
    runtime.words[0x8008u] = 0xc000u;
    runtime.words[0xb004u] = 0x8000u;
    runtime.words[0xb008u] = 0xc000u;
    runtime.words[0xc000u] = 0xd000u;
    runtime.words[0xc004u] = 0xb000u;
    runtime.words[0xc008u] = globals.nil_node;
    runtime.words[0xd000u] = globals.nil_node;
    runtime.words[0xd004u] = 0xc000u;
    runtime.words[0xd008u] = globals.nil_node;

    Check(pal4::XmlDocumentRecord::AdvanceAndGetPrevious(
              0x5000u, 0x6000u, globals, runtime) == 0x6000u &&
              runtime.words[0x5000u] == 0xd000u &&
              runtime.words[0x6000u] == 0xb000u,
          "advance with previous output descends to right-subtree minimum");
    Check(pal4::XmlDocumentRecord::AdvanceAndGetPrevious(
              0x5000u, 0x6000u, globals, runtime) == 0x6000u &&
              runtime.words[0x5000u] == 0xc000u &&
              runtime.words[0x6000u] == 0xd000u,
          "advance with previous output ascends from a left child");
    Check(pal4::XmlDocumentRecord::AdvanceAndGetPrevious(
              0x5000u, 0x6000u, globals, runtime) == 0x6000u &&
              runtime.words[0x5000u] == 0x8000u &&
              runtime.words[0x6000u] == 0xc000u,
          "advance with previous output moves the last node to head");
}

void TestAllocateAttributeTreeNode() {
    Runtime runtime;
    Check(pal4::XmlDocumentRecord::AllocateAttributeTreeNode(
              0x5000u, 1u, runtime) == 0x7000u &&
              runtime.words[0x7004u] == 0x5000u &&
              runtime.words[0x7038u] == 1u &&
              runtime.allocations == std::vector<std::uint32_t>{0x7000u},
          "attribute tree allocation initializes parent and flag");

    Runtime failed;
    failed.fail_attempt = 1u;
    bool caught = false;
    try {
        (void)pal4::XmlDocumentRecord::AllocateAttributeTreeNode(
            0x5000u, 0u, failed);
    } catch (const std::bad_alloc&) {
        caught = true;
    }
    Check(caught && failed.words.empty(),
          "attribute tree allocation failure writes no node fields");
}

void TestRetreatTreeIterator() {
    pal4::XmlDocumentTreeGlobals globals{0x9000u, 1u};
    Runtime runtime;
    runtime.words[0x8000u] = 0xa000u;
    runtime.words[0x8004u] = 0xb000u;
    runtime.words[0xa000u] = globals.nil_node;
    runtime.words[0xa004u] = 0xb000u;
    runtime.words[0xa008u] = 0xd000u;
    runtime.words[0xb000u] = 0xa000u;
    runtime.words[0xb004u] = 0x8000u;
    runtime.words[0xb008u] = 0xc000u;
    runtime.words[0xb038u] = 1u;
    runtime.words[0xc000u] = globals.nil_node;
    runtime.words[0xc004u] = 0xb000u;
    runtime.words[0xd000u] = globals.nil_node;
    runtime.words[0xd004u] = 0xa000u;
    runtime.words[0xd008u] = globals.nil_node;
    runtime.words[0xd038u] = 1u;

    runtime.words[0x5000u] = 0xb000u;
    Check(pal4::XmlDocumentRecord::RetreatTreeIterator(
              0x5000u, globals, runtime) == globals.nil_node &&
              runtime.words[0x5000u] == 0xd000u,
          "iterator retreat finds the rightmost node of a left subtree");
    Check(pal4::XmlDocumentRecord::RetreatTreeIterator(
              0x5000u, globals, runtime) == 0xa000u &&
              runtime.words[0x5000u] == 0xa000u,
          "iterator retreat ascends from a right child");
    Check(pal4::XmlDocumentRecord::RetreatTreeIterator(
              0x5000u, globals, runtime) == 0x8000u &&
              runtime.words[0x5000u] == 0x8000u,
          "iterator retreat ascends from the minimum to head");

    Runtime head;
    head.words[0x5000u] = 0x8000u;
    head.words[0x8038u] = 0u;
    head.words[0x8004u] = globals.nil_node;
    head.words[globals.nil_node + 4u] = 0x8000u;
    head.words[0x8008u] = 0xc000u;
    Check(pal4::XmlDocumentRecord::RetreatTreeIterator(
              0x5000u, globals, head) == 0xc000u &&
              head.words[0x5000u] == 0xc000u,
          "special head retreat jumps to the maximum node");
}

void TestCopyAttributeTreePayload() {
    Runtime shared;
    shared.bytes[0x2000u] = 0x5au;
    shared.words[0x2004u] = 0x9001u;
    shared.words[0x2008u] = 3u;
    shared.words[0x200cu] = 8u;
    shared.bytes[0x9000u] = 2u;
    pal4::XmlDocumentRecord::CopyAttributeTreePayload(
        0xa000u, 0x2000u, shared);
    Check(shared.bytes[0xa000u] == 0x5au &&
              shared.words[0xa004u] == 0x9001u &&
              shared.words[0xa008u] == 3u &&
              shared.words[0xa00cu] == 8u &&
              shared.bytes[0x9000u] == 3u &&
              shared.text_allocations.empty() &&
              shared.embedded_attribute_copies ==
                  (std::vector<std::pair<std::uint32_t,
                                         std::uint32_t>>{{0xa010u, 0x2010u}}),
          "copy payload shares reference-counted name and copies attribute");

    Runtime deep;
    deep.bytes[0x2000u] = 0x3cu;
    deep.words[0x2004u] = 0x9001u;
    deep.words[0x2008u] = 3u;
    deep.bytes[0x9000u] = 0xfeu;
    deep.bytes[0x9001u] = 'x';
    deep.bytes[0x9002u] = 'y';
    deep.bytes[0x9003u] = 'z';
    deep.allocated_text_pointer = 0xb001u;
    pal4::XmlDocumentRecord::CopyAttributeTreePayload(
        0xa000u, 0x2000u, deep);
    Check(deep.bytes[0xa000u] == 0x3cu &&
              deep.words[0xa004u] == 0xb001u &&
              deep.words[0xa008u] == 3u &&
              deep.bytes[0xb001u] == 'x' &&
              deep.bytes[0xb002u] == 'y' &&
              deep.bytes[0xb003u] == 'z' &&
              deep.bytes[0x9000u] == 0xfeu &&
              deep.text_allocations ==
                  (std::vector<std::pair<std::uint32_t,
                                         std::uint32_t>>{{0xa000u, 3u}}) &&
              deep.embedded_attribute_copies.size() == 1u,
          "copy payload duplicates non-shareable name bytes");

    Runtime empty;
    empty.allocated_text_pointer = 0xb001u;
    pal4::XmlDocumentRecord::CopyAttributeTreePayload(
        0xa000u, 0x2000u, empty);
    Check(empty.text_allocations ==
              (std::vector<std::pair<std::uint32_t,
                                     std::uint32_t>>{{0xa000u, 0u}}) &&
              empty.words[0xa008u] == 0u &&
              empty.embedded_attribute_copies.size() == 1u,
          "empty name still invokes text allocation and attribute copy");

    Runtime failed;
    failed.words[0x2008u] = 1u;
    failed.words[0x2004u] = 0x9001u;
    failed.bytes[0x9000u] = 0xffu;
    failed.text_allocation_result = 0u;
    pal4::XmlDocumentRecord::CopyAttributeTreePayload(
        0xa000u, 0x2000u, failed);
    Check(failed.words[0xa004u] == 0u &&
              failed.words[0xa008u] == 0u &&
              failed.embedded_attribute_copies.size() == 1u,
          "failed name allocation still reaches embedded attribute copy");

    Runtime self;
    self.words[0x2008u] = 2u;
    pal4::XmlDocumentRecord::CopyAttributeTreePayload(
        0x2000u, 0x2000u, self);
    Check(self.words[0x2008u] == 0u &&
              self.text_detachments ==
                  (std::vector<std::uint32_t>{0x2000u, 0x2000u}) &&
              self.embedded_attribute_copies.size() == 1u,
          "self-copy resets name before both detach calls");
    pal4::XmlDocumentRecord::CopyAttributeTreePayload(0u, 0x2000u, self);
    Check(self.embedded_attribute_copies.size() == 1u,
          "null destination exits without copying attribute");
}

void InitializeInsertionTree(Runtime& runtime, std::uint8_t mode) {
    runtime.words[0x1004u] = 0x5000u;
    runtime.bytes[0x1008u] = mode;
    runtime.words[0x100cu] = 0u;
    runtime.words[0x5000u] = 0x5000u;
    runtime.words[0x5004u] = 0x6000u;
    runtime.words[0x5008u] = 0x5000u;
    runtime.words[0x6000u] = 0x6000u;
    runtime.words[0x6008u] = 0x6000u;
    runtime.words[0x6038u] = 1u;
}

void SetInsertionInput(Runtime& runtime, std::uint32_t record,
                       char name) {
    const auto text = record + 0x10000u;
    runtime.words[record + 4u] = text;
    runtime.words[record + 8u] = 1u;
    runtime.bytes[text] = static_cast<std::uint8_t>(name);
}

void CheckInsertionTree(Runtime& runtime, const std::string& expected) {
    constexpr std::uint32_t head = 0x5000u;
    constexpr std::uint32_t nil = 0x6000u;
    const auto root = runtime.words[head + 4u];
    std::set<std::uint32_t> visited;
    std::string inorder;
    std::uint32_t minimum = 0u;
    std::uint32_t maximum = 0u;
    std::function<int(std::uint32_t, std::uint32_t)> visit =
        [&](const std::uint32_t node, const std::uint32_t parent) -> int {
        if (node == nil) return 1;
        Check(visited.insert(node).second &&
                  runtime.words[node + 4u] == parent &&
                  runtime.words[node + 56u] <= 1u,
              "inserted tree has unique nodes, correct parents and colors");
        const auto left = runtime.words[node];
        const auto right = runtime.words[node + 8u];
        if (runtime.words[node + 56u] == 0u)
            Check(runtime.words[left + 56u] == 1u &&
                      runtime.words[right + 56u] == 1u,
                  "inserted red node has black children");
        const auto left_depth = visit(left, node);
        if (minimum == 0u) minimum = node;
        inorder.push_back(static_cast<char>(
            runtime.bytes[runtime.words[node + 16u]]));
        maximum = node;
        const auto right_depth = visit(right, node);
        Check(left_depth == right_depth,
              "inserted tree paths have equal black height");
        return left_depth + static_cast<int>(runtime.words[node + 56u]);
    };
    if (root != nil)
        Check(runtime.words[root + 56u] == 1u,
              "inserted tree root is black");
    (void)visit(root, head);
    Check(inorder == expected &&
              runtime.words[0x100cu] == expected.size() &&
              runtime.words[head] ==
                  (expected.empty() ? head : minimum) &&
              runtime.words[head + 8u] ==
                  (expected.empty() ? head : maximum),
          "inserted tree maintains key order, count and extremes");
}

void TestInsertAttributeTreeNodeAt() {
    pal4::XmlDocumentTreeGlobals globals{0x6000u, 1u};
    Runtime runtime;
    InitializeInsertionTree(runtime, 0u);
    std::uint32_t root = 0u;
    for (const auto name : std::string{"bac"}) {
        const auto input = 0x2000u + runtime.allocations.size() * 0x20u;
        SetInsertionInput(runtime, input, name);
        const auto parent = root == 0u ? 0x5000u : root;
        Check(pal4::XmlDocumentRecord::InsertAttributeTreeNodeAt(
                  0x1000u, 0x4000u, globals.nil_node, parent,
                  input, globals, runtime) == 0x4000u,
              "positioned insertion returns its result address");
        if (root == 0u) root = runtime.words[0x4000u];
        Check(runtime.words[0x4000u] == runtime.allocations.back() &&
                  runtime.words[0x4000u + 4u] == 0u,
              "positioned insertion writes the newly allocated node");
        CheckInsertionTree(runtime, name == 'b' ? "b" :
                                  name == 'a' ? "ab" : "abc");
    }
    Check(runtime.words[0x5004u] == root &&
              runtime.embedded_attribute_copies.size() == 3u,
          "positioned insertion handles root, left and right attachment");

    Runtime failed;
    InitializeInsertionTree(failed, 0u);
    failed.fail_attempt = 1u;
    SetInsertionInput(failed, 0x2000u, 'x');
    bool caught = false;
    try {
        (void)pal4::XmlDocumentRecord::InsertAttributeTreeNodeAt(
            0x1000u, 0x4000u, globals.nil_node, 0x5000u,
            0x2000u, globals, failed);
    } catch (const std::bad_alloc&) {
        caught = true;
    }
    Check(caught && failed.words[0x100cu] == 0u &&
              failed.words[0x5004u] == globals.nil_node &&
              failed.embedded_attribute_copies.empty(),
          "positioned insertion allocation failure leaves the tree empty");
}

void TestInsertAttributeTreeNode() {
    pal4::XmlDocumentTreeGlobals globals{0x6000u, 1u};
    Runtime unique;
    InitializeInsertionTree(unique, 0u);
    const std::string values = "bacade";
    const std::vector<std::string> expected = {
        "b", "ab", "abc", "abc", "abcd", "abcde"};
    std::uint32_t first_a = 0u;
    for (std::uint32_t index = 0u; index < values.size(); ++index) {
        const auto input = 0x2000u + index * 0x20u;
        SetInsertionInput(unique, input, values[index]);
        Check(pal4::XmlDocumentRecord::InsertAttributeTreeNode(
                  0x1000u, 0x4000u, input, globals, unique) == 0x4000u,
              "insert returns result storage address");
        if (index == 1u) first_a = unique.words[0x4000u];
        Check(unique.bytes[0x4004u] == (index == 3u ? 0u : 1u) &&
                  (index != 3u || unique.words[0x4000u] == first_a),
              "unique insertion reports duplicate without allocating a node");
        CheckInsertionTree(unique, expected[index]);
    }
    Check(unique.allocations.size() == 5u &&
              unique.embedded_attribute_copies.size() == 5u,
          "duplicate unique key does not copy payload");

    for (const std::string pattern : {"cab", "acb"}) {
        Runtime zigzag;
        InitializeInsertionTree(zigzag, 0u);
        for (std::uint32_t index = 0u; index < pattern.size(); ++index) {
            const auto input = 0x2000u + index * 0x20u;
            SetInsertionInput(zigzag, input, pattern[index]);
            (void)pal4::XmlDocumentRecord::InsertAttributeTreeNode(
                0x1000u, 0x4000u, input, globals, zigzag);
        }
        CheckInsertionTree(zigzag, "abc");
        const auto root = zigzag.words[0x5004u];
        Check(zigzag.bytes[zigzag.words[root + 16u]] == 'b',
              "zigzag insertion rotates middle key to root");
    }

    Runtime multiple;
    InitializeInsertionTree(multiple, 1u);
    for (std::uint32_t index = 0u; index < 3u; ++index) {
        const auto input = 0x2000u + index * 0x20u;
        SetInsertionInput(multiple, input, 'b');
        (void)pal4::XmlDocumentRecord::InsertAttributeTreeNode(
            0x1000u, 0x4000u, input, globals, multiple);
        Check(multiple.bytes[0x4004u] == 1u,
              "multi-key mode inserts equal names");
        CheckInsertionTree(multiple, std::string(index + 1u, 'b'));
    }

    Runtime failed;
    InitializeInsertionTree(failed, 1u);
    failed.fail_attempt = 1u;
    SetInsertionInput(failed, 0x2000u, 'x');
    bool caught = false;
    try {
        (void)pal4::XmlDocumentRecord::InsertAttributeTreeNode(
            0x1000u, 0x4000u, 0x2000u, globals, failed);
    } catch (const std::bad_alloc&) {
        caught = true;
    }
    Check(caught && failed.words[0x100cu] == 0u &&
              failed.words[0x5004u] == 0x6000u &&
              failed.embedded_attribute_copies.empty(),
          "allocation failure leaves tree links and count unchanged");

    Runtime mixed;
    InitializeInsertionTree(mixed, 0u);
    std::map<char, std::uint32_t> nodes_by_name;
    const std::string insertion_order = "dbfaceg";
    for (std::uint32_t index = 0u; index < insertion_order.size(); ++index) {
        const auto input = 0x2000u + index * 0x20u;
        SetInsertionInput(mixed, input, insertion_order[index]);
        mixed.bytes[input + 0xffffu] = 2u;
        (void)pal4::XmlDocumentRecord::InsertAttributeTreeNode(
            0x1000u, 0x4000u, input, globals, mixed);
        nodes_by_name[insertion_order[index]] = mixed.words[0x4000u];
    }
    std::string remaining = "abcdefg";
    CheckInsertionTree(mixed, remaining);
    for (const char name : std::string{"dacfgeb"}) {
        const auto node = nodes_by_name.at(name);
        (void)pal4::XmlDocumentRecord::RemoveAttributeNode(
            0x1000u, 0x4500u, node, globals, mixed);
        remaining.erase(remaining.find(name), 1u);
        CheckInsertionTree(mixed, remaining);
    }
    Check(mixed.words[0x5004u] == 0x6000u &&
              mixed.frees.size() == insertion_order.size(),
          "inserted tree can delete all nodes across root and edge positions");
}

void TestAddAttributeDuplicateAndNewName() {
    Runtime duplicate;
    duplicate.words[0x3018u] = 0x8000u;
    duplicate.words[0xd004u] = 0x9100u;
    duplicate.bytes[0x9100u] = 'h';
    duplicate.bytes[0x9101u] = 'p';
    duplicate.bytes[0x9102u] = 0u;
    duplicate.attribute_find_result = 0x8100u;
    Check(pal4::XmlDocumentRecord::AddAttribute(
              0x3000u, 0xd000u, duplicate) == 1u &&
              duplicate.attribute_queries ==
                  (std::vector<std::pair<std::uint32_t, std::string>>{
                      {0x3014u, "hp"}}) &&
              duplicate.attribute_events ==
                  std::vector<std::string>{"find"} &&
              duplicate.attribute_insertions.empty() &&
              duplicate.attribute_assignments.empty() &&
              duplicate.words[0x3034u] == 0u,
          "existing attribute name returns without replacing the value");

    Runtime missing;
    missing.words[0x3018u] = 0x8000u;
    missing.words[0x3034u] = 2u;
    missing.words[0xd004u] = 0x9100u;
    missing.bytes[0x9100u] = 'h';
    missing.bytes[0x9101u] = 'p';
    missing.bytes[0x9102u] = 0u;
    missing.attribute_find_result = 0x8000u;
    Check(pal4::XmlDocumentRecord::AddAttribute(
              0x3000u, 0xd000u, missing) == 1u &&
              missing.attribute_queries ==
                  (std::vector<std::pair<std::uint32_t, std::string>>{
                      {0x3014u, "hp"}}) &&
              missing.attribute_insertions ==
                  (std::vector<std::pair<std::uint32_t, std::string>>{
                      {0x3014u, "hp"}}) &&
              missing.attribute_assignments ==
                  (std::vector<std::pair<std::uint32_t,
                                         std::uint32_t>>{
                      {0x901cu, 0xd000u}}) &&
              missing.attribute_events ==
                  (std::vector<std::string>{"find", "insert", "assign"}) &&
              missing.words[0x3034u] == 3u,
          "new attribute is inserted, assigned and counted in order");
}

void TestAddAttributeEmptyNameAndFailures() {
    Runtime empty;
    empty.words[0x3018u] = 0x8000u;
    empty.attribute_find_result = 0x8000u;
    Check(pal4::XmlDocumentRecord::AddAttribute(
              0x3000u, 0xd000u, empty) == 1u &&
              empty.attribute_queries ==
                  (std::vector<std::pair<std::uint32_t, std::string>>{
                      {0x3014u, ""}}) &&
              empty.attribute_insertions ==
                  (std::vector<std::pair<std::uint32_t, std::string>>{
                      {0x3014u, ""}}) &&
              empty.words[0x3034u] == 1u,
          "null attribute name resolves to the empty literal");

    for (const bool fail_insert : {true, false}) {
        Runtime runtime;
        runtime.words[0x3018u] = 0x8000u;
        runtime.attribute_find_result = 0x8000u;
        runtime.fail_attribute_insert = fail_insert;
        runtime.fail_attribute_assign = !fail_insert;
        bool caught = false;
        try {
            (void)pal4::XmlDocumentRecord::AddAttribute(
                0x3000u, 0xd000u, runtime);
        } catch (const std::bad_alloc&) {
            caught = true;
        }
        Check(caught && runtime.words[0x3034u] == 0u &&
                  runtime.attribute_events ==
                      (fail_insert
                           ? std::vector<std::string>{"find", "insert"}
                           : std::vector<std::string>{
                                 "find", "insert", "assign"}),
              "failed insert or assignment does not increment attribute count");
    }
}

void TestNodeDestructorReleasesChildrenTreeAndSharedName() {
    Runtime runtime;
    pal4::XmlDocumentTreeGlobals globals{0x9000u, 1u};
    runtime.words[0x3004u] = 0xa001u;
    runtime.bytes[0xa000u] = 2u;
    runtime.words[0x3018u] = 0x8000u;
    runtime.words[0x3020u] = 1u;
    runtime.words[0x3028u] = 0xb000u;
    runtime.words[0x302cu] = 0xb004u;
    runtime.words[0x3030u] = 0xb008u;
    runtime.words[0x8000u] = 0xc000u;
    runtime.words[0x8004u] = 0xc000u;
    Check(pal4::XmlDocumentRecord::DestroyNode(
              0x3000u, globals, runtime) == 0xa000u &&
              runtime.cleaned_nodes == std::vector<std::uint32_t>{0x3000u} &&
              runtime.frees ==
                  (std::vector<std::uint32_t>{
                      0xb000u, 0x8000u, 0x9000u}) &&
              runtime.deleted_nodes == std::vector<std::uint32_t>{0xc000u} &&
              runtime.first_element_requests ==
                  (std::vector<std::uint32_t>{0x3014u, 0x3014u}) &&
              runtime.bytes[0xa000u] == 1u &&
              runtime.words[0x3004u] == 0u &&
              runtime.words[0x3008u] == 0u &&
              runtime.words[0x300cu] == 0u &&
              runtime.words[0x3018u] == 0u &&
              runtime.words[0x3028u] == 0u &&
              runtime.words[0x302cu] == 0u &&
              runtime.words[0x3030u] == 0u,
          "node destructor releases child vector, tree and shared name");
}

void TestNodeDestructorUniqueAndAbsentNames() {
    for (const auto header : {0u, 0xffu}) {
        Runtime runtime;
        pal4::XmlDocumentTreeGlobals globals{0x9000u, 2u};
        runtime.words[0x3004u] = 0xa001u;
        runtime.bytes[0xa000u] = static_cast<std::uint8_t>(header);
        runtime.words[0x3018u] = 0x8000u;
        runtime.words[0x8000u] = 0x8000u;
        Check(pal4::XmlDocumentRecord::DestroyNode(
                  0x3000u, globals, runtime) == 0xfaceu &&
                  runtime.frees ==
                      (std::vector<std::uint32_t>{
                          0u, 0x8000u, 0xa000u}) &&
                  globals.nil_node == 0x9000u &&
                  globals.construction_count == 1u,
              "unique name header releases name allocation");
    }

    Runtime runtime;
    pal4::XmlDocumentTreeGlobals globals{0x9000u, 2u};
    runtime.words[0x3018u] = 0x8000u;
    runtime.words[0x8000u] = 0x8000u;
    Check(pal4::XmlDocumentRecord::DestroyNode(
              0x3000u, globals, runtime) == 0u &&
              runtime.frees ==
                  (std::vector<std::uint32_t>{0u, 0x8000u}),
          "absent name returns zero after child vector and tree cleanup");
}

}  // namespace

int main() {
    TestFirstDocumentEstablishesNilNode();
    TestLaterDocumentDiscardsProposedNil();
    TestAllocationFailures();
    TestStringAndParentInitialization();
    TestEmptyAndNullSource();
    TestStringConstructorFailures();
    TestDestructorReleasesSharedNilOnLastDocument();
    TestDestructorRetainsSharedNilWithOtherDocuments();
    TestDestructorWholeTreePath();
    TestDestructorIteratesUncountedNodes();
    TestCopyConstructorCopiesAttributesAndChildren();
    TestCopyConstructorNameFallback();
    TestCopyConstructorCleansTemporaryAfterInsertFailure();
    TestCopyAssignmentReusesCopyFlow();
    TestCopyAssignmentWithEmptyName();
    TestSetNameNullAndEmptySources();
    TestSetNameCapacityAndSharing();
    TestSetNameAllocationFailureStopsBeforeParent();
    TestCloneCopiesAttributesAndNestedChild();
    TestCloneAllocationFailures();
    TestAssignmentResultFollowsSourceCounts();
    TestCleanupDetachesFromParent();
    TestCleanupReleasesAttributeTree();
    TestCleanupRecursivelyDestroysChildren();
    TestCleanupNormalizesNameBuffer();
    TestGetAttributeByIndex();
    TestGetChildByIndex();
    TestFindChildByName();
    TestFindNextChildByName();
    TestAddChildNullAndInPlace();
    TestAddChildGrowsVector();
    TestAddChildMovesBetweenParents();
    TestAddChildReordersWithinParent();
    TestAddChildAllocationFailureAfterDetach();
    TestCreateNodeInitializesAndAttaches();
    TestCreateNodeAllocationFailures();
    TestRemoveChildMembershipAndDestruction();
    TestCleanupDocumentValue();
    TestRemoveAttributeNodeTreeCases();
    TestRemoveAttributeNodeReleasesName();
    TestClearAttributeRange();
    TestAdvanceTreeIterator();
    TestAdvanceAndGetPrevious();
    TestAllocateAttributeTreeNode();
    TestRetreatTreeIterator();
    TestCopyAttributeTreePayload();
    TestInsertAttributeTreeNode();
    TestInsertAttributeTreeNodeAt();
    TestDeleteAttributeSubtree();
    TestCreateAttributeTree();
    TestAttributeTreeRotationsAndMinimum();
    TestAddAttributeDuplicateAndNewName();
    TestAddAttributeEmptyNameAndFailures();
    TestNodeDestructorReleasesChildrenTreeAndSharedName();
    TestNodeDestructorUniqueAndAbsentNames();
}
