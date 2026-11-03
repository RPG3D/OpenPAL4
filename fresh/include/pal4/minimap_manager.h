#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "pal4/minimap_collection.h"

namespace pal4 {

struct MinimapManagerAccessState {
    std::uint8_t guard{};
    std::uint32_t instance{};
};

class IMinimapManagerAccessRuntime {
public:
    virtual ~IMinimapManagerAccessRuntime() = default;
    [[nodiscard]] virtual std::uint32_t GetSingletonManager() = 0;
    [[nodiscard]] virtual std::uint32_t Register(
        std::uint32_t manager, std::string_view name,
        std::uint32_t id, std::uint32_t factory,
        std::uint32_t destroy) = 0;
};

class MinimapManagerAccess final {
public:
    static constexpr std::uint32_t kGuardAddress = 0x8e3f70u;
    static constexpr std::uint32_t kInstanceAddress = 0x8e3f6cu;
    static constexpr std::uint32_t kFactoryAddress = 0x40dc70u;
    static constexpr std::uint32_t kDestroyAddress = 0x40dcd0u;

    [[nodiscard]] static std::uint32_t Get(
        MinimapManagerAccessState& state,
        IMinimapManagerAccessRuntime& runtime);
};

class MinimapManagerObjectState final {
public:
    static constexpr std::uint32_t kBytes = 0x58u;
    [[nodiscard]] std::array<std::uint8_t, kBytes>& Bytes() noexcept {
        return bytes_;
    }
    [[nodiscard]] const std::array<std::uint8_t, kBytes>& Bytes()
        const noexcept { return bytes_; }
    [[nodiscard]] std::uint32_t ReadDword(std::uint32_t offset) const;
    void WriteDword(std::uint32_t offset, std::uint32_t value);

private:
    std::array<std::uint8_t, kBytes> bytes_{};
};

class IMinimapManagerObjectRuntime {
public:
    virtual ~IMinimapManagerObjectRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t size) = 0;
    [[nodiscard]] virtual MinimapManagerObjectState& Access(
        std::uint32_t address) = 0;
    virtual void BaseConstruct(std::uint32_t address) = 0;
    virtual void CleanupCollection(std::uint32_t address) = 0;
    virtual void Free(std::uint32_t address) = 0;
    virtual void ResetTexture(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t GridPixelBuffer(
        std::uint32_t grid) = 0;
    [[nodiscard]] virtual std::uint32_t InitializeTexture(
        std::uint32_t address,
        const std::array<std::uint32_t, 4>& region,
        std::uint32_t grid, bool mode) = 0;
    [[nodiscard]] virtual std::uint32_t ReadDword(
        std::uint32_t address) = 0;
    virtual void CleanupGrid(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t CleanupObjects(
        std::uint32_t address) = 0;
    virtual void LogInvalidGridIndex(std::uint32_t index) = 0;
    virtual void InitializeDialogSystem(std::uint32_t manager) = 0;
};

class IMinimapManagerDrawRuntime {
public:
    virtual ~IMinimapManagerDrawRuntime() = default;
    [[nodiscard]] virtual MinimapManagerObjectState& Access(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t TextureField(
        std::uint32_t texture, std::uint32_t offset) = 0;
    virtual void CommitTextureRaster(std::uint32_t texture) = 0;
    virtual void DrawFilledRectangle(
        std::uint32_t context, const std::array<std::uint32_t, 4>& region,
        const std::array<std::uint8_t, 3>& rgb,
        std::uint8_t alpha) = 0;
    virtual void DrawTexturedRectangle(
        std::uint32_t context, const std::array<std::uint32_t, 4>& region,
        std::uint32_t texture, const std::array<std::uint8_t, 3>& rgb,
        std::uint8_t alpha,
        bool alpha_mode) = 0;
    [[nodiscard]] virtual std::uint32_t DrawMinimapTextures(
        std::uint32_t manager, std::uint32_t context,
        std::uint32_t x, std::uint32_t y,
        std::uint32_t texture_region_address) = 0;
    [[nodiscard]] virtual std::uint32_t FindObjectAt(
        std::uint32_t x, std::uint32_t y,
        std::uint32_t context) = 0;
};

class IMinimapManagerUpdateRuntime {
public:
    virtual ~IMinimapManagerUpdateRuntime() = default;
    [[nodiscard]] virtual MinimapManagerObjectState& Access(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t GridPixelBuffer(
        std::uint32_t grid) = 0;
    [[nodiscard]] virtual std::array<float, 2> WorldToGrid(
        std::uint32_t grid, const std::array<float, 3>& world) = 0;
    [[nodiscard]] virtual std::int32_t GridWidth(std::uint32_t grid) = 0;
    [[nodiscard]] virtual std::int32_t GridHeight(std::uint32_t grid) = 0;
    [[nodiscard]] virtual std::int32_t FloatToInt(float value) = 0;
    virtual void SetCoordinates(std::uint32_t coordinate_object,
                                std::int32_t x, std::int32_t y) = 0;
    virtual void CopyThreeDwords(
        std::uint32_t coordinate_object,
        const std::array<std::uint32_t, 3>& data) = 0;
    [[nodiscard]] virtual bool ApplyRadius(
        std::uint32_t grid, std::int32_t x, std::int32_t y) = 0;
    [[nodiscard]] virtual bool UpdateTexturePosition(
        std::uint32_t texture, std::int32_t x, std::int32_t y) = 0;
    virtual void RefreshTexture(std::uint32_t texture) = 0;
};

class IMinimapManagerScriptRuntime {
public:
    virtual ~IMinimapManagerScriptRuntime() = default;
    [[nodiscard]] virtual MinimapManagerObjectState& Access(
        std::uint32_t address) = 0;
    virtual void Reset(std::uint32_t manager) = 0;
    [[nodiscard]] virtual std::vector<std::uint32_t> LoadLayerRecords(
        const std::string& folder, const std::string& scene,
        std::uint8_t incoming_header_byte) = 0;
    virtual void WriteListWord(std::uint32_t address,
                               std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t AllocateList(
        std::uint32_t bytes) = 0;
    virtual void FreeList(std::uint32_t address) = 0;
    virtual void SelectTexture(std::uint32_t manager,
                               std::uint32_t index) = 0;
};

class IMinimapManagerSnapshotRuntime : public IMinimapCollectionRuntime {
public:
    [[nodiscard]] virtual MinimapManagerObjectState& AccessManager(
        std::uint32_t address) = 0;
    virtual void EncodeGridMask(std::uint32_t grid,
                                std::uint32_t mask_record) = 0;
};

class IMinimapManagerRestoreRuntime {
public:
    virtual ~IMinimapManagerRestoreRuntime() = default;
    [[nodiscard]] virtual MinimapManagerObjectState& AccessManager(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t ReadDword(
        std::uint32_t address) = 0;
    virtual void ApplyGridMask(std::uint32_t grid,
                               std::uint32_t mask_record) = 0;
    virtual void SelectTexture(std::uint32_t manager,
                               std::uint32_t index) = 0;
};

class MinimapManagerDrawStub final : public IMinimapManagerDrawRuntime {
public:
    [[nodiscard]] MinimapManagerObjectState& Access(
        std::uint32_t) override;
    [[nodiscard]] std::uint32_t TextureField(
        std::uint32_t, std::uint32_t) override;
    void CommitTextureRaster(std::uint32_t) override;
    void DrawFilledRectangle(
        std::uint32_t, const std::array<std::uint32_t, 4>&,
        const std::array<std::uint8_t, 3>&, std::uint8_t) override;
    void DrawTexturedRectangle(
        std::uint32_t, const std::array<std::uint32_t, 4>&,
        std::uint32_t, const std::array<std::uint8_t, 3>&,
        std::uint8_t, bool) override;
    [[nodiscard]] std::uint32_t DrawMinimapTextures(
        std::uint32_t, std::uint32_t, std::uint32_t,
        std::uint32_t, std::uint32_t) override;
    [[nodiscard]] std::uint32_t FindObjectAt(
        std::uint32_t, std::uint32_t, std::uint32_t) override;

private:
    MinimapManagerObjectState state_{};
};

class MinimapManagerObject final {
public:
    [[nodiscard]] static std::uint32_t Create(
        std::uint8_t incoming_stack_byte,
        IMinimapManagerObjectRuntime& runtime);
    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t address, std::uint8_t incoming_stack_byte,
        IMinimapManagerObjectRuntime& runtime);
    static void Destruct(std::uint32_t address,
                         IMinimapManagerObjectRuntime& runtime);
    static void Destroy(std::uint32_t address,
                        IMinimapManagerObjectRuntime& runtime);
    [[nodiscard]] static std::uint32_t Reset(
        std::uint32_t address, IMinimapManagerObjectRuntime& runtime);
    static void SetupTexture(
        std::uint32_t address, const std::array<std::uint32_t, 4>& region,
        IMinimapManagerObjectRuntime& runtime);
    static void SetTextureByIndex(
        std::uint32_t address, std::uint32_t index,
        IMinimapManagerObjectRuntime& runtime);
    [[nodiscard]] static std::uint32_t CurrentGridIndex(
        std::uint32_t address, IMinimapManagerObjectRuntime& runtime);
    [[nodiscard]] static std::uint32_t Draw(
        std::uint32_t address, std::uint32_t context,
        IMinimapManagerDrawRuntime& runtime);
    [[nodiscard]] static std::uint32_t FindObjectWithOffset(
        std::uint32_t address, std::uint32_t x, std::uint32_t y,
        std::uint32_t context, IMinimapManagerDrawRuntime& runtime);
    static void UpdateGridDisplay(
        std::uint32_t address, const std::array<float, 3>& world,
        const std::array<std::uint32_t, 3>& data,
        IMinimapManagerUpdateRuntime& runtime);
    [[nodiscard]] static bool LoadScriptData(
        std::uint32_t address, const std::string& folder,
        const std::string& scene, std::uint8_t incoming_header_byte,
        IMinimapManagerScriptRuntime& runtime);
    [[nodiscard]] static const std::array<float, 3>* ConvertGridCoordinates(
        std::uint32_t address, const std::array<float, 3>& world,
        std::array<float, 2>& grid_coordinates,
        IMinimapManagerUpdateRuntime& runtime);
    [[nodiscard]] static std::uint32_t CaptureGridData(
        std::uint32_t address, std::uint8_t collection_stack_byte,
        std::uint8_t mask_stack_byte,
        IMinimapManagerSnapshotRuntime& runtime);
    static void RestoreGridData(std::uint32_t address,
                                std::uint32_t snapshot,
                                IMinimapManagerRestoreRuntime& runtime);
};

}  // namespace pal4
