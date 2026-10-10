#pragma once

#include "graph/resources.hpp"
#include "tkit/container/span.hpp"

namespace Graph
{
struct DescriptorBinding
{
    u32 Binding;
    u32 DescriptorCount;
    DescriptorType Type;
    ShaderStageFlags ShaderStages;
    DescriptorBindingFlags Flags;

    static constexpr DescriptorBinding Create(const u32 binding, const DescriptorType type,
                                              const ShaderStageFlags stages, const u32 count = 1,
                                              const DescriptorBindingFlags flags = 0)
    {
        return {binding, count, type, stages, flags};
    }
};

struct DescriptorBufferInfo
{
    Buffer Buffer;
    usz Offset;
    usz Size;

    static constexpr DescriptorBufferInfo Create(const Graph::Buffer handle, const usz size, const usz offset = 0)
    {
        return {handle, offset, size};
    }
    static constexpr DescriptorBufferInfo Create(const Graph::Buffer handle)
    {
        return {handle, 0, Buffer_GetSize(handle)};
    }
};

struct DescriptorImageInfo
{
    ImageView View;
    Sampler CombinedSampler;
    ImageLayout Layout;

    static constexpr DescriptorImageInfo Create(const ImageView handle, const ImageLayout layout,
                                                const Sampler sampler = NullHandle)
    {
        return {handle, sampler, layout};
    }
};

DescriptorSetLayout DescriptorSetLayout_Create(TKit::Span<const DescriptorBinding> bindings);
void DescriptorSetLayout_Destroy(DescriptorSetLayout layout);

void DescriptorSetLayout_SetName(DescriptorSetLayout layout, const char *name);
bool DescriptorSetLayout_IsHandleValid(DescriptorSetLayout layout);

DescriptorSet DescriptorSet_Create(DescriptorSetLayout layout);
void DescriptorSet_Destroy(DescriptorSet set);

void DescriptorSet_BeginRecordWrite(DescriptorSet set, DescriptorSetLayout layout = NullHandle);
void DescriptorSet_RecordWrite(DescriptorSet set, u32 binding, TKit::Span<const DescriptorBufferInfo> bufferInfo,
                               u32 dstElement = 0);
void DescriptorSet_RecordWrite(DescriptorSet set, u32 binding, TKit::Span<const DescriptorImageInfo> imageInfo,
                               u32 dstElement = 0);
void DescriptorSet_RecordWrite(DescriptorSet set, u32 binding, TKit::Span<const Sampler> samplers, u32 dstElement = 0);
void DescriptorSet_EndRecordWrite(DescriptorSet set);

void DescriptorSet_Write(const DescriptorSet set, u32 binding, const TKit::Span<const DescriptorBufferInfo> bufferInfo,
                         const u32 dstElement = 0, const DescriptorSetLayout layout = NullHandle)
{
    DescriptorSet_BeginRecordWrite(set, layout);
    DescriptorSet_RecordWrite(set, binding, bufferInfo, dstElement);
    DescriptorSet_EndRecordWrite(set);
}

void DescriptorSet_Write(const DescriptorSet set, const u32 binding,
                         const TKit::Span<const DescriptorImageInfo> imageInfo, const u32 dstElement = 0,
                         const DescriptorSetLayout layout = NullHandle)
{
    DescriptorSet_BeginRecordWrite(set, layout);
    DescriptorSet_RecordWrite(set, binding, imageInfo, dstElement);
    DescriptorSet_EndRecordWrite(set);
}

void DescriptorSet_Write(const DescriptorSet set, const u32 binding, const TKit::Span<const Sampler> samplers,
                         const u32 dstElement = 0, const DescriptorSetLayout layout = NullHandle)
{
    DescriptorSet_BeginRecordWrite(set, layout);
    DescriptorSet_RecordWrite(set, binding, samplers, dstElement);
    DescriptorSet_EndRecordWrite(set);
}

void DescriptorSet_SetName(DescriptorSet set, const char *name);
bool DescriptorSet_IsHandleValid(DescriptorSet set);
} // namespace Graph
