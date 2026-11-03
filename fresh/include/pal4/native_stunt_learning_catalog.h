#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>
namespace pal4 {
class NativeGameDatabase;
struct NativeGameDbTable;
// Immutable projection shared by Scene and Combat learning. Logical handles
// expose only ID and four learning requirements to the original430640 helper.
// They are never native Stunt pointers or an ending-UI record representation.
class NativeStuntLearningCatalog final {
public:
    static constexpr std::uint32_t kOwner = 1u;
    static constexpr std::uint32_t kRecords = 0x1000u;
    static constexpr std::uint32_t kRecordLimit = 0x100000u;
    explicit NativeStuntLearningCatalog(const NativeGameDatabase&);
    explicit NativeStuntLearningCatalog(const NativeGameDbTable&);
    std::size_t Count() const noexcept { return records_.size(); }
    std::uint32_t ReadWord(std::uint32_t address) const;
private:
    std::vector<std::array<std::uint32_t,5>> records_;
};
}
