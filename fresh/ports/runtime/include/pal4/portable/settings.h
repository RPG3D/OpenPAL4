#pragma once
#include "pal4/portable/script_stack.h"
#include <array>
#include <filesystem>
#include <span>

namespace pal4::portable {
struct Settings {
    std::uint32_t movie_flags{};
    bool tutorials_enabled{true};
    std::array<std::uint32_t,3> pal_test_results{};
    // Original GlobalData+0x54: independent of the 128 tutorial seen bits.
    std::array<std::uint32_t,8> help_seen{};
    bool HelpSeen(std::uint32_t id) const;
    bool operator==(const Settings&) const=default;
};
std::array<std::uint8_t,64> EncodeSettings(const Settings& settings);
Settings DecodeSettings(std::span<const std::uint8_t> bytes);
class ISettingsStore {
public:
    virtual ~ISettingsStore()=default;
    virtual Settings Load()=0;
    virtual void Save(const Settings& settings)=0;
};
// One owner/game thread writes this file. A committed file is replaced only
// after the complete temporary payload has been flushed and closed.
class FileSettingsStore final:public ISettingsStore {
public:
    explicit FileSettingsStore(std::filesystem::path path):path_(std::move(path)) {}
    Settings Load() override;
    void Save(const Settings& settings) override;
private:
    std::filesystem::path path_;
};
class SettingsServices final:public IScriptContextServices {
public:
    SettingsServices(IScriptContextServices& next,Settings& state,ISettingsStore& store):next_(next),state_(state),store_(store) {}
    void OnCall(const ScriptRequest& request) override {next_.OnCall(request);}
    ScriptResponse Invoke(const ScriptRequest& request) override;
    void SaveTutorial(std::uint32_t key);
    void SetTutorialsEnabled(bool enabled);
    // Returns true only when a previously unread valid bit was committed.
    bool MarkHelpSeen(std::uint32_t id);
    unsigned Writes() const {return writes_;}
private:
    IScriptContextServices& next_;
    Settings& state_;
    ISettingsStore& store_;
    unsigned writes_{};
};
} // namespace pal4::portable
