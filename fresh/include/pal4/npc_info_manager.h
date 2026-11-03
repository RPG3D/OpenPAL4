#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <string_view>

namespace pal4 {

class INpcInfoRuntime {
public:
    virtual ~INpcInfoRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    virtual void Free(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    virtual void SetStringFlag(std::uint32_t address, std::uint8_t value) = 0;
    virtual void InitializeString(std::uint32_t address) = 0;
    virtual void AssignString(std::uint32_t address, std::string_view value) = 0;
    [[nodiscard]] virtual std::string ReadStringValue(std::uint32_t address) = 0;
    virtual void CleanupString(std::uint32_t address) = 0;
    virtual void ReadStreamString(std::uint32_t stream, std::uint32_t address) = 0;
    virtual void ReadStreamData(std::uint32_t stream, std::uint32_t address,
                                std::uint32_t bytes) = 0;
    [[nodiscard]] virtual std::uint32_t ReadStreamWord(std::uint32_t stream) = 0;
    [[nodiscard]] virtual std::uint32_t StreamKind(std::uint32_t stream) = 0;
    [[nodiscard]] virtual std::uint32_t StreamLocale(std::uint32_t stream) = 0;
    [[nodiscard]] virtual std::uint32_t CreateLocaleFacet(std::uint32_t locale) = 0;
    virtual void DestroyLocaleFacet(std::uint32_t facet) = 0;
    virtual void CheckXml(std::uint32_t locale, std::uint32_t document) = 0;
    [[nodiscard]] virtual std::uint32_t InitializeXml(
        std::uint32_t storage, std::uint32_t kind) = 0;
    virtual void DestroyXml(std::uint32_t document) = 0;
    [[nodiscard]] virtual std::uint32_t OpenNpcInfo(
        std::string_view directory, std::string_view name) = 0;
    [[nodiscard]] virtual std::uint32_t OpenMonsterInfo(
        std::string_view directory, std::string_view name) = 0;
    virtual void CloseStream(std::uint32_t stream) = 0;
};

class NpcInfoRuntimeStub : public INpcInfoRuntime {
public:
    std::uint32_t Allocate(std::uint32_t) override { return 0; }
    void Free(std::uint32_t) override {}
    std::uint32_t ReadWord(std::uint32_t) override { return 0; }
    void WriteWord(std::uint32_t, std::uint32_t) override {}
    std::uint8_t ReadByte(std::uint32_t) override { return 0; }
    void WriteByte(std::uint32_t, std::uint8_t) override {}
    void SetStringFlag(std::uint32_t, std::uint8_t) override {}
    void InitializeString(std::uint32_t) override {}
    void AssignString(std::uint32_t, std::string_view) override {}
    std::string ReadStringValue(std::uint32_t) override { return {}; }
    void CleanupString(std::uint32_t) override {}
    void ReadStreamString(std::uint32_t, std::uint32_t) override {}
    void ReadStreamData(std::uint32_t, std::uint32_t, std::uint32_t) override {}
    std::uint32_t ReadStreamWord(std::uint32_t) override { return 0; }
    std::uint32_t StreamKind(std::uint32_t) override { return 0; }
    std::uint32_t StreamLocale(std::uint32_t) override { return 0; }
    std::uint32_t CreateLocaleFacet(std::uint32_t locale) override {
        return locale;
    }
    void DestroyLocaleFacet(std::uint32_t) override {}
    void CheckXml(std::uint32_t, std::uint32_t) override {}
    std::uint32_t InitializeXml(std::uint32_t storage,
                                std::uint32_t) override { return storage; }
    void DestroyXml(std::uint32_t) override {}
    std::uint32_t OpenNpcInfo(std::string_view, std::string_view) override {
        return 0;
    }
    std::uint32_t OpenMonsterInfo(std::string_view, std::string_view) override {
        return 0;
    }
    void CloseStream(std::uint32_t) override {}
};

class NpcInfoRecord final {
public:
    static constexpr std::uint32_t kBytes = 0x54;
    static void Initialize(std::uint32_t record, INpcInfoRuntime& runtime);
    static void Read(std::uint32_t record, std::uint32_t stream,
                     INpcInfoRuntime& runtime);
    static void InitializeDocuments(std::uint32_t record,
                                    INpcInfoRuntime& runtime);
    static void CleanupDocuments(std::uint32_t record,
                                 INpcInfoRuntime& runtime);
    static void CleanupStrings(std::uint32_t record,
                               INpcInfoRuntime& runtime);
    static void Destroy(std::uint32_t record, INpcInfoRuntime& runtime);
};

struct NpcInfoSharedTree {
    std::uint32_t sentinel = 0;
    std::uint32_t instances = 0;
};

class NpcInfoManager final {
public:
    using Visitor = bool (*)(std::uint32_t record, std::uint32_t context);
    NpcInfoManager(std::uint32_t address, NpcInfoSharedTree& shared,
                   INpcInfoRuntime& runtime);
    [[nodiscard]] bool Initialize();
    void Clear();
    void DestroyTreeContainer();
    void Destroy();
    [[nodiscard]] std::uint32_t CreateOrGet(
        std::string_view name, std::string_view resource,
        const std::array<std::uint32_t, 3>& position,
        const std::array<std::uint32_t, 3>& rotation);
    [[nodiscard]] std::uint32_t Find(std::string_view name);
    [[nodiscard]] bool Remove(std::string_view name);
    [[nodiscard]] bool ForEach(Visitor visitor, std::uint32_t context) const;
    [[nodiscard]] bool Load(std::string_view directory, std::string_view name);

private:
    [[nodiscard]] bool Insert(std::string name, std::uint32_t record);
    std::uint32_t address_;
    NpcInfoSharedTree& shared_;
    INpcInfoRuntime& runtime_;
    bool initialized_ = false;
};

}  // namespace pal4
