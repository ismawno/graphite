#include "pch.hpp"
#include "graph/resources.hpp"
#include "vk_core.hpp"
#include "vk_error.hpp"
#ifdef GRAPH_HAS_PLATFORM_BACKEND
#    include "graph/platform.hpp"
#endif
#ifdef GRAPH_PLATFORM_BACKEND_GLFW
#    define GLFW_INCLUDE_VULKAN
#    include "glfw_core.hpp"
#endif
#include "vkit/core/core.hpp"
#include "vkit/device/logical_device.hpp"
#include "tkit/container/stack_array.hpp"

namespace Graph
{
static TKit::Storage<VKit::Instance> s_Instance{};
static TKit::Storage<VKit::PhysicalDevice> s_Physical{};
static TKit::Storage<VKit::LogicalDevice> s_Device{};
static VmaAllocator s_VulkanAllocator = VK_NULL_HANDLE;
static Specs s_Specs{};

#ifdef GRAPH_HAS_PLATFORM_BACKEND
Window s_DummyWindow = NullHandle;

static VkSurfaceKHR createDummySurface()
{
    TKIT_ASSERT(s_DummyWindow == NullHandle, "[GRAPH][CORE] Can only create a single dummy surface at the same time");
    s_DummyWindow = Window_Create({.Title = "Eduardo", .Dimensions = 120, .Flags = 0});

    return GetSurface(s_DummyWindow);
}
static void destroyDummySurface()
{
    TKIT_ASSERT(s_DummyWindow != NullHandle, "[GRAPH][CORE] Can only destroy a dummy surface if one was created");

    Window_Destroy(s_DummyWindow);
}
#endif

static const char *toString(const VkDeviceFaultAddressTypeEXT faultType)
{
    switch (faultType)
    {
    case VK_DEVICE_FAULT_ADDRESS_TYPE_NONE_EXT:
        return "NONE";
    case VK_DEVICE_FAULT_ADDRESS_TYPE_READ_INVALID_EXT:
        return "READ_INVALID";
    case VK_DEVICE_FAULT_ADDRESS_TYPE_WRITE_INVALID_EXT:
        return "WRITE_INVALID";
    case VK_DEVICE_FAULT_ADDRESS_TYPE_EXECUTE_INVALID_EXT:
        return "EXECUTE_INVALID";
    case VK_DEVICE_FAULT_ADDRESS_TYPE_INSTRUCTION_POINTER_UNKNOWN_EXT:
        return "INSTRUCTION_POINTER_UNKNOWN";
    case VK_DEVICE_FAULT_ADDRESS_TYPE_INSTRUCTION_POINTER_INVALID_EXT:
        return "INSTRUCTION_POINTER_INVALID";
    case VK_DEVICE_FAULT_ADDRESS_TYPE_INSTRUCTION_POINTER_FAULT_EXT:
        return "INSTRUCTION_POINTER_FAULT";
    default:
        return "UNKNOWN";
    }
}

static void createInstance()
{
    VKit::Instance::Builder builder{};
    builder.SetApplicationName(s_Specs.ApplicationName)
        .RequestApiVersion(1, 4, 0)
        .RequireApiVersion(1, 2, 0)
        .SetApplicationVersion(1, 2, 0)
        .RequestExtension("VK_KHR_get_physical_device_properties2")
        .RequestExtension("VK_KHR_portability_enumeration");

    const Capabilities caps = s_Specs.EnabledCapabilities;
    if (caps & Capability_Validation)
    {
        TKIT_LOG_INFO("[GRAPH][CORE] Validation layers (VK_LAYER_KHRONOS_validation) have been requested");
        TKIT_LOG_INFO("[GRAPH][CORE] Debug utils extension (VK_EXT_debug_utils) has been requested");
        TKIT_LOG_INFO("[GRAPH][CORE] Device assisted debug feature has been requested");
        TKIT_LOG_INFO("[GRAPH][CORE] Best practices debug feature has been requested");
        TKIT_LOG_INFO("[GRAPH][CORE] Sync validation debug feature has been requested");
        builder.RequestLayer("VK_LAYER_KHRONOS_validation")
            .RequestExtension("VK_EXT_debug_utils")
            .RequestExtension("VK_EXT_validation_features")
            .SetValidationFeature(VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_EXT)
            .SetValidationFeature(VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_RESERVE_BINDING_SLOT_EXT)
            .SetValidationFeature(VK_VALIDATION_FEATURE_ENABLE_BEST_PRACTICES_EXT)
            .SetValidationFeature(VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT);
    }
    if (caps & Capability_DebugPrintf)
    {
        TKIT_LOG_INFO("[GRAPH][CORE] Printf debug feature has been requested");
        builder.SetValidationFeature(VK_VALIDATION_FEATURE_ENABLE_DEBUG_PRINTF_EXT);
    }

#ifndef GRAPH_HAS_PLATFORM_BACKEND
    builder.SetHeadless();
#endif

    *s_Instance = GRAPH_CHECK_RESULT(builder.Build());
    const VKit::Instance::Info &info = s_Instance->GetInfo();
    TKIT_LOG_INFO("[GRAPH][CORE] Created vulkan instance. API version: {}.{}.{}", VKIT_EXPAND_VERSION(info.ApiVersion));

    TKIT_LOG_INFO("[GRAPH][CORE] The following instance layers were enabled");
#ifdef TKIT_ENABLE_INFO_LOGS
    for (const char *layer : info.EnabledLayers)
        TKIT_LOG_INFO("[GRAPH][CORE]     {}", layer);
#endif

    TKIT_LOG_INFO("[GRAPH][CORE] The following instance extensions were enabled");
#ifdef TKIT_ENABLE_INFO_LOGS
    for (const char *ext : info.EnabledExtensions)
        TKIT_LOG_INFO("[GRAPH][CORE]     {}", ext);
#endif

    TKIT_LOG_ERROR_IF((caps & Capability_Validation) && !s_Instance->IsLayerEnabled("VK_LAYER_KHRONOS_validation"),
                      "[GRAPH][CORE] Validation layers (VK_LAYER_KHRONOS_validation) could not be enabled");
    TKIT_LOG_ERROR_IF((caps & Capability_Validation) && !s_Instance->IsExtensionEnabled("VK_EXT_debug_utils"),
                      "[GRAPH][CORE] Debug utils extension (VK_EXT_debug_utils) could not be enabled");
    TKIT_LOG_ERROR_IF((caps & Capability_Validation) && !s_Instance->IsExtensionEnabled("VK_EXT_validation_features"),
                      "[GRAPH][CORE] Validation features extension (VK_EXT_validation_features) could not be enabled");
}

static void createDevice()
{
    TKIT_LOG_INFO("[GRAPH][CORE] Initializing");
    TKIT_LOG_INFO("[GRAPH][CORE] Vulkan headers version: {}.{}.{}", VKIT_EXPAND_VERSION(VK_HEADER_VERSION_COMPLETE));

    VKit::PhysicalDevice::Selector selector(&s_Instance.Get());

    TKIT_COMPILER_WARNING_IGNORE_PUSH()
    TKIT_MSVC_WARNING_IGNORE(4996)
    const char *dev = std::getenv("GRAPH_DEVICE");
    TKIT_COMPILER_WARNING_IGNORE_POP()

    if (dev)
    {
        u32 id = TKIT_U32_MAX;
        const char *begin = dev;
        const char *end = dev + std::strlen(dev);
        const auto [ptr, ec] = std::from_chars(begin, end, id);
        if (ec == std::errc{} && ptr == end)
            selector.SetId(id);
        else
            selector.SetName(dev);
    }
    TKIT_LOG_INFO_IF(
        !dev, "[GRAPH][CORE] The environment variable 'GRAPH_DEVICE' was not set, meaning the physical device will be "
              "chosen automatically. To force a specific option, set such variable with the device name or device ID");

#ifdef GRAPH_HAS_PLATFORM_BACKEND
    const VkSurfaceKHR dummy = createDummySurface();
    selector.SetSurface(dummy);
#endif

    selector.PreferType(VKit::Device_Discrete)
        .AddFlags(VKit::DeviceSelectorFlag_AnyType | VKit::DeviceSelectorFlag_PortabilitySubset |
                  VKit::DeviceSelectorFlag_RequireGraphicsQueue | VKit::DeviceSelectorFlag_RequirePresentQueue |
                  VKit::DeviceSelectorFlag_RequireTransferQueue)
        .RequireExtension("VK_KHR_synchronization2")
        .RequireExtension("VK_KHR_copy_commands2")
        .RequireExtension("VK_KHR_timeline_semaphore")
        .RequireExtension("VK_KHR_dynamic_rendering")
        .RequireApiVersion(1, 2, 0)
        .RequestApiVersion(1, 4, 0);

    const Capabilities caps = s_Specs.EnabledCapabilities;
    if (caps & Capability_BindlessDescriptors)
        selector.RequireExtension("VK_EXT_descriptor_indexing");
    if (caps & Capability_ExtendedDynamicState)
        selector.RequireExtension("VK_EXT_extended_dynamic_state");
    if (caps & Capability_ImageMultiFormat)
        selector.RequireExtension("VK_KHR_image_format_list");

    const bool faultDump = caps & Capability_FaultDump;
    if (faultDump)
        selector.RequestExtension("VK_EXT_device_fault");

    *s_Physical = GRAPH_CHECK_RESULT(selector.Select());

#ifdef GRAPH_HAS_PLATFORM_BACKEND
    destroyDummySurface();
#endif

    TKIT_LOG_INFO("[GRAPH][CORE] Selected vulkan device: {}. API version: {}.{}.{}",
                  s_Physical->GetInfo().Properties.Core.deviceName,
                  VKIT_EXPAND_VERSION(s_Physical->GetInfo().ApiVersion));

    TKIT_LOG_WARNING_IF(!(s_Physical->GetInfo().Flags & VKit::DeviceFlag_Optimal),
                        "[GRAPH][CORE] The device is suitable, but not optimal");

    TKIT_LOG_INFO("[GRAPH][CORE] The following device extensions were enabled");
#ifdef TKIT_ENABLE_INFO_LOGS
    for (const TKit::TierString &ext : s_Physical->GetInfo().EnabledExtensions)
        TKIT_LOG_INFO("[GRAPH][CORE]     {}", ext);
#endif

    TKIT_LOG_ERROR_IF(faultDump && !s_Physical->IsExtensionEnabled("VK_EXT_device_fault"),
                      "[GRAPH][CORE] The device fault extension (VK_EXT_device_fault) could not be enabled");

    TKIT_LOG_INFO_IF(s_Physical->GetInfo().Flags & VKit::DeviceFlag_Optimal, "[GRAPH][CORE] The device is optimal");

    const u32 apiVersion = s_Physical->GetInfo().ApiVersion;

    VkPhysicalDeviceFaultFeaturesEXT faultFeatures{};
    if (faultDump && s_Physical->IsExtensionEnabled("VK_EXT_device_fault"))
    {
        faultFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FAULT_FEATURES_EXT;

        const auto table = GetInstanceTable();
        VkPhysicalDeviceFeatures2KHR features2{};
        features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2_KHR;
        features2.pNext = &faultFeatures;
        table->GetPhysicalDeviceFeatures2KHR(*s_Physical, &features2);

        TKIT_LOG_WARNING_IF(!faultFeatures.deviceFaultVendorBinary,
                            "[GRAPH][CORE] The 'deviceFaultVendorBinary' feature is not supported");
        TKIT_LOG_ERROR_IF(!faultFeatures.deviceFault, "[GRAPH][CORE] The 'deviceFault' feature is not supported. The "
                                                      "extension 'VK_EXT_device_fault' is virtually useless");
        s_Physical->EnableExtensionBoundFeature(&faultFeatures);
    }

    VKit::DeviceFeatures features{};
    // User-controlled
    if (caps & Capability_IndependentBlend)
        features.Core.independentBlend = VK_TRUE;

    if (caps & Capability_MultiDrawIndirect)
    {
        features.Core.drawIndirectFirstInstance = VK_TRUE;
        features.Core.multiDrawIndirect = VK_TRUE;
    }

    if (caps & Capability_ShaderDrawParameters)
        features.Vulkan11.shaderDrawParameters = VK_TRUE;

    features.Vulkan12.timelineSemaphore = VK_TRUE;

    if (caps & Capability_BindlessDescriptors)
    {
        features.Vulkan12.descriptorBindingPartiallyBound = VK_TRUE;
        features.Vulkan12.runtimeDescriptorArray = VK_TRUE;
        features.Vulkan12.descriptorIndexing = VK_TRUE;
        features.Vulkan12.descriptorBindingUpdateUnusedWhilePending = VK_TRUE;
        features.Vulkan12.shaderSampledImageArrayNonUniformIndexing = VK_TRUE;
    }

    VkPhysicalDeviceSynchronization2FeaturesKHR sync2{};
    sync2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES_KHR;
    sync2.synchronization2 = VK_TRUE;

    VkPhysicalDeviceDynamicRenderingFeaturesKHR drendering{};
    drendering.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES_KHR;
    drendering.dynamicRendering = VK_TRUE;

    VkPhysicalDeviceExtendedDynamicStateFeaturesEXT extState{};
    extState.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_FEATURES_EXT;
    if (caps & Capability_ExtendedDynamicState)
    {
        extState.extendedDynamicState = VK_TRUE;
        s_Physical->EnableExtensionBoundFeature(&extState);
    }

    if (apiVersion >= VKIT_API_VERSION_1_3)
    {
        features.Vulkan13.synchronization2 = VK_TRUE;
        features.Vulkan13.dynamicRendering = VK_TRUE;

        TKIT_ASSERT(s_Physical->EnableFeatures(features), "[GRAPH][CORE] Failed to enable requested features");
    }
    else
    {
        s_Physical->EnableExtensionBoundFeature(&sync2);
        s_Physical->EnableExtensionBoundFeature(&drendering);

        TKIT_ASSERT(s_Physical->EnableFeatures(features), "[GRAPH][CORE] Failed to enable requested features");
    }

    VKit::LogicalDevice::Builder devBuild{&s_Instance.Get(), &s_Physical.Get()};
    if (caps & Capability_RequireGraphicsQueue)
        devBuild.RequireQueue(VKit::Queue_Graphics);
    if (caps & Capability_RequireTransferQueue)
        devBuild.RequireQueue(VKit::Queue_Transfer);
    if (caps & Capability_RequireComputeQueue)
        devBuild.RequireQueue(VKit::Queue_Compute);

#ifdef GRAPH_HAS_PLATFORM_BACKEND
    devBuild.RequireQueue(VKit::Queue_Present);
#endif

    *s_Device = GRAPH_CHECK_RESULT(devBuild.Build());
    if (IsDebugUtilsEnabled())
    {
        GRAPH_CHECK_RESULT(s_Device->SetName("graph-device"));
    }
}

static void createVulkanAllocator()
{
    TKIT_LOG_INFO("[GRAPH][CORE] Creating vulkan allocator");
    s_VulkanAllocator = GRAPH_CHECK_RESULT(VKit::CreateAllocator(*s_Device));
}

void Initialize(const Specs &specs)
{
    s_Instance.Construct();
    s_Physical.Construct();
    s_Device.Construct();

    s_Specs = specs;
    VKit::Specs vspecs{};
    vspecs.Allocators.Arena = specs.Allocators.Arena;
    vspecs.Allocators.Stack = specs.Allocators.Stack;
    vspecs.Allocators.Tier = specs.Allocators.Tier;
    vspecs.LoaderPath = specs.LoaderPath;
    GRAPH_CHECK_RESULT(VKit::Initialize(vspecs));

#ifdef GRAPH_HAS_PLATFORM_BACKEND
#    if defined(GRAPH_PLATFORM_BACKEND_GLFW) && GRAPH_GLFW_VERSION_COMBINED >= 3400
    glfwInitVulkanLoader(VKit::Vulkan::vkGetInstanceProcAddr);
#    endif
    Platform_Initialize(specs.TargetPlatform, specs.MaxSurfaces);
    Surface_Initialize(specs.MaxSurfaces);
#endif

    createInstance();
    createDevice();
    createVulkanAllocator();

    Execution_Initialize(specs.MaxCommandPools, specs.MaxCommandBuffers);
    Descriptor_Initialize(specs.MaxDescriptorSets, specs.DescriptorPoolSizes);
    Shader_Initialize(specs.MaxShaders);
#ifdef GRAPH_HAS_SHADER_COMPILATION_BACKEND
    Compilation_Initialize(specs.MaxCompilations);
#endif
#ifdef GRAPH_HAS_SHADER_REFLECTION_BACKEND
    Reflection_Initialize(specs.MaxReflections);
#endif
    Pipeline_Initialize(specs.MaxPipelineLayouts, specs.MaxPipelines);
    Resources_Initialize(specs.MaxBuffers, specs.MaxImages, specs.MaxSamplers, specs.MaxImageViews);
}

void Terminate()
{
    Resources_Terminate();
    Pipeline_Terminate();
#ifdef GRAPH_HAS_SHADER_REFLECTION_BACKEND
    Reflection_Terminate();
#endif
#ifdef GRAPH_HAS_SHADER_COMPILATION_BACKEND
    Compilation_Terminate();
#endif
    Shader_Terminate();
    Descriptor_Terminate();
    Execution_Terminate();

    Surface_Terminate();

#ifdef GRAPH_HAS_PLATFORM_BACKEND
    Platform_Terminate();
#endif

    VKit::DestroyAllocator(s_VulkanAllocator);

    s_Device->Destroy();
    s_Instance->Destroy();

    VKit::Terminate();

    s_Device.Destruct();
    s_Physical.Destruct();
    s_Instance.Destruct();
}

void DeviceWaitIdle()
{
    GRAPH_CHECK_RESULT(s_Device->WaitIdle());
}
bool IsValidationEnabled()
{
    return IsDebugUtilsEnabled();
}
void HandleVulkanResult(const VkResult result)
{
#ifdef TKIT_ENABLE_ERROR_LOGS
    if (result != VK_ERROR_DEVICE_LOST)
        return;
    if (!s_Physical->IsExtensionEnabled("VK_EXT_device_fault"))
    {
        TKIT_LOG_ERROR("[GRAPH][CORE] A device lost error was encountered, but could not study it further because the "
                       "'VK_EXT_device_fault' extension was not enabled");
        return;
    }

    VkDeviceFaultCountsEXT counts{};
    counts.sType = VK_STRUCTURE_TYPE_DEVICE_FAULT_COUNTS_EXT;
    const auto &device = *s_Device;
    const auto table = s_Device->GetInfo().Table;

    GRAPH_CHECK_RESULT(table->GetDeviceFaultInfoEXT(device, &counts, nullptr));

    TKit::StackArray<VkDeviceFaultAddressInfoEXT> addresses{};
    TKit::StackArray<VkDeviceFaultVendorInfoEXT> vendors{};
    TKit::StackArray<std::byte> vendorBinary{};

    addresses.Resize(counts.addressInfoCount);
    vendors.Resize(counts.vendorInfoCount);
    vendorBinary.Resize(u32(counts.vendorBinarySize));

    VkDeviceFaultInfoEXT faultInfo{};
    faultInfo.sType = VK_STRUCTURE_TYPE_DEVICE_FAULT_INFO_EXT;
    faultInfo.pAddressInfos = addresses.GetData();
    faultInfo.pVendorInfos = vendors.GetData();
    faultInfo.pVendorBinaryData = vendorBinary.GetData();

    GRAPH_CHECK_RESULT(table->GetDeviceFaultInfoEXT(device, &counts, &faultInfo));

    TKIT_LOG_ERROR("[GRAPH][CORE] Device fault description: {}", faultInfo.description);

    for (u32 i = 0; i < counts.addressInfoCount; ++i)
    {
        const VkDeviceFaultAddressInfoEXT &info = addresses[i];
        TKIT_LOG_ERROR("[GRAPH][CORE]    Address[{}]: {}", i, toString(info.addressType));
        TKIT_LOG_ERROR("[GRAPH][CORE]        addr={:#18X}", info.reportedAddress);
        TKIT_LOG_ERROR("[GRAPH][CORE]        precision={}", info.addressPrecision);

        if (info.addressPrecision > 1)
        {
            const u64 lo = info.reportedAddress & ~(info.addressPrecision - 1);
            const u64 hi = lo + info.addressPrecision;
            TKIT_LOG_ERROR("        Actual address in range [{:#18X}, {:#18X}]", lo, hi);
        }
    }

    for (u32 i = 0; i < counts.vendorInfoCount; ++i)
    {
        const VkDeviceFaultVendorInfoEXT &info = vendors[i];
        TKIT_LOG_ERROR("[GRAPH][CORE]    Vendor[{}]: {}", i, info.description);
        TKIT_LOG_ERROR("[GRAPH][CORE]        faultcode={:#18X}", info.vendorFaultCode);
        TKIT_LOG_ERROR("[GRAPH][CORE]        faultdata={:#18X}", info.vendorFaultData);
    }
    if (counts.vendorBinarySize == 0)
        return;

    if (vendorBinary.GetSize() < sizeof(VkDeviceFaultVendorBinaryHeaderVersionOneEXT))
    {
        TKIT_LOG_ERROR(
            "[GRAPH][CORE] Cannot retrieve vendor binary data: It is too small to be contained in the vulkan "
            "header version one");
        return;
    }

    const VkDeviceFaultVendorBinaryHeaderVersionOneEXT *header =
        reinterpret_cast<const VkDeviceFaultVendorBinaryHeaderVersionOneEXT *>(vendorBinary.GetData());
#endif

    TKIT_LOG_ERROR("[GRAPH][CORE] Vendor binary ({:L} bytes)", vendorBinary.GetSize());
    TKIT_LOG_ERROR("[GRAPH][CORE]    vendor={:#010x}", header->vendorID);
    TKIT_LOG_ERROR("[GRAPH][CORE]    device={:#010x}", header->deviceID);

    namespace fs = std::filesystem;
    const auto path = s_Specs.DumpPath ? fs::path(s_Specs.DumpPath) : fs::temp_directory_path();
    std::ofstream f{path, std::ios::binary};
    if (!f)
    {
        TKIT_LOG_ERROR("[GRAPH][CORE] Failed to open file at '{}' to write device fault dump", path.string());
        return;
    }
    f.write(reinterpret_cast<const char *>(vendorBinary.GetData()), std::streamsize(vendorBinary.GetSize()));

    TKIT_LOG_ERROR("[GRAPH][CORE] Wrote crash dump to '{}'", path.string());
}

VKit::Instance &GetInstance()
{
    return *s_Instance;
}
VKit::PhysicalDevice &GetPhysical()
{
    return *s_Physical;
}
VKit::LogicalDevice &GetDevice()
{
    return *s_Device;
}
VmaAllocator GetAllocator()
{
    return s_VulkanAllocator;
}

const VKit::Vulkan::InstanceTable *GetInstanceTable()
{
    return s_Instance->GetInfo().Table;
}
const VKit::Vulkan::DeviceTable *GetDeviceTable()
{
    return s_Device->GetInfo().Table;
}
bool IsDebugUtilsEnabled()
{
    return s_Instance->IsExtensionEnabled("VK_EXT_debug_utils");
}

VkFormat ToVulkan(const Format format)
{
    switch (format)
    {
    case Format_Undefined:
        return VK_FORMAT_UNDEFINED;

    case Format_R8_UNORM:
        return VK_FORMAT_R8_UNORM;
    case Format_R8_SNORM:
        return VK_FORMAT_R8_SNORM;
    case Format_R8_UINT:
        return VK_FORMAT_R8_UINT;
    case Format_R8_SINT:
        return VK_FORMAT_R8_SINT;
    case Format_R8_SRGB:
        return VK_FORMAT_R8_SRGB;

    case Format_R8G8_UNORM:
        return VK_FORMAT_R8G8_UNORM;
    case Format_R8G8_SNORM:
        return VK_FORMAT_R8G8_SNORM;
    case Format_R8G8_UINT:
        return VK_FORMAT_R8G8_UINT;
    case Format_R8G8_SINT:
        return VK_FORMAT_R8G8_SINT;
    case Format_R8G8_SRGB:
        return VK_FORMAT_R8G8_SRGB;

    case Format_R8G8B8_UNORM:
        return VK_FORMAT_R8G8B8_UNORM;
    case Format_R8G8B8_SNORM:
        return VK_FORMAT_R8G8B8_SNORM;
    case Format_R8G8B8_UINT:
        return VK_FORMAT_R8G8B8_UINT;
    case Format_R8G8B8_SINT:
        return VK_FORMAT_R8G8B8_SINT;
    case Format_R8G8B8_SRGB:
        return VK_FORMAT_R8G8B8_SRGB;

    case Format_R8G8B8A8_UNORM:
        return VK_FORMAT_R8G8B8A8_UNORM;
    case Format_R8G8B8A8_SNORM:
        return VK_FORMAT_R8G8B8A8_SNORM;
    case Format_R8G8B8A8_UINT:
        return VK_FORMAT_R8G8B8A8_UINT;
    case Format_R8G8B8A8_SINT:
        return VK_FORMAT_R8G8B8A8_SINT;
    case Format_R8G8B8A8_SRGB:
        return VK_FORMAT_R8G8B8A8_SRGB;

    case Format_B8G8R8A8_UNORM:
        return VK_FORMAT_B8G8R8A8_UNORM;
    case Format_B8G8R8A8_SNORM:
        return VK_FORMAT_B8G8R8A8_SNORM;
    case Format_B8G8R8A8_UINT:
        return VK_FORMAT_B8G8R8A8_UINT;
    case Format_B8G8R8A8_SINT:
        return VK_FORMAT_B8G8R8A8_SINT;
    case Format_B8G8R8A8_SRGB:
        return VK_FORMAT_B8G8R8A8_SRGB;

    case Format_R16_UNORM:
        return VK_FORMAT_R16_UNORM;
    case Format_R16_SNORM:
        return VK_FORMAT_R16_SNORM;
    case Format_R16_UINT:
        return VK_FORMAT_R16_UINT;
    case Format_R16_SINT:
        return VK_FORMAT_R16_SINT;
    case Format_R16_SFLOAT:
        return VK_FORMAT_R16_SFLOAT;

    case Format_R16G16_UNORM:
        return VK_FORMAT_R16G16_UNORM;
    case Format_R16G16_SNORM:
        return VK_FORMAT_R16G16_SNORM;
    case Format_R16G16_UINT:
        return VK_FORMAT_R16G16_UINT;
    case Format_R16G16_SINT:
        return VK_FORMAT_R16G16_SINT;
    case Format_R16G16_SFLOAT:
        return VK_FORMAT_R16G16_SFLOAT;

    case Format_R16G16B16_UNORM:
        return VK_FORMAT_R16G16B16_UNORM;
    case Format_R16G16B16_SNORM:
        return VK_FORMAT_R16G16B16_SNORM;
    case Format_R16G16B16_UINT:
        return VK_FORMAT_R16G16B16_UINT;
    case Format_R16G16B16_SINT:
        return VK_FORMAT_R16G16B16_SINT;
    case Format_R16G16B16_SFLOAT:
        return VK_FORMAT_R16G16B16_SFLOAT;

    case Format_R16G16B16A16_UNORM:
        return VK_FORMAT_R16G16B16A16_UNORM;
    case Format_R16G16B16A16_SNORM:
        return VK_FORMAT_R16G16B16A16_SNORM;
    case Format_R16G16B16A16_UINT:
        return VK_FORMAT_R16G16B16A16_UINT;
    case Format_R16G16B16A16_SINT:
        return VK_FORMAT_R16G16B16A16_SINT;
    case Format_R16G16B16A16_SFLOAT:
        return VK_FORMAT_R16G16B16A16_SFLOAT;

    case Format_R32_UINT:
        return VK_FORMAT_R32_UINT;
    case Format_R32_SINT:
        return VK_FORMAT_R32_SINT;
    case Format_R32_SFLOAT:
        return VK_FORMAT_R32_SFLOAT;

    case Format_R32G32_UINT:
        return VK_FORMAT_R32G32_UINT;
    case Format_R32G32_SINT:
        return VK_FORMAT_R32G32_SINT;
    case Format_R32G32_SFLOAT:
        return VK_FORMAT_R32G32_SFLOAT;

    case Format_R32G32B32_UINT:
        return VK_FORMAT_R32G32B32_UINT;
    case Format_R32G32B32_SINT:
        return VK_FORMAT_R32G32B32_SINT;
    case Format_R32G32B32_SFLOAT:
        return VK_FORMAT_R32G32B32_SFLOAT;

    case Format_R32G32B32A32_UINT:
        return VK_FORMAT_R32G32B32A32_UINT;
    case Format_R32G32B32A32_SINT:
        return VK_FORMAT_R32G32B32A32_SINT;
    case Format_R32G32B32A32_SFLOAT:
        return VK_FORMAT_R32G32B32A32_SFLOAT;

    case Format_D16_UNORM:
        return VK_FORMAT_D16_UNORM;
    case Format_D32_SFLOAT:
        return VK_FORMAT_D32_SFLOAT;

    case Format_D24_UNORM_S8_UINT:
        return VK_FORMAT_D24_UNORM_S8_UINT;
    case Format_D32_SFLOAT_S8_UINT:
        return VK_FORMAT_D32_SFLOAT_S8_UINT;

    case Format_BC1_RGBA_UNORM:
        return VK_FORMAT_BC1_RGBA_UNORM_BLOCK;
    case Format_BC1_RGBA_SRGB:
        return VK_FORMAT_BC1_RGBA_SRGB_BLOCK;
    case Format_BC5_UNORM:
        return VK_FORMAT_BC5_UNORM_BLOCK;
    case Format_BC5_SNORM:
        return VK_FORMAT_BC5_SNORM_BLOCK;
    case Format_BC7_UNORM:
        return VK_FORMAT_BC7_UNORM_BLOCK;
    case Format_BC7_SRGB:
        return VK_FORMAT_BC7_SRGB_BLOCK;

    default:
        return VK_FORMAT_UNDEFINED;
    }
}

VkImageTiling ToVulkan(const ImageTiling tiling)
{
    switch (tiling)
    {
    case ImageTiling_Optimal:
        return VK_IMAGE_TILING_OPTIMAL;
    case ImageTiling_Linear:
        return VK_IMAGE_TILING_LINEAR;
    default:
        TKIT_FATAL("[GRAPH] Unknown image tiling: {}", u32(tiling));
        return VK_IMAGE_TILING_OPTIMAL;
    }
}

VkImageLayout ToVulkan(const ImageLayout layout)
{
    switch (layout)
    {
    case ImageLayout_Undefined:
        return VK_IMAGE_LAYOUT_UNDEFINED;
    case ImageLayout_General:
        return VK_IMAGE_LAYOUT_GENERAL;
    case ImageLayout_ColorAttachment:
        return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    case ImageLayout_DepthStencilAttachment:
        return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    case ImageLayout_DepthStencilReadOnly:
        return VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
    case ImageLayout_ShaderReadOnly:
        return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    case ImageLayout_TransferSrc:
        return VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    case ImageLayout_TransferDst:
        return VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    case ImageLayout_Present:
        return VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    default:
        TKIT_FATAL("[GRAPH] Unknown image layout: {}", u32(layout));
        return VK_IMAGE_LAYOUT_UNDEFINED;
    }
}

VkSampleCountFlagBits ToVulkan(const SampleCount samples)
{
    switch (samples)
    {
    case SampleCount_1:
        return VK_SAMPLE_COUNT_1_BIT;
    case SampleCount_2:
        return VK_SAMPLE_COUNT_2_BIT;
    case SampleCount_4:
        return VK_SAMPLE_COUNT_4_BIT;
    case SampleCount_8:
        return VK_SAMPLE_COUNT_8_BIT;
    case SampleCount_16:
        return VK_SAMPLE_COUNT_16_BIT;
    case SampleCount_32:
        return VK_SAMPLE_COUNT_32_BIT;
    case SampleCount_64:
        return VK_SAMPLE_COUNT_64_BIT;
    default:
        TKIT_FATAL("[GRAPH] Unknown sample count: {}", u32(samples));
        return VK_SAMPLE_COUNT_1_BIT;
    }
}

VkImageType ToVulkan(const ImageType type)
{
    switch (type)
    {
    case ImageType_1D:
        return VK_IMAGE_TYPE_1D;
    case ImageType_2D:
        return VK_IMAGE_TYPE_2D;
    case ImageType_3D:
        return VK_IMAGE_TYPE_3D;
    default:
        TKIT_FATAL("[GRAPH] Unknown image type: {}", u32(type));
        return VK_IMAGE_TYPE_2D;
    }
}

VkImageViewType ToVulkan(const ImageViewType type)
{
    switch (type)
    {
    case ImageViewType_1D:
        return VK_IMAGE_VIEW_TYPE_1D;
    case ImageViewType_2D:
        return VK_IMAGE_VIEW_TYPE_2D;
    case ImageViewType_3D:
        return VK_IMAGE_VIEW_TYPE_3D;
    default:
        TKIT_FATAL("[GRAPH] Unknown image view type: {}", u32(type));
        return VK_IMAGE_VIEW_TYPE_2D;
    }
}

VkImageAspectFlags ToVulkanImageAspectFlags(const ImageAspectFlags aspects)
{
    VkImageAspectFlags result = VK_IMAGE_ASPECT_NONE;
    if (aspects & ImageAspectFlag_Color)
        result |= VK_IMAGE_ASPECT_COLOR_BIT;
    if (aspects & ImageAspectFlag_Depth)
        result |= VK_IMAGE_ASPECT_DEPTH_BIT;
    if (aspects & ImageAspectFlag_Stencil)
        result |= VK_IMAGE_ASPECT_STENCIL_BIT;
    return result;
}
VkImageAspectFlags ToVulkanImageAspectFlags(const VKit::DeviceImage &img, const ImageAspectFlags aspects)
{
    return (aspects & ImageAspectFlag_Auto) ? img.InferAspectMask() : ToVulkanImageAspectFlags(aspects);
}

VkSamplerMipmapMode ToVulkan(const SamplerMode mode)
{
    switch (mode)
    {
    case SamplerMode_Linear:
        return VK_SAMPLER_MIPMAP_MODE_LINEAR;
    case SamplerMode_Nearest:
        return VK_SAMPLER_MIPMAP_MODE_NEAREST;
    default:
        TKIT_FATAL("[GRAPH] Unknown sampler mode: {}", u32(mode));
        return VK_SAMPLER_MIPMAP_MODE_LINEAR;
    }
}

VkFilter ToVulkan(const Filter filter)
{
    switch (filter)
    {
    case Filter_Linear:
        return VK_FILTER_LINEAR;
    case Filter_Nearest:
        return VK_FILTER_NEAREST;
    case Filter_Cubic:
        return VK_FILTER_CUBIC_EXT;
    default:
        TKIT_FATAL("[GRAPH] Unknown filter: {}", u32(filter));
        return VK_FILTER_LINEAR;
    }
}

VkSamplerAddressMode ToVulkan(const Wrap wrap)
{
    switch (wrap)
    {
    case Wrap_Repeat:
        return VK_SAMPLER_ADDRESS_MODE_REPEAT;
    case Wrap_ClampToEdge:
        return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    case Wrap_MirroredRepeat:
        return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
    default:
        TKIT_FATAL("[GRAPH] Unknown wrap mode: {}", u32(wrap));
        return VK_SAMPLER_ADDRESS_MODE_REPEAT;
    }
}

VkCompareOp ToVulkan(const CompareOp op)
{
    switch (op)
    {
    case CompareOp_Never:
        return VK_COMPARE_OP_NEVER;
    case CompareOp_Less:
        return VK_COMPARE_OP_LESS;
    case CompareOp_Equal:
        return VK_COMPARE_OP_EQUAL;
    case CompareOp_LessOrEqual:
        return VK_COMPARE_OP_LESS_OR_EQUAL;
    case CompareOp_Greater:
        return VK_COMPARE_OP_GREATER;
    case CompareOp_NotEqual:
        return VK_COMPARE_OP_NOT_EQUAL;
    case CompareOp_GreaterOrEqual:
        return VK_COMPARE_OP_GREATER_OR_EQUAL;
    case CompareOp_Always:
        return VK_COMPARE_OP_ALWAYS;
    default:
        TKIT_FATAL("[GRAPH] Unknown compare operation: {}", u32(op));
        return VK_COMPARE_OP_NEVER;
    }
}

VkBorderColor ToVulkan(const BorderColor color)
{
    switch (color)
    {
    case BorderColor_FloatTransparentBlack:
        return VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
    case BorderColor_IntTransparentBlack:
        return VK_BORDER_COLOR_INT_TRANSPARENT_BLACK;
    case BorderColor_FloatOpaqueBlack:
        return VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
    case BorderColor_IntOpaqueBlack:
        return VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    case BorderColor_FloatOpaqueWhite:
        return VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
    case BorderColor_IntOpaqueWhite:
        return VK_BORDER_COLOR_INT_OPAQUE_WHITE;
    default:
        TKIT_FATAL("[GRAPH] Unknown border color: {}", u32(color));
        return VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
    }
}
VkShaderStageFlags ToVulkanShaderStageFlags(const ShaderStageFlags stages)
{
    VkShaderStageFlags result = 0;
    if (stages & ShaderStageFlag_Vertex)
        result |= VK_SHADER_STAGE_VERTEX_BIT;
    if (stages & ShaderStageFlag_Fragment)
        result |= VK_SHADER_STAGE_FRAGMENT_BIT;
    if (stages & ShaderStageFlag_Compute)
        result |= VK_SHADER_STAGE_COMPUTE_BIT;
    return result;
}
VkDescriptorBindingFlags ToVulkanDescriptorBindingFlags(const DescriptorBindingFlags flags)
{
    VkDescriptorBindingFlags result = 0;
    if (flags & DescriptorBindingFlag_UpdateAfterBind)
        result |= VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT;
    if (flags & DescriptorBindingFlag_UpdateUnusedWhilePending)
        result |= VK_DESCRIPTOR_BINDING_UPDATE_UNUSED_WHILE_PENDING_BIT;
    if (flags & DescriptorBindingFlag_PartiallyBound)
        result |= VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT;
    if (flags & DescriptorBindingFlag_VariableDescriptorCount)
        result |= VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT;
    return result;
}
VkPrimitiveTopology ToVulkan(const Topology topology)
{
    switch (topology)
    {
    case Topology_PointList:
        return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
    case Topology_LineList:
        return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
    case Topology_LineStrip:
        return VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
    case Topology_TriangleList:
        return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    case Topology_TriangleStrip:
        return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
    case Topology_TriangleFan:
        return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN;
    default:
        TKIT_FATAL("[GRAPH] Unknown topology: {}", u32(topology));
        return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    }
}

VkPolygonMode ToVulkan(const PolygonMode mode)
{
    switch (mode)
    {
    case PolygonMode_Fill:
        return VK_POLYGON_MODE_FILL;
    case PolygonMode_Line:
        return VK_POLYGON_MODE_LINE;
    case PolygonMode_Point:
        return VK_POLYGON_MODE_POINT;
    default:
        TKIT_FATAL("[GRAPH] Unknown polygon mode: {}", u32(mode));
        return VK_POLYGON_MODE_FILL;
    }
}

VkCullModeFlags ToVulkan(const CullMode mode)
{
    switch (mode)
    {
    case CullMode_None:
        return VK_CULL_MODE_NONE;
    case CullMode_Front:
        return VK_CULL_MODE_FRONT_BIT;
    case CullMode_Back:
        return VK_CULL_MODE_BACK_BIT;
    case CullMode_FrontAndBack:
        return VK_CULL_MODE_FRONT_AND_BACK;
    default:
        TKIT_FATAL("[GRAPH] Unknown cull mode: {}", u32(mode));
        return VK_CULL_MODE_NONE;
    }
}

VkFrontFace ToVulkan(const FrontFace face)
{
    switch (face)
    {
    case FrontFace_CounterClockwise:
        return VK_FRONT_FACE_COUNTER_CLOCKWISE;
    case FrontFace_Clockwise:
        return VK_FRONT_FACE_CLOCKWISE;
    default:
        TKIT_FATAL("[GRAPH] Unknown front face: {}", u32(face));
        return VK_FRONT_FACE_COUNTER_CLOCKWISE;
    }
}

VkBlendFactor ToVulkan(const BlendFactor factor)
{
    switch (factor)
    {
    case BlendFactor_Zero:
        return VK_BLEND_FACTOR_ZERO;
    case BlendFactor_One:
        return VK_BLEND_FACTOR_ONE;
    case BlendFactor_SrcColor:
        return VK_BLEND_FACTOR_SRC_COLOR;
    case BlendFactor_OneMinusSrcColor:
        return VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
    case BlendFactor_DstColor:
        return VK_BLEND_FACTOR_DST_COLOR;
    case BlendFactor_OneMinusDstColor:
        return VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
    case BlendFactor_SrcAlpha:
        return VK_BLEND_FACTOR_SRC_ALPHA;
    case BlendFactor_OneMinusSrcAlpha:
        return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    case BlendFactor_DstAlpha:
        return VK_BLEND_FACTOR_DST_ALPHA;
    case BlendFactor_OneMinusDstAlpha:
        return VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
    default:
        TKIT_FATAL("[GRAPH] Unknown blend factor: {}", u32(factor));
        return VK_BLEND_FACTOR_ZERO;
    }
}

VkBlendOp ToVulkan(const BlendOp op)
{
    switch (op)
    {
    case BlendOp_Add:
        return VK_BLEND_OP_ADD;
    case BlendOp_Subtract:
        return VK_BLEND_OP_SUBTRACT;
    case BlendOp_ReverseSubtract:
        return VK_BLEND_OP_REVERSE_SUBTRACT;
    case BlendOp_Min:
        return VK_BLEND_OP_MIN;
    case BlendOp_Max:
        return VK_BLEND_OP_MAX;
    default:
        TKIT_FATAL("[GRAPH] Unknown blend op: {}", u32(op));
        return VK_BLEND_OP_ADD;
    }
}

VkColorComponentFlags ToVulkanColorWriteMask(const ColorWriteMask mask)
{
    VkColorComponentFlags result = 0;
    if (mask & ColorWrite_R)
        result |= VK_COLOR_COMPONENT_R_BIT;
    if (mask & ColorWrite_G)
        result |= VK_COLOR_COMPONENT_G_BIT;
    if (mask & ColorWrite_B)
        result |= VK_COLOR_COMPONENT_B_BIT;
    if (mask & ColorWrite_A)
        result |= VK_COLOR_COMPONENT_A_BIT;
    return result;
}

VkStencilOp ToVulkan(const StencilOp op)
{
    switch (op)
    {
    case StencilOp_Keep:
        return VK_STENCIL_OP_KEEP;
    case StencilOp_Zero:
        return VK_STENCIL_OP_ZERO;
    case StencilOp_Replace:
        return VK_STENCIL_OP_REPLACE;
    case StencilOp_IncrementAndClamp:
        return VK_STENCIL_OP_INCREMENT_AND_CLAMP;
    case StencilOp_DecrementAndClamp:
        return VK_STENCIL_OP_DECREMENT_AND_CLAMP;
    case StencilOp_Invert:
        return VK_STENCIL_OP_INVERT;
    case StencilOp_IncrementAndWrap:
        return VK_STENCIL_OP_INCREMENT_AND_WRAP;
    case StencilOp_DecrementAndWrap:
        return VK_STENCIL_OP_DECREMENT_AND_WRAP;
    default:
        TKIT_FATAL("[GRAPH] Unknown stencil op: {}", u32(op));
        return VK_STENCIL_OP_KEEP;
    }
}

VkVertexInputRate ToVulkan(const VertexInputRate rate)
{
    switch (rate)
    {
    case VertexInputRate_Vertex:
        return VK_VERTEX_INPUT_RATE_VERTEX;
    case VertexInputRate_Instance:
        return VK_VERTEX_INPUT_RATE_INSTANCE;
    default:
        TKIT_FATAL("[GRAPH] Unknown vertex input rate: {}", u32(rate));
        return VK_VERTEX_INPUT_RATE_VERTEX;
    }
}
VkDescriptorType ToVulkan(const DescriptorType type)
{
    switch (type)
    {
    case Descriptor_StorageBuffer:
        return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    case Descriptor_UniformBuffer:
        return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    case Descriptor_Sampler:
        return VK_DESCRIPTOR_TYPE_SAMPLER;
    case Descriptor_SampledImage:
        return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    case Descriptor_CombinedImageSampler:
        return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    case Descriptor_StorageImage:
        return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    default:
        TKIT_FATAL("[GRAPH] Unknown descriptor type: {}", u32(type));
        return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    }
}
VkAttachmentLoadOp ToVulkan(const LoadOp op)
{
    switch (op)
    {
    case LoadOp_Load:
        return VK_ATTACHMENT_LOAD_OP_LOAD;
    case LoadOp_Clear:
        return VK_ATTACHMENT_LOAD_OP_CLEAR;
    case LoadOp_DontCare:
        return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    default:
        TKIT_FATAL("[GRAPH] Unknown load operation: {}", u32(op));
        return VK_ATTACHMENT_LOAD_OP_LOAD;
    }
}
VkAttachmentStoreOp ToVulkan(const StoreOp op)
{
    switch (op)
    {
    case StoreOp_Store:
        return VK_ATTACHMENT_STORE_OP_STORE;
    case StoreOp_DontCare:
        return VK_ATTACHMENT_STORE_OP_DONT_CARE;
    default:
        TKIT_FATAL("[GRAPH] Unknown store operation: {}", u32(op));
        return VK_ATTACHMENT_STORE_OP_STORE;
    }
}

VkResolveModeFlagBits ToVulkan(const ResolveMode mode)
{
    switch (mode)
    {
    case Resolve_None:
        return VK_RESOLVE_MODE_NONE;
    case Resolve_SampleZero:
        return VK_RESOLVE_MODE_SAMPLE_ZERO_BIT;
    case Resolve_Average:
        return VK_RESOLVE_MODE_AVERAGE_BIT;
    case Resolve_Min:
        return VK_RESOLVE_MODE_MIN_BIT;
    case Resolve_Max:
        return VK_RESOLVE_MODE_MAX_BIT;
    default:
        TKIT_FATAL("[GRAPH] Unknown resolve mode: {}", u32(mode));
        return VK_RESOLVE_MODE_NONE;
    }
}
VkPipelineStageFlags2KHR ToVulkanPipelineStageFlags(const PipelineStageFlags stages)
{
    VkPipelineStageFlags2KHR result = VK_PIPELINE_STAGE_2_NONE_KHR;
    if (stages & PipelineStageFlag_DrawIndirect)
        result |= VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT_KHR;
    if (stages & PipelineStageFlag_VertexInput)
        result |= VK_PIPELINE_STAGE_2_VERTEX_INPUT_BIT_KHR;
    if (stages & PipelineStageFlag_VertexShader)
        result |= VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT_KHR;
    if (stages & PipelineStageFlag_FragmentShader)
        result |= VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT_KHR;
    if (stages & PipelineStageFlag_EarlyFragmentTests)
        result |= VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT_KHR;
    if (stages & PipelineStageFlag_LateFragmentTests)
        result |= VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT_KHR;
    if (stages & PipelineStageFlag_ColorAttachmentOutput)
        result |= VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR;
    if (stages & PipelineStageFlag_ComputeShader)
        result |= VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT_KHR;
    if (stages & PipelineStageFlag_Transfer)
        result |= VK_PIPELINE_STAGE_2_TRANSFER_BIT_KHR;
    if (stages & PipelineStageFlag_Host)
        result |= VK_PIPELINE_STAGE_2_HOST_BIT_KHR;
    if (stages & PipelineStageFlag_AllCommands)
        result |= VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT_KHR;
    return result;
}

VkPipelineBindPoint ToVulkan(const BindPoint point)
{
    switch (point)
    {
    case BindPoint_Graphics:
        return VK_PIPELINE_BIND_POINT_GRAPHICS;
    case BindPoint_Compute:
        return VK_PIPELINE_BIND_POINT_COMPUTE;
    default:
        TKIT_FATAL("[GRAPH] Unknown bind point: {}", u32(point));
        return VK_PIPELINE_BIND_POINT_GRAPHICS;
    }
}

VkIndexType ToVulkan(const IndexType type)
{
    switch (type)
    {
    case IndexType_Unsigned8:
        return VK_INDEX_TYPE_UINT8_EXT;
    case IndexType_Unsigned16:
        return VK_INDEX_TYPE_UINT16;
    case IndexType_Unsigned32:
        return VK_INDEX_TYPE_UINT32;
    default:
        TKIT_FATAL("[GRAPH] Unknown index type: {}", u32(type));
        return VK_INDEX_TYPE_UINT32;
    }
}

#ifdef GRAPH_HAS_PLATFORM_BACKEND
VkPresentModeKHR ToVulkan(const PresentMode mode)
{
    switch (mode)
    {
    case PresentMode_Immediate:
        return VK_PRESENT_MODE_IMMEDIATE_KHR;
    case PresentMode_Mailbox:
        return VK_PRESENT_MODE_MAILBOX_KHR;
    case PresentMode_VSync:
        return VK_PRESENT_MODE_FIFO_KHR;
    default:
        TKIT_FATAL("[GRAPH] Unknown present mode: {}", u32(mode));
        return VK_PRESENT_MODE_FIFO_KHR;
    }
}
#endif

VkAccessFlags2KHR ToVulkanAccessFlags(const AccessFlags access)
{
    VkAccessFlags2KHR result = VK_ACCESS_2_NONE_KHR;
    if (access & AccessFlag_IndirectCommandRead)
        result |= VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT_KHR;
    if (access & AccessFlag_IndexRead)
        result |= VK_ACCESS_2_INDEX_READ_BIT_KHR;
    if (access & AccessFlag_VertexAttributeRead)
        result |= VK_ACCESS_2_VERTEX_ATTRIBUTE_READ_BIT_KHR;
    if (access & AccessFlag_UniformRead)
        result |= VK_ACCESS_2_UNIFORM_READ_BIT_KHR;
    if (access & AccessFlag_ShaderRead)
        result |= VK_ACCESS_2_SHADER_READ_BIT_KHR;
    if (access & AccessFlag_ShaderWrite)
        result |= VK_ACCESS_2_SHADER_WRITE_BIT_KHR;
    if (access & AccessFlag_ColorAttachmentRead)
        result |= VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT_KHR;
    if (access & AccessFlag_ColorAttachmentWrite)
        result |= VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT_KHR;
    if (access & AccessFlag_DepthStencilAttachmentRead)
        result |= VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT_KHR;
    if (access & AccessFlag_DepthStencilAttachmentWrite)
        result |= VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT_KHR;
    if (access & AccessFlag_TransferRead)
        result |= VK_ACCESS_2_TRANSFER_READ_BIT_KHR;
    if (access & AccessFlag_TransferWrite)
        result |= VK_ACCESS_2_TRANSFER_WRITE_BIT_KHR;
    if (access & AccessFlag_HostRead)
        result |= VK_ACCESS_2_HOST_READ_BIT_KHR;
    if (access & AccessFlag_HostWrite)
        result |= VK_ACCESS_2_HOST_WRITE_BIT_KHR;
    if (access & AccessFlag_MemoryRead)
        result |= VK_ACCESS_2_MEMORY_READ_BIT_KHR;
    if (access & AccessFlag_MemoryWrite)
        result |= VK_ACCESS_2_MEMORY_WRITE_BIT_KHR;
    return result;
}
VKit::DeviceBufferFlags ToVulkanBufferFlags(const BufferFlags flags)
{
    VKit::DeviceBufferFlags result = 0;
    if (flags & BufferFlag_DeviceLocal)
        result |= VKit::DeviceBufferFlag_DeviceLocal;
    if (flags & BufferFlag_HostVisible)
        result |= VKit::DeviceBufferFlag_HostVisible;
    if (flags & BufferFlag_Source)
        result |= VKit::DeviceBufferFlag_Source;
    if (flags & BufferFlag_Destination)
        result |= VKit::DeviceBufferFlag_Destination;
    if (flags & BufferFlag_Staging)
        result |= VKit::DeviceBufferFlag_Staging;
    if (flags & BufferFlag_Vertex)
        result |= VKit::DeviceBufferFlag_Vertex;
    if (flags & BufferFlag_Index)
        result |= VKit::DeviceBufferFlag_Index;
    if (flags & BufferFlag_Storage)
        result |= VKit::DeviceBufferFlag_Storage;
    if (flags & BufferFlag_Indirect)
        result |= VKit::DeviceBufferFlag_Indirect;
    if (flags & BufferFlag_HostMapped)
        result |= VKit::DeviceBufferFlag_HostMapped;
    if (flags & BufferFlag_HostRandomAccess)
        result |= VKit::DeviceBufferFlag_HostRandomAccess;
    return result;
}

VKit::DeviceImageFlags ToVulkanImageFlags(const ImageFlags flags)
{
    VKit::DeviceImageFlags result = 0;
    if (flags & ImageFlag_Color)
        result |= VKit::DeviceImageFlag_Color;
    if (flags & ImageFlag_Depth)
        result |= VKit::DeviceImageFlag_Depth;
    if (flags & ImageFlag_Stencil)
        result |= VKit::DeviceImageFlag_Stencil;
    if (flags & ImageFlag_ColorAttachment)
        result |= VKit::DeviceImageFlag_ColorAttachment;
    if (flags & ImageFlag_DepthAttachment)
        result |= VKit::DeviceImageFlag_DepthAttachment;
    if (flags & ImageFlag_StencilAttachment)
        result |= VKit::DeviceImageFlag_StencilAttachment;
    if (flags & ImageFlag_InputAttachment)
        result |= VKit::DeviceImageFlag_InputAttachment;
    if (flags & ImageFlag_Sampled)
        result |= VKit::DeviceImageFlag_Sampled;
    if (flags & ImageFlag_Storage)
        result |= VKit::DeviceImageFlag_Storage;
    if (flags & ImageFlag_ForceHostVisible)
        result |= VKit::DeviceImageFlag_ForceHostVisible;
    if (flags & ImageFlag_Source)
        result |= VKit::DeviceImageFlag_Source;
    if (flags & ImageFlag_Destination)
        result |= VKit::DeviceImageFlag_Destination;
    return result;
}

VKit::QueueType ToVulkan(const QueueType type)
{
    switch (type)
    {
    case Queue_Graphics:
        return VKit::Queue_Graphics;
    case Queue_Transfer:
        return VKit::Queue_Transfer;
    case Queue_Compute:
        return VKit::Queue_Compute;
    default:
        TKIT_FATAL("[GRAPH] Unknown queue type: {}", u32(type));
        return VKit::Queue_Graphics;
    }
}

VkImageSubresourceRange ToVulkan(const VKit::DeviceImage &img, const ImageSubresourceRange &range)
{
    VkImageSubresourceRange r{};
    r.aspectMask = ToVulkanImageAspectFlags(img, range.Aspect);
    r.baseMipLevel = range.MipStart;
    r.levelCount = range.MipCount == GRAPH_WHOLE_THING ? img.GetInfo().MipLevels : range.MipCount;
    r.baseArrayLayer = range.LayerStart;
    r.layerCount = range.LayerCount == GRAPH_WHOLE_THING ? img.GetInfo().ArrayLayers : range.LayerCount;
    return r;
}
VkImageSubresourceLayers ToVulkan(const VKit::DeviceImage &img, const ImageSubresourceLayers &layers)
{
    VkImageSubresourceLayers l{};
    l.aspectMask = ToVulkanImageAspectFlags(img, layers.Aspect);
    l.mipLevel = layers.MipLevel;
    l.baseArrayLayer = layers.LayerStart;
    l.layerCount = layers.LayerCount == GRAPH_WHOLE_THING ? img.GetInfo().ArrayLayers : layers.LayerCount;
    return l;
}
} // namespace Graph
