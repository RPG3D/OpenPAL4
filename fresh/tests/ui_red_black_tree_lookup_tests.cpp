#include "pal4/ui_red_black_tree_lookup.h"

#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IUiRedBlackTreeLookupRuntime {
    static constexpr std::uint32_t tree = 0x1000u;
    static constexpr std::uint32_t head = 0x2000u;
    static constexpr std::uint32_t nil = 0x3000u;
    static constexpr std::uint32_t output = 0x4000u;
    static constexpr std::uint32_t query = 0x5000u;
    static constexpr std::uint32_t query_text = 0x6000u;
    static constexpr std::uint32_t empty_text = 0x7000u;
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::string> strings;
    std::vector<std::uint32_t> compared;

    Runtime() {
        words[tree + 4u] = head;
        words[head + 4u] = nil;
        words[query + 4u] = query_text;
        words[query + 8u] = 0u;
        strings[empty_text] = "";
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words.at(address);
    }
    std::uint32_t SharedNil() override { return nil; }
    std::uint32_t EmptyText() override { return empty_text; }
    std::int32_t CompareRange(std::uint32_t node_text,
                              std::uint32_t node_length,
                              std::uint32_t text,
                              std::uint32_t query_length) override {
        compared.push_back(node_text);
        const auto left = strings.at(node_text).substr(0u, node_length);
        const auto right = strings.at(text).substr(0u, query_length);
        return left.compare(right);
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
    }
    void Node(std::uint32_t address, std::uint32_t left,
              std::uint32_t right, const std::string& text) {
        words[address] = left;
        words[address + 8u] = right;
        words[address + 20u] = static_cast<std::uint32_t>(text.size());
        strings[address + 12u] = text;
    }
    std::uint32_t Find(const std::string& text, bool null_text = false) {
        compared.clear();
        strings[query_text] = text;
        words[query + 4u] = null_text ? 0u : query_text;
        words[query + 8u] = static_cast<std::uint32_t>(text.size());
        const auto result = pal4::UiRedBlackTreeLookup::LowerBound(
            tree, output, query, *this);
        Check(result == output, "returns output address");
        return words.at(output);
    }
};

void CheckEmpty() {
    Runtime r;
    Check(r.Find("x") == r.head, "empty tree returns head");
    Check(r.compared.empty(), "empty tree performs no comparison");
}

void CheckLowerBounds() {
    Runtime r;
    r.words[r.head + 4u] = 0x8000u;
    r.Node(0x8000u, 0x8100u, 0x8200u, "m");
    r.Node(0x8100u, r.nil, r.nil, "d");
    r.Node(0x8200u, r.nil, r.nil, "t");
    Check(r.Find("a") == 0x8100u, "below minimum returns minimum");
    Check(r.Find("d") == 0x8100u, "equal key descends left");
    Check(r.Find("h") == 0x8000u, "middle gap returns lower bound");
    Check(r.Find("t") == 0x8200u, "right exact key");
    Check(r.Find("z") == r.head, "above maximum returns head");
    Check(r.compared == (std::vector<std::uint32_t>{0x800cu, 0x820cu}),
          "right descent comparison order");
    Check(r.Find("", true) == 0x8100u,
          "null query text uses empty-string fallback");
}

void CheckLeftmostFromSlot() {
    Runtime r;
    r.words[0x9000u] = r.nil;
    Check(pal4::UiRedBlackTreeLookup::LeftmostFromSlot(0x9000u, r) ==
              0x9000u,
          "nil slot returns slot address");
    r.words[0x9000u] = 0x9100u;
    r.words[0x9100u] = 0x9200u;
    r.words[0x9200u] = r.nil;
    Check(pal4::UiRedBlackTreeLookup::LeftmostFromSlot(0x9000u, r) ==
              0x9200u,
          "left spine returns smallest node");
}

}  // namespace

int main() {
    try {
        CheckEmpty();
        CheckLowerBounds();
        CheckLeftmostFromSlot();
        std::cout << "ui_red_black_tree_lookup_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
