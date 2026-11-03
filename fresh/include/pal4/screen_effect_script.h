#pragma once

#include <cstdint>
#include <string_view>

namespace pal4 {

struct ScreenEffectState {
    std::uint32_t vtable{};
    std::uint8_t loaded{};
    std::uint8_t enabled{};
    std::uint8_t reserved[2]{};
    std::uint32_t script{};
};
static_assert(sizeof(ScreenEffectState) == 12u);

struct GlassEffectState : ScreenEffectState {
    std::uint32_t noise_texture{};
};
static_assert(sizeof(GlassEffectState) == 16u);

class IScreenEffectRuntime {
public:
    virtual ~IScreenEffectRuntime() = default;
    [[nodiscard]] virtual std::uint8_t ReadByte(
        std::uint32_t address) = 0;
    virtual void SelectTechnique(std::uint32_t script,
                                 std::string_view name) = 0;
    virtual void BindTexture(std::uint32_t script,
                             std::string_view name,
                             std::uint32_t texture) = 0;
    virtual void ExecuteTechnique(std::uint32_t script,
                                  std::uint32_t& input,
                                  std::uint32_t flags) = 0;
    virtual void Reset(ScreenEffectState& state) = 0;
    [[nodiscard]] virtual bool CheckCapability(
        std::uint32_t version) = 0;
    virtual void ProcessPackageScript() = 0;
    virtual void CleanupScriptEnvironment(std::uint32_t& local_word) = 0;
    [[nodiscard]] virtual std::uint32_t OpenFile(
        std::string_view path, std::string_view mode) = 0;
    virtual void SeekFile(std::uint32_t file, std::uint32_t offset,
                          std::uint32_t origin) = 0;
    [[nodiscard]] virtual std::uint32_t FileSize(
        std::uint32_t file) = 0;
    [[nodiscard]] virtual std::uint32_t AllocateBuffer(
        std::uint32_t bytes) = 0;
    virtual void ReadFile(std::uint32_t buffer, std::uint32_t bytes,
                          std::uint32_t count, std::uint32_t file) = 0;
    virtual void CloseFile(std::uint32_t file) = 0;
    [[nodiscard]] virtual std::uint32_t CurrentD3DDevice() = 0;
    virtual void LoadTexture(std::uint32_t device, std::uint32_t buffer,
                             std::uint32_t bytes,
                             std::uint32_t& output_texture) = 0;
    virtual void ReleaseEffectResource(std::uint32_t resource) = 0;
    [[nodiscard]] virtual std::uint32_t InvokeRenderSlot(
        std::uint32_t script, std::uint32_t slot_offset) = 0;
    [[nodiscard]] virtual std::uint32_t InvokeParameterSlot(
        std::uint32_t script, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t ReleaseScript(
        std::uint32_t script) = 0;
    [[nodiscard]] virtual std::uint32_t LoadScript(
        std::uint32_t device, std::uint32_t buffer,
        std::uint32_t bytes, std::uint32_t compile_flags,
        std::uint32_t compile_options, std::uint32_t configuration,
        std::uint32_t execution_flags, std::uint32_t& output_script,
        std::uint32_t& error) = 0;
    [[nodiscard]] virtual std::string_view ErrorText(
        std::uint32_t error, std::string_view label) = 0;
    virtual void ShowError(std::string_view text,
                           std::uint32_t message_box_type) = 0;
    virtual void ReleaseEnvironmentOnLoadError(
        std::uint32_t environment) = 0;
    virtual void FreeBuffer(std::uint32_t buffer) = 0;
};

class ScreenEffectRuntimeStub : public IScreenEffectRuntime {
public:
    [[nodiscard]] std::uint8_t ReadByte(std::uint32_t address) override;
    void SelectTechnique(std::uint32_t script,
                         std::string_view name) override;
    void BindTexture(std::uint32_t script,
                     std::string_view name,
                     std::uint32_t texture) override;
    void ExecuteTechnique(std::uint32_t script,
                          std::uint32_t& input,
                          std::uint32_t flags) override;
    void Reset(ScreenEffectState& state) override;
    [[nodiscard]] bool CheckCapability(std::uint32_t version) override;
    void ProcessPackageScript() override;
    void CleanupScriptEnvironment(std::uint32_t& local_word) override;
    [[nodiscard]] std::uint32_t OpenFile(std::string_view path,
                                         std::string_view mode) override;
    void SeekFile(std::uint32_t file, std::uint32_t offset,
                  std::uint32_t origin) override;
    [[nodiscard]] std::uint32_t FileSize(std::uint32_t file) override;
    [[nodiscard]] std::uint32_t AllocateBuffer(std::uint32_t bytes) override;
    void ReadFile(std::uint32_t buffer, std::uint32_t bytes,
                  std::uint32_t count, std::uint32_t file) override;
    void CloseFile(std::uint32_t file) override;
    [[nodiscard]] std::uint32_t CurrentD3DDevice() override;
    void LoadTexture(std::uint32_t device, std::uint32_t buffer,
                     std::uint32_t bytes,
                     std::uint32_t& output_texture) override;
    void ReleaseEffectResource(std::uint32_t resource) override;
    [[nodiscard]] std::uint32_t InvokeRenderSlot(
        std::uint32_t script, std::uint32_t slot_offset) override;
    [[nodiscard]] std::uint32_t InvokeParameterSlot(
        std::uint32_t script, std::uint32_t value) override;
    [[nodiscard]] std::uint32_t ReleaseScript(
        std::uint32_t script) override;
    [[nodiscard]] std::uint32_t LoadScript(
        std::uint32_t device, std::uint32_t buffer,
        std::uint32_t bytes, std::uint32_t compile_flags,
        std::uint32_t compile_options, std::uint32_t configuration,
        std::uint32_t execution_flags, std::uint32_t& output_script,
        std::uint32_t& error) override;
    [[nodiscard]] std::string_view ErrorText(
        std::uint32_t error, std::string_view label) override;
    void ShowError(std::string_view text,
                   std::uint32_t message_box_type) override;
    void ReleaseEnvironmentOnLoadError(
        std::uint32_t environment) override;
    void FreeBuffer(std::uint32_t buffer) override;
};

class ScreenEffectScript final {
public:
    [[nodiscard]] static std::uint32_t ApplyBloom(
        const ScreenEffectState& state, std::uint32_t input,
        IScreenEffectRuntime& runtime);
    [[nodiscard]] static bool LoadBloom(
        ScreenEffectState& state, std::uint32_t incoming_edi,
        std::uint32_t incoming_environment_word,
        IScreenEffectRuntime& runtime);
    static void NoOp(std::uint32_t first, std::uint32_t second);
    [[nodiscard]] static std::uint32_t ApplyBlur(
        const ScreenEffectState& state, std::uint32_t input,
        IScreenEffectRuntime& runtime);
    [[nodiscard]] static bool LoadBlur(
        ScreenEffectState& state, std::uint32_t incoming_edi,
        std::uint32_t incoming_environment_word,
        IScreenEffectRuntime& runtime);
    [[nodiscard]] static std::uint32_t ApplyEmbossed(
        const ScreenEffectState& state, std::uint32_t input,
        IScreenEffectRuntime& runtime);
    [[nodiscard]] static bool LoadEmbossed(
        ScreenEffectState& state, std::uint32_t incoming_edi,
        std::uint32_t incoming_environment_word,
        IScreenEffectRuntime& runtime);
    [[nodiscard]] static std::uint32_t ApplyGlass(
        const GlassEffectState& state, std::uint32_t input,
        IScreenEffectRuntime& runtime);
    [[nodiscard]] static bool LoadGlass(
        GlassEffectState& state, std::uint32_t incoming_edi,
        std::uint32_t incoming_environment_word,
        IScreenEffectRuntime& runtime);
    static void CleanupGlass(GlassEffectState& state,
                             IScreenEffectRuntime& runtime);
    [[nodiscard]] static std::uint32_t ApplyGray(
        const ScreenEffectState& state, std::uint32_t input,
        IScreenEffectRuntime& runtime);
    [[nodiscard]] static bool LoadGray(
        ScreenEffectState& state, std::uint32_t incoming_edi,
        std::uint32_t incoming_environment_word,
        IScreenEffectRuntime& runtime);
    [[nodiscard]] static std::uint32_t ApplyHilight(
        const ScreenEffectState& state, std::uint32_t input,
        IScreenEffectRuntime& runtime);
    [[nodiscard]] static bool LoadHilight(
        ScreenEffectState& state, std::uint32_t incoming_edi,
        std::uint32_t incoming_environment_word,
        IScreenEffectRuntime& runtime);
    [[nodiscard]] static std::uint32_t ApplyInvert(
        const ScreenEffectState& state, std::uint32_t input,
        IScreenEffectRuntime& runtime);
    [[nodiscard]] static bool LoadInvert(
        ScreenEffectState& state, std::uint32_t incoming_edi,
        std::uint32_t incoming_environment_word,
        IScreenEffectRuntime& runtime);
    [[nodiscard]] static std::uint32_t ApplyLaplace(
        const ScreenEffectState& state, std::uint32_t input,
        IScreenEffectRuntime& runtime);
    [[nodiscard]] static bool LoadLaplace(
        ScreenEffectState& state, std::uint32_t incoming_edi,
        std::uint32_t incoming_environment_word,
        IScreenEffectRuntime& runtime);
    [[nodiscard]] static std::uint32_t ApplyOldTV(
        const ScreenEffectState& state, std::uint32_t input,
        IScreenEffectRuntime& runtime);
    [[nodiscard]] static bool LoadOldTV(
        ScreenEffectState& state, std::uint32_t incoming_edi,
        std::uint32_t incoming_environment_word,
        IScreenEffectRuntime& runtime);
    [[nodiscard]] static std::uint32_t ApplyPosterize(
        const ScreenEffectState& state, std::uint32_t input,
        IScreenEffectRuntime& runtime);
    [[nodiscard]] static bool LoadPosterize(
        ScreenEffectState& state, std::uint32_t incoming_edi,
        std::uint32_t incoming_environment_word,
        IScreenEffectRuntime& runtime);
    [[nodiscard]] static std::uint32_t ApplyRed(
        const ScreenEffectState& state, std::uint32_t input,
        IScreenEffectRuntime& runtime);
    [[nodiscard]] static bool LoadRed(
        ScreenEffectState& state, std::uint32_t incoming_edi,
        std::uint32_t incoming_environment_word,
        IScreenEffectRuntime& runtime);
    [[nodiscard]] static std::uint32_t ApplySharpenEdges(
        const ScreenEffectState& state, std::uint32_t input,
        IScreenEffectRuntime& runtime);
    [[nodiscard]] static bool LoadSharpenEdges(
        ScreenEffectState& state, std::uint32_t incoming_edi,
        std::uint32_t incoming_environment_word,
        IScreenEffectRuntime& runtime);
    [[nodiscard]] static std::uint32_t ApplyTiling(
        const ScreenEffectState& state, std::uint32_t input,
        IScreenEffectRuntime& runtime);
    [[nodiscard]] static std::uint8_t SetTilingParameter(
        const ScreenEffectState& state, std::uint32_t value,
        IScreenEffectRuntime& runtime);
    [[nodiscard]] static bool LoadTiling(
        ScreenEffectState& state, std::uint32_t incoming_edi,
        std::uint32_t incoming_environment_word,
        IScreenEffectRuntime& runtime);
    [[nodiscard]] static std::uint8_t CleanupSingleScript(
        ScreenEffectState& state, IScreenEffectRuntime& runtime);
    [[nodiscard]] static std::uint32_t& InitializeEnvironmentRecord(
        std::uint32_t& local_word, IScreenEffectRuntime& runtime);

private:
    [[nodiscard]] static std::uint32_t ApplyTechnique(
        const ScreenEffectState& state, std::uint32_t input,
        std::string_view technique, IScreenEffectRuntime& runtime);
    [[nodiscard]] static bool LoadFromPath(
        ScreenEffectState& state, std::uint32_t incoming_edi,
        std::uint32_t incoming_environment_word,
        std::string_view path, std::uint32_t capability,
        std::uint32_t script_configuration,
        IScreenEffectRuntime& runtime);
};

class ScreenEffectControl final {
public:
    [[nodiscard]] static std::uint8_t Enable(ScreenEffectState& state);
    [[nodiscard]] static std::uint8_t Disable(ScreenEffectState& state);
    [[nodiscard]] static std::uint8_t Render(
        const ScreenEffectState& state, IScreenEffectRuntime& runtime);
    [[nodiscard]] static std::uint8_t StopRendering(
        const ScreenEffectState& state, IScreenEffectRuntime& runtime);
};

}  // namespace pal4
