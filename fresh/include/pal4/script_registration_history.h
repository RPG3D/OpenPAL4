#pragma once

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace pal4 {

class IScriptRegistrationHistoryRuntime {
public:
    virtual ~IScriptRegistrationHistoryRuntime() = default;
    virtual int RegisterAtExit(std::uint32_t callback) = 0;
};

class ScriptRegistrationHistory final {
public:
    using Entry = std::pair<std::string, std::string>;
    static constexpr std::uint32_t kCleanupCallback = 0x7ECD90u;

    int Initialize(std::uint8_t seed, IScriptRegistrationHistoryRuntime& runtime);
    void Record(const char* declaration, const char* description);
    void Cleanup();

    [[nodiscard]] std::uint8_t Seed() const { return seed_; }
    [[nodiscard]] const std::vector<Entry>& Entries() const { return entries_; }

private:
    std::uint8_t seed_{};
    std::vector<Entry> entries_;
};

}  // namespace pal4
