#pragma once
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace pal4 {
class NativeInventorySession;
struct NativeInventoryProperty {
    // Borrowed entry/definition addresses expire on inventory mutation/reset.
    std::uint32_t address{},definition{},id{},sequence{},count{};
    std::string name;
};
// Read the live inventory through the existing sequence/category algorithms.
// Returned text and recent sequence snapshots own their host storage.
class NativeInventoryPropertyReader final {
public:
    explicit NativeInventoryPropertyReader(NativeInventorySession&);
    std::int32_t Count(std::int32_t category);
    std::optional<NativeInventoryProperty> AtReverse(std::int32_t category,std::uint32_t index);
    std::optional<NativeInventoryProperty> Featured(std::uint32_t category);
    std::optional<NativeInventoryProperty> FindSequence(std::uint32_t sequence,std::uint32_t category=5u);
    std::optional<NativeInventoryProperty> FindId(std::uint32_t id,std::uint32_t category=5u);
    std::optional<NativeInventoryProperty> FindAddress(std::uint32_t address);
    std::vector<std::uint32_t> RecentSequences();
private:
    NativeInventorySession& inventory_;
};
}
