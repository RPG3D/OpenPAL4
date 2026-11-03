#include "pal4/rw_texture_dictionary.h"

namespace pal4 {

RwTextureDictionaryState& RwTextureDictionary::Construct(
    RwTextureDictionaryState& state) noexcept {
    state.vtable = kVtableAddress;
    state.dictionary = 0;
    state.enabled = 1;
    return state;
}

void RwTextureDictionary::Destruct(
    RwTextureDictionaryState& state) noexcept {
    state.vtable = kVtableAddress;
}

bool RwTextureDictionary::Initialize(
    RwTextureDictionaryState& state,
    IRwTextureDictionaryRuntime& runtime) {
    (void)Cleanup(state, runtime);
    state.dictionary = runtime.CreateDictionary();
    return state.dictionary != 0;
}

std::uint32_t RwTextureDictionary::DecrementTextureReferences(
    const std::uint32_t texture, IRwTextureDictionaryRuntime& runtime) {
    if (runtime.TextureReferences(texture) > 1)
        runtime.SetTextureReferences(texture, 1);
    return texture;
}

std::uint32_t RwTextureDictionary::Cleanup(
    RwTextureDictionaryState& state,
    IRwTextureDictionaryRuntime& runtime) {
    auto result = state.dictionary;
    if (result != 0) {
        runtime.ForEachTexture(result, [&](std::uint32_t texture) {
            return DecrementTextureReferences(texture, runtime);
        });
        result = runtime.DestroyDictionary(state.dictionary);
        state.dictionary = 0;
    }
    state.enabled = 1;
    return result;
}

}  // namespace pal4
