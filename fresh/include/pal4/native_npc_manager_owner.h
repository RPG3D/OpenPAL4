#pragma once

#include "pal4/npc_info_manager.h"
#include "pal4/npc_manager_access.h"
#include "pal4/native_npc_info_reader.h"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <span>
#include <vector>

namespace pal4 {

class IStringBufferStorageRuntime;

// The manager owns the pointer vector; helper services own element behavior.
// Update receives the original secondary-grid context (member +16), not the
// spatial wrapper. Release dispatches virtual slot AC; its concrete helper
// implementation may invoke the deleting destructor and free the element.
class INativeNpcGridElements {
public:
    virtual ~INativeNpcGridElements() = default;
    virtual void Update(std::uint32_t element, std::uint32_t context,
                        std::uint32_t argument) = 0;
    virtual void Release(std::uint32_t element) = 0;
};

class NativeNpcManagerOwner final {
public:
    NativeNpcManagerOwner();
    ~NativeNpcManagerOwner();
    NativeNpcManagerOwner(const NativeNpcManagerOwner&) = delete;
    NativeNpcManagerOwner& operator=(const NativeNpcManagerOwner&) = delete;

    [[nodiscard]] std::uint32_t Construct();
    [[nodiscard]] bool LoadInfo(std::span<const NativeNpcInfoRecord> records);
    [[nodiscard]] std::uint32_t FindInfo(std::string_view name);
    [[nodiscard]] bool RegisterConstructedObject(
        std::string_view name, std::uint32_t object);
    [[nodiscard]] std::uint32_t FindObject(std::string_view name);
    [[nodiscard]] std::uint32_t ObjectCount() const;
    void ForEachObject(const std::function<bool(std::uint32_t)>& visitor);
    [[nodiscard]] std::uint32_t ProcessObjects(
        std::uint32_t camera, IStringBufferStorageRuntime& objects,
        const std::function<void(std::uint32_t, std::uint32_t)>& dispatch);
    [[nodiscard]] std::uint32_t ProcessAlternativeObjects(
        std::uint32_t camera, IStringBufferStorageRuntime& objects,
        const std::function<void(std::uint32_t, std::uint32_t)>& dispatch);
    [[nodiscard]] std::uint32_t ProcessWorldObjects(
        float elapsed, std::uint32_t mode, IStringBufferStorageRuntime& objects,
        const std::function<void(std::uint32_t, float, std::uint32_t)>& dispatch);
    [[nodiscard]] const BinaryXmlConfig* InfoDocument(
        std::uint32_t address) const;
    [[nodiscard]] std::uint32_t InfoCount() const;
    [[nodiscard]] std::uint32_t InitializeSceneGrid();
    // Rebinding requires an empty list. Services outlive registered elements.
    void BindGridElements(std::shared_ptr<INativeNpcGridElements> services);
    [[nodiscard]] bool AddGridElement(std::uint32_t element);
    [[nodiscard]] bool RemoveGridElement(std::uint32_t element);
    [[nodiscard]] std::uint32_t GridWrapperAddress() const noexcept;
    // Original helper input member+16: borrowed static sphere-record pointers.
    [[nodiscard]] std::vector<std::uint32_t> GridObstacleCandidates() const;
    void SetGridUpdating(bool enabled);
    // IDA43C6C0: retire objects using their creating services, retaining the
    // manager, tree sentinels and grid member. Callbacks must not mutate it.
    void CleanupScene(const std::function<void(std::uint32_t)>& notify,
                      const std::function<void(std::uint32_t)>& destroy);
    void Destroy();
    [[nodiscard]] std::uint32_t Address() const noexcept { return object_; }
    [[nodiscard]] const NpcManagerTreeGlobals& TreeGlobals() const noexcept {
        return object_tree_;
    }
    [[nodiscard]] std::uint32_t GridAddress() const noexcept;
    [[nodiscard]] std::size_t OutstandingAllocations() const noexcept;

private:
    [[nodiscard]] std::uint32_t ProcessFrameObjects(
        std::uint32_t camera, IStringBufferStorageRuntime& objects,
        const std::function<void(std::uint32_t, std::uint32_t)>& dispatch,
        bool alternative);
    class Memory;
    class InfoRuntime;
    class ObjectRuntime;
    class GridRuntime;
    class TreeRuntime;
    class CleanupRuntime;

    std::unique_ptr<Memory> memory_;
    std::unique_ptr<InfoRuntime> info_runtime_;
    std::unique_ptr<GridRuntime> grid_runtime_;
    std::unique_ptr<TreeRuntime> tree_runtime_;
    std::unique_ptr<ObjectRuntime> object_runtime_;
    std::unique_ptr<NpcInfoManager> info_manager_;
    NpcInfoSharedTree info_tree_{};
    NpcManagerTreeGlobals object_tree_{};
    std::uint32_t object_ = 0u;
    bool cleaning_scene_ = false;
};

}  // namespace pal4
