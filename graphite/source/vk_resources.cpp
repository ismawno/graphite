#include "pch.hpp"
#include "vk_core.hpp"
#include "graph/resources.hpp"
#include "tkit/utils/storage.hpp"
#include "tkit/container/hive.hpp"

namespace Graph
{
static TKit::Storage<TKit::ArenaHive<VKit::DeviceBuffer>> s_Buffers{};
static TKit::Storage<TKit::ArenaHive<VKit::DeviceImage>> s_Images{};
static TKit::Storage<TKit::ArenaHive<VKit::Sampler>> s_Samplers{};

void Resources_Initialize(const u32 maxBuffers, const u32 maxImages, const u32 maxSamplers)
{
    s_Buffers.Construct();
    s_Images.Construct();
    s_Samplers.Construct();

    s_Buffers->Reserve(maxBuffers);
    s_Images->Reserve(maxImages);
    s_Samplers->Reserve(maxSamplers);
}

void Resources_Terminate()
{
    GRAPH_CLEANUP_WITH_WARNING_ACCESSOR(s_Buffers, "RESOURCES", "buffers", Destroy());
    GRAPH_CLEANUP_WITH_WARNING_ACCESSOR(s_Images, "RESOURCES", "images", Destroy());
    GRAPH_CLEANUP_WITH_WARNING_ACCESSOR(s_Samplers, "RESOURCES", "samplers", Destroy());

    s_Buffers.Destruct();
    s_Images.Destruct();
    s_Samplers.Destruct();
}

bool Buffer_IsHandleValid(const Buffer buffer)
{
    GRAPH_IS_HANDLE_VALID_FUNCTION_BODY(s_Buffers, buffer, Handle_Buffer);
}

VKit::DeviceBuffer &GetBuffer(const Buffer buffer)
{
    GRAPH_CHECK_HANDLE(buffer, Handle_Buffer);

    return s_Buffers->At(Handle_GetId(buffer));
}
} // namespace Graph
