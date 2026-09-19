#include "vk_core.hpp"
#include "vk_error.hpp"
#include "vkit/core/core.hpp"
#include "vkit/device/logical_device.hpp"
#include "vkit/memory/allocator.hpp"
#include "tkit/container/stack_array.hpp"

namespace Graph
{
static TKit::Storage<VKit::Instance> s_Instance{};
static TKit::Storage<VKit::PhysicalDevice> s_Physical{};
static TKit::Storage<VKit::LogicalDevice> s_Device{};
static VmaAllocator s_VulkanAllocator = VK_NULL_HANDLE;
static Specs s_Specs{};

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

    *s_Instance = GRAPH_CHECK_VKIT_RESULT(builder.Build());
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
    TKIT_LOG_INFO("[GRAPH][CORE] Initializing Vulkit");

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

    const VkSurfaceKHR dummy = CreateDummySurface();
    selector.SetSurface(dummy)
        .PreferType(VKit::Device_Discrete)
        .AddFlags(VKit::DeviceSelectorFlag_AnyType | VKit::DeviceSelectorFlag_PortabilitySubset |
                  VKit::DeviceSelectorFlag_RequireGraphicsQueue | VKit::DeviceSelectorFlag_RequirePresentQueue |
                  VKit::DeviceSelectorFlag_RequireTransferQueue)
        // Auto-enabled — Graphite always needs these
        .RequireExtension("VK_KHR_synchronization2")
        .RequireExtension("VK_KHR_copy_commands2")
        .RequireApiVersion(1, 2, 0)
        .RequestApiVersion(1, 4, 0);

    const Capabilities caps = s_Specs.EnabledCapabilities;
    if (caps & Capability_DynamicRendering)
        selector.RequireExtension("VK_KHR_dynamic_rendering");
    if (caps & Capability_TimelineSemaphores)
        selector.RequireExtension("VK_KHR_timeline_semaphore");
    if (caps & Capability_BindlessDescriptors)
        selector.RequireExtension("VK_EXT_descriptor_indexing");
    if (caps & Capability_ExtendedDynamicState)
        selector.RequireExtension("VK_EXT_extended_dynamic_state");
    if (caps & Capability_ImageMultiFormat)
        selector.RequireExtension("VK_KHR_image_format_list");

    const bool faultDump = caps & Capability_FaultDump;
    if (faultDump)
        selector.RequestExtension("VK_EXT_device_fault");

    *s_Physical = GRAPH_CHECK_VKIT_RESULT(selector.Select());
    DestroyDummySurface();

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

    VkPhysicalDeviceShaderDrawParameterFeatures drawParams{};
    drawParams.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_DRAW_PARAMETERS_FEATURES;

    VkPhysicalDeviceTimelineSemaphoreFeaturesKHR tsem{};
    tsem.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TIMELINE_SEMAPHORE_FEATURES_KHR;
    tsem.timelineSemaphore = VK_TRUE;

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

    if (caps & Capability_TimelineSemaphores)
        features.Vulkan12.timelineSemaphore = VK_TRUE;

    if (caps & Capability_BindlessDescriptors)
    {
        features.Vulkan12.descriptorBindingPartiallyBound = VK_TRUE;
        features.Vulkan12.runtimeDescriptorArray = VK_TRUE;
        features.Vulkan12.descriptorIndexing = VK_TRUE;
        features.Vulkan12.descriptorBindingUpdateUnusedWhilePending = VK_TRUE;
        features.Vulkan12.shaderSampledImageArrayNonUniformIndexing = VK_TRUE;
    }

    // Auto-enabled — sync2
    VkPhysicalDeviceSynchronization2FeaturesKHR sync2{};
    sync2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES_KHR;
    sync2.synchronization2 = VK_TRUE;

    // User-controlled
    VkPhysicalDeviceDynamicRenderingFeaturesKHR drendering{};
    drendering.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES_KHR;
    if (caps & Capability_DynamicRendering)
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
        if (caps & Capability_DynamicRendering)
            features.Vulkan13.dynamicRendering = VK_TRUE;

        TKIT_ASSERT(s_Physical->EnableFeatures(features), "[GRAPH][CORE] Failed to enable requested features");
    }
    else
    {
        s_Physical->EnableExtensionBoundFeature(&sync2);
        if (caps & Capability_DynamicRendering)
            s_Physical->EnableExtensionBoundFeature(&drendering);

        TKIT_ASSERT(s_Physical->EnableFeatures(features), "[GRAPH][CORE] Failed to enable requested features");
    }

    *s_Device = GRAPH_CHECK_VKIT_RESULT(VKit::LogicalDevice::Builder(&s_Instance.Get(), &s_Physical.Get())
                                            .RequireQueue(VKit::Queue_Graphics)
                                            .RequireQueue(VKit::Queue_Present)
                                            .RequireQueue(VKit::Queue_Transfer)
                                            .Build());

    if (IsDebugUtilsEnabled())
    {
        GRAPH_CHECK_VKIT_RESULT(s_Device->SetName("graph-device"));
    }
}

static void createVulkanAllocator()
{
    TKIT_LOG_INFO("[GRAPH][CORE] Creating vulkan allocator");
    s_VulkanAllocator = GRAPH_CHECK_VKIT_RESULT(VKit::CreateAllocator(*s_Device));
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
    GRAPH_CHECK_VKIT_RESULT(VKit::Initialize(vspecs));

    Platform_Initialize(specs.TargetPlatform);

    Surface_Initialize();

    createInstance();
    createDevice();
    createVulkanAllocator();
}

void Terminate()
{
    VKit::DestroyAllocator(s_VulkanAllocator);
    s_Device->Destroy();
    s_Instance->Destroy();

    Surface_Terminate();

    Platform_Terminate();

    VKit::Terminate();

    s_Device.Destruct();
    s_Physical.Destruct();
    s_Instance.Destruct();
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

    GRAPH_CHECK_VKIT_RESULT(table->GetDeviceFaultInfoEXT(device, &counts, nullptr));

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

    GRAPH_CHECK_VKIT_RESULT(table->GetDeviceFaultInfoEXT(device, &counts, &faultInfo));

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
} // namespace Graph
