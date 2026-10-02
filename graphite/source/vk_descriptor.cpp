#include "pch.hpp"
#include "graph/descriptor.hpp"
#include "vk_core.hpp"
#include "vk_error.hpp"
#include "vkit/state/descriptor_set.hpp"
#include "vkit/state/descriptor_pool.hpp"
#include "tkit/utils/storage.hpp"
#include "tkit/container/hive.hpp"

namespace Graph
{
struct Vulkan_DescriptorSet
{
    VKit::DescriptorSet Set{};
    DescriptorSetLayout Layout;
    TKit::Storage<VKit::DescriptorSet::Writer> Writer{};
};
static TKit::Storage<VKit::DescriptorPool> s_DescriptorPool{};
static TKit::Storage<TKit::ArenaHive<VKit::DescriptorSetLayout>> s_Layouts{};
static TKit::Storage<TKit::ArenaHive<Vulkan_DescriptorSet>> s_Sets{};

void Descriptor_Initialize(const u32 maxSets, const TKit::FixedArray<u32, Descriptor_Count> &poolSizes)
{
    s_DescriptorPool.Construct();
    s_Layouts.Construct();
    s_Sets.Construct();

    *s_DescriptorPool = GRAPH_CHECK_VKIT_RESULT(
        VKit::DescriptorPool::Builder(GetDevice())
            .SetMaxSets(maxSets)
            .AddPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, poolSizes[Descriptor_StorageBuffer])
            .AddPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, poolSizes[Descriptor_UniformBuffer])
            .AddPoolSize(VK_DESCRIPTOR_TYPE_SAMPLER, poolSizes[Descriptor_Sampler])
            .AddPoolSize(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, poolSizes[Descriptor_SampledImage])
            .AddPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, poolSizes[Descriptor_CombinedImageSampler])
            .AddPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, poolSizes[Descriptor_StorageImage])
            .SetFlags(VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT)
            .Build());

    GRAPH_CHECK_VKIT_RESULT(s_DescriptorPool->SetName("graph-descriptor-pool"));
}

void Descriptor_Terminate()
{
    GRAPH_CLEANUP_WITH_WARNING(s_Layouts, "DESCRIPTOR", "descriptor layouts");
    s_DescriptorPool->Destroy();

    s_Sets.Destruct();
    s_Layouts.Destruct();
    s_DescriptorPool.Destruct();
}

DescriptorSetLayout DescriptorSetLayout_Create(TKit::Span<const DescriptorBinding> bindings)
{
    VKit::DescriptorSetLayout::Builder builder{GetDevice()};
    for (const DescriptorBinding &b : bindings)
        builder.AddBinding2(b.Binding, ToVulkan(b.Type), ToVulkanShaderStageFlags(b.ShaderStages), b.DescriptorCount,
                            ToVulkanDescriptorBindingFlags(b.Flags));

    const VKit::DescriptorSetLayout layout = GRAPH_CHECK_VKIT_RESULT(builder.Build());
    return Handle_Create(Handle_DescriptorSetLayout, s_Layouts->Insert(layout));
}

void DescriptorSetLayout_Destroy(const DescriptorSetLayout layout)
{
    GRAPH_CHECK_HANDLE(layout, Handle_DescriptorSetLayout);
    GRAPH_DESTROY_FUNCTION_BODY(s_Layouts, layout);
}

void DescriptorSetLayout_SetName(const DescriptorSetLayout layout, const char *name)
{
    GRAPH_CHECK_HANDLE(layout, Handle_DescriptorSetLayout);
    GRAPH_CHECK_VKIT_RESULT(s_Layouts->At(Handle_GetId(layout)).SetName(name));
}

bool DescriptorSetLayout_IsHandleValid(const DescriptorSetLayout layout)
{
    GRAPH_IS_HANDLE_VALID_FUNCTION_BODY(s_Layouts, layout, Handle_DescriptorSetLayout);
}

DescriptorSet DescriptorSet_Create(const DescriptorSetLayout layout)
{
    GRAPH_CHECK_HANDLE(layout, Handle_DescriptorSetLayout);

    const VKit::DescriptorSet set =
        GRAPH_CHECK_VKIT_RESULT(s_DescriptorPool->Allocate(s_Layouts->At(Handle_GetId(layout))));
    return Handle_Create(Handle_DescriptorSet, s_Sets->Insert(set, layout));
}

void DescriptorSet_Destroy(const DescriptorSet set)
{
    GRAPH_CHECK_HANDLE(set, Handle_DescriptorSet);
    const VkDescriptorSet handle = s_Sets->At(Handle_GetId(set)).Set;
    GRAPH_CHECK_VKIT_RESULT(s_DescriptorPool->Deallocate(handle));
}

void DescriptorSet_BeginRecordWrite(const DescriptorSet set, DescriptorSetLayout layout)
{
    GRAPH_CHECK_HANDLE(set, Handle_DescriptorSet);

    Vulkan_DescriptorSet &vset = s_Sets->At(Handle_GetId(set));
    if (layout == NullHandle)
        layout = vset.Layout;

    GRAPH_CHECK_HANDLE(layout, Handle_DescriptorSetLayout);
    vset.Writer.Construct(GetDevice(), &s_Layouts->At(Handle_GetId(layout)));
}
void DescriptorSet_RecordWrite(const DescriptorSet set, const u32 binding,
                               const TKit::Span<const DescriptorBufferInfo> bufferInfo, const u32 elementOffset)
{
    GRAPH_CHECK_HANDLE(set, Handle_DescriptorSet);

    TKit::StackArray<VkDescriptorBufferInfo> infos{};
    infos.Reserve(bufferInfo.GetSize());
    for (const DescriptorBufferInfo &info : bufferInfo)
        infos.Append(GetBuffer(info.Handle), info.Offset, info.Size);

    s_Sets->At(Handle_GetId(set)).Writer->WriteBuffer(binding, infos, elementOffset);
}
void DescriptorSet_RecordWrite(const DescriptorSet set, const u32 binding,
                               const TKit::Span<const DescriptorImageInfo> imageInfo, const u32 elementOffset)
{
    GRAPH_CHECK_HANDLE(set, Handle_DescriptorSet);

    TKit::StackArray<VkDescriptorImageInfo> infos{};
    infos.Reserve(imageInfo.GetSize());
    for (const DescriptorImageInfo &info : imageInfo)
        infos.Append(info.CombinedSampler == NullHandle ? VK_NULL_HANDLE : GetSampler(info.CombinedSampler),
                     GetImageView(info.Handle), ToVulkan(info.Layout));

    s_Sets->At(Handle_GetId(set)).Writer->WriteImage(binding, infos, elementOffset);
}
void DescriptorSet_RecordWrite(const DescriptorSet set, const u32 binding, const TKit::Span<const Sampler> samplers,
                               const u32 elementOffset)
{
    GRAPH_CHECK_HANDLE(set, Handle_DescriptorSet);

    TKit::StackArray<VkDescriptorImageInfo> infos{};
    infos.Reserve(samplers.GetSize());
    for (const Sampler smp : samplers)
        infos.Append(GetSampler(smp), VK_NULL_HANDLE, VK_IMAGE_LAYOUT_UNDEFINED);

    s_Sets->At(Handle_GetId(set)).Writer->WriteImage(binding, infos, elementOffset);
}
void DescriptorSet_EndRecordWrite(const DescriptorSet set)
{
    GRAPH_CHECK_HANDLE(set, Handle_DescriptorSet);
    Vulkan_DescriptorSet &vset = s_Sets->At(Handle_GetId(set));
    vset.Writer->Overwrite(vset.Set);
    vset.Writer.Destruct();
}

void DescriptorSet_SetName(const DescriptorSet set, const char *name)
{
    GRAPH_CHECK_HANDLE(set, Handle_DescriptorSet);
    GRAPH_CHECK_VKIT_RESULT(s_Sets->At(Handle_GetId(set)).Set.SetName(name));
}
bool DescriptorSet_IsHandleValid(const DescriptorSet set)
{
    GRAPH_IS_HANDLE_VALID_FUNCTION_BODY(s_Sets, set, Handle_DescriptorSet);
}

VKit::DescriptorSetLayout &GetDescriptorSetLayout(const DescriptorSetLayout layout)
{
    GRAPH_CHECK_HANDLE(layout, Handle_DescriptorSetLayout);
    return s_Layouts->At(Handle_GetId(layout));
}
VKit::DescriptorSet &GetDescriptorSet(const DescriptorSet set)
{
    GRAPH_CHECK_HANDLE(set, Handle_DescriptorSetLayout);
    return s_Sets->At(Handle_GetId(set)).Set;
}

} // namespace Graph
