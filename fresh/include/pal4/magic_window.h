#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <string_view>

namespace pal4 {

struct MagicWindowState {
    std::uint32_t vtable{};
    std::uint32_t member_vtable{};
    std::uint32_t selection_vtable{};
    std::uint32_t list_vtable{};
    std::uint32_t event_vtable{};
    std::int32_t selected_character_index{};
    std::uint32_t category_position{};
    std::uint32_t member_storage{};
    std::uint32_t member_size{};
    std::uint32_t member_capacity{};
    std::array<std::uint32_t, 6> slot_order{};
    std::array<std::string, 7> stunt_text{};
    std::uint32_t list_owner{};
    std::uint32_t selection_owner{};
};

class IMagicWindowRuntime {
public:
    virtual ~IMagicWindowRuntime() = default;
    virtual void LoadLayout(std::uint32_t object, std::uint32_t name) = 0;
    virtual void ConstructMember(std::uint32_t object) = 0;
    virtual void ConstructTextArray(std::uint32_t object, std::uint32_t count) = 0;
    virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    virtual std::uint32_t FrameManager() = 0;
    virtual void InsertOrReplace(std::uint32_t tree,
                                 std::string_view command,
                                 std::uint32_t& callback) = 0;
    virtual void DeleteCallback(std::uint32_t callback, std::uint32_t flags) = 0;
    virtual bool HasStuntText(std::uint32_t index) = 0;
    virtual std::string ReadStuntText(std::uint32_t index) = 0;
    virtual void InitializePanel(std::uint32_t object) = 0;
    virtual void DestroyTextArray(std::uint32_t object, std::uint32_t count) = 0;
    virtual void FreeMemberStorage(std::uint32_t storage) = 0;
    virtual std::uint32_t DestroyBase(std::uint32_t object) = 0;
};

class MagicWindowRuntimeStub : public IMagicWindowRuntime {
public:
    void LoadLayout(std::uint32_t, std::uint32_t) override {}
    void ConstructMember(std::uint32_t) override {}
    void ConstructTextArray(std::uint32_t, std::uint32_t) override {}
    std::uint32_t Allocate(std::uint32_t) override { return 0; }
    void WriteWord(std::uint32_t, std::uint32_t) override {}
    std::uint32_t FrameManager() override { return 0; }
    void InsertOrReplace(std::uint32_t, std::string_view,
                         std::uint32_t&) override {}
    void DeleteCallback(std::uint32_t, std::uint32_t) override {}
    bool HasStuntText(std::uint32_t) override { return false; }
    std::string ReadStuntText(std::uint32_t) override { return {}; }
    void InitializePanel(std::uint32_t) override {}
    void DestroyTextArray(std::uint32_t, std::uint32_t) override {}
    void FreeMemberStorage(std::uint32_t) override {}
    std::uint32_t DestroyBase(std::uint32_t object) override { return object; }
};

class MagicWindow final {
public:
    static constexpr std::uint32_t kVtable = 0x843920u;
    static constexpr std::uint32_t kMemberVtable = 0x843938u;
    static constexpr std::uint32_t kSelectionVtable = 0x84397cu;
    static constexpr std::uint32_t kListVtable = 0x843964u;
    static constexpr std::uint32_t kEventVtable = 0x843960u;
    static constexpr std::uint32_t kCallbackVtable = 0x843914u;
    static constexpr std::uint32_t kCallbackHandler = 0x45a500u;

    static MagicWindowState& Construct(std::uint32_t object,
                                       std::uint32_t name,
                                       MagicWindowState& state,
                                       IMagicWindowRuntime& runtime);
    static std::uint32_t Destroy(std::uint32_t object,
                                  MagicWindowState& state,
                                  IMagicWindowRuntime& runtime);
};

}  // namespace pal4
