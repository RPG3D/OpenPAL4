#pragma once

#include <array>
#include <cstdint>
#include <deque>
#include <string>
#include <string_view>
#include <vector>

namespace pal4 {
struct MonsterBehaviorManagerState;

// Owns native encounter records/names. Published x86 views are borrowed by the
// existing game algorithms; strings have host STL ownership, no VC6 headers.
class NativeMonsterEncounterStorage final {
public:
    NativeMonsterEncounterStorage() = default;
    NativeMonsterEncounterStorage(const NativeMonsterEncounterStorage&) = delete;
    NativeMonsterEncounterStorage& operator=(const NativeMonsterEncounterStorage&) = delete;
    void Append(MonsterBehaviorManagerState& image,
        const std::array<std::uint32_t, 3>& record);
    void AddName(MonsterBehaviorManagerState& image, std::string_view name);
    bool ReleaseHistoryRecord(std::uint32_t record);
    bool ReleaseBuffer(std::uint32_t address);
private:
    struct TextView {
        std::uint32_t flags{}, data{}, length{}, capacity{};
    };
    static_assert(sizeof(TextView) == 16u);
    void Bind(MonsterBehaviorManagerState& image);
    void SynchronizeRecords();
    void SynchronizeNames();
    void PublishRecords();
    void PublishNames();
    MonsterBehaviorManagerState* image_{};
    std::vector<std::array<std::uint32_t, 3>> records_;
    std::vector<TextView> history_;
    // Stable strings across history-view growth, including SSO strings.
    std::deque<std::string> names_;
    bool records_owned_{}, names_owned_{};
};
}
