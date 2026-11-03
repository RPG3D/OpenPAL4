#include "angelscript.h"

#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::fprintf(stderr, "%s\n", message);
        std::exit(EXIT_FAILURE);
    }
}

class MemoryStream final : public asIBinaryStream {
public:
    void Read(void* destination, int size) override {
        Check(size >= 0 && position_ + static_cast<std::size_t>(size) <= bytes_.size(),
              "AngelScript read beyond saved bytecode");
        std::memcpy(destination, bytes_.data() + position_, size);
        position_ += static_cast<std::size_t>(size);
    }

    void Write(void* source, int size) override {
        Check(size >= 0, "AngelScript negative write size");
        const auto* first = static_cast<const std::byte*>(source);
        bytes_.insert(bytes_.end(), first, first + size);
    }

    void Rewind() { position_ = 0; }
    bool AtEnd() const { return position_ == bytes_.size(); }
    bool Empty() const { return bytes_.empty(); }

private:
    std::vector<std::byte> bytes_;
    std::size_t position_ = 0;
};

}  // namespace

int main() {
    static_assert(sizeof(void*) == 4);
    Check(ANGELSCRIPT_VERSION == 20200, "wrong AngelScript SDK version");

    asIScriptEngine* compiler = asCreateScriptEngine(ANGELSCRIPT_VERSION);
    Check(compiler != nullptr, "create AngelScript compiler engine");
    constexpr char kSource[] = "int Answer() { return 42; }";
    Check(compiler->AddScriptSection("roundtrip", "roundtrip.cs", kSource,
                                     sizeof(kSource) - 1) >= 0,
          "add script section");
    Check(compiler->Build("roundtrip") >= 0, "build script module");

    MemoryStream stream;
    Check(compiler->SaveByteCode("roundtrip", &stream) >= 0 && !stream.Empty(),
          "save AngelScript bytecode");
    Check(compiler->Release() == 0, "release compiler engine");

    asIScriptEngine* runtime = asCreateScriptEngine(ANGELSCRIPT_VERSION);
    Check(runtime != nullptr, "create AngelScript runtime engine");
    stream.Rewind();
    Check(runtime->LoadByteCode("roundtrip", &stream) >= 0 && stream.AtEnd(),
          "restore AngelScript bytecode");
    const int function = runtime->GetFunctionIDByName("roundtrip", "Answer");
    Check(function >= 0, "resolve restored function");

    asIScriptContext* context = nullptr;
    Check(runtime->CreateContext(&context) >= 0 && context != nullptr,
          "create AngelScript context");
    Check(context->Prepare(function) >= 0 && context->Execute() == asEXECUTION_FINISHED &&
              context->GetReturnDWord() == 42,
          "execute restored bytecode");
    Check(context->Release() == 0, "release AngelScript context");
    Check(runtime->Release() == 0, "release AngelScript runtime engine");

    return EXIT_SUCCESS;
}
