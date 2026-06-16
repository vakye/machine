
// NOTE(vak): Vulkan implementation of rendering-related functionality

#pragma once

// ----------------------------------------------------------
// NOTE(vak): Vulkan include and configuration
// ----------------------------------------------------------

#define VK_NO_PROTOTYPES

#define VK_NO_STDINT_H
#define VK_NO_STDDEF_H

typedef s8  int8_t;
typedef s16 int16_t;
typedef s32 int32_t;
typedef s64 int64_t;

typedef u8  uint8_t;
typedef u16 uint16_t;
typedef u32 uint32_t;
typedef u64 uint64_t;

typedef usize size_t;

#include <vulkan/vulkan_core.h>

#if PlatformWindows
#  define VK_USE_PLATFORM_WIN32_KHR
#  include <vulkan/vulkan_win32.h>
#else
#  error No Vulkan implementation available for this platform
#endif

// ----------------------------------------------------------
// NOTE(vak): Collection of Vulkan functions that are
// necessary for this implementation.
// ----------------------------------------------------------

local PFN_vkGetInstanceProcAddr      vkGetInstanceProcAddr      = {0};
local PFN_vkCreateInstance           vkCreateInstance           = {0};
local PFN_vkEnumerateInstanceVersion vkEnumerateInstanceVersion = {0};

#define AllVulkanFunctions(X) \
    X(vkCreateWin32SurfaceKHR) \
    \
    X(vkEnumeratePhysicalDevices) \
    X(vkGetPhysicalDeviceProperties) \
    X(vkGetPhysicalDeviceQueueFamilyProperties) \
    \
    X(vkCreateDevice) \
    X(vkGetDeviceQueue) \
    \
    X(vkCreateCommandPool) \
    X(vkAllocateCommandBuffers) \
    \
    X(vkDestroyInstance) \
    X(vkDestroySurfaceKHR) \
    X(vkDestroyDevice) \
    X(vkDestroyCommandPool)

#define DeclareVulkanFunction(Name) \
    local PFN_##Name Name = {0};

AllVulkanFunctions(DeclareVulkanFunction)

#undef DeclareVulkanFunction

// ----------------------------------------------------------
// NOTE(vak): Helpers
// ----------------------------------------------------------

#define VulkanCheck(VulkanCall) DebugAssert(VulkanCall == VK_SUCCESS)

// ----------------------------------------------------------
// NOTE(vak): Implementation
// ----------------------------------------------------------

typedef struct
{
    u32 VersionOfAPI;

    VkInstance Instance;
    VkSurfaceKHR Surface;

    VkPhysicalDevice PhysicalDevice;
    u32 QueueFamilyIndex;

    VkDevice Device;
    VkQueue Queue;

    VkCommandPool CommandPool;
    VkCommandBuffer CommandBuffer;
} vulkan_context;

local vulkan_context Vulkan = {0};

local void VulkanDeleteRenderer(void);
local void VulkanBeginRendering(void);
local void VulkanEndRendering(void);

local void VulkanMakeRenderer(void)
{
    // NOTE(vak): Setup renderer functions
    {
        RendererFunctions.CurrentAPI     = RendererAPI_Vulkan;
        RendererFunctions.DeleteRenderer = &VulkanDeleteRenderer;
        RendererFunctions.BeginRendering = &VulkanBeginRendering;
        RendererFunctions.EndRendering   = &VulkanEndRendering;
    }

    // NOTE(vak): Setup Vulkan and load non-instance functions
    {
        vkGetInstanceProcAddr = (PFN_vkGetInstanceProcAddr)LoadVulkan();

        AlwaysAssert(vkGetInstanceProcAddr);

        vkCreateInstance = (PFN_vkCreateInstance)
            vkGetInstanceProcAddr(0, "vkCreateInstance");

        vkEnumerateInstanceVersion = (PFN_vkEnumerateInstanceVersion)
            vkGetInstanceProcAddr(0, "vkEnumerateInstanceVersion");

        AlwaysAssert(vkCreateInstance);
        AlwaysAssert(vkEnumerateInstanceVersion);
    }

    // NOTE(vak): Version
    {
        Vulkan.VersionOfAPI = VK_API_VERSION_1_3;

        u32 Version = 0;
        VulkanCheck(vkEnumerateInstanceVersion(&Version));

        AlwaysAssert(Version >= Vulkan.VersionOfAPI);
    }

    // NOTE(vak): Instance
    {
        VkApplicationInfo ApplicationInfo =
        {
            .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
            .pApplicationName = "Machine",
            .applicationVersion = VK_MAKE_VERSION(0, 0, 1),
            .pEngineName = "Machine",
            .engineVersion = VK_MAKE_VERSION(0, 0, 1),
            .apiVersion = Vulkan.VersionOfAPI,
        };

        char* Layers[] =
        {
            "VK_LAYER_KHRONOS_validation",
        };

        char* Extensions[] =
        {
            VK_KHR_SURFACE_EXTENSION_NAME,

            #if defined(VK_USE_PLATFORM_WIN32_KHR)
                VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
            #else
            #  error Missing surface extension for platform
            #endif
        };

        VkInstanceCreateInfo InstanceInfo =
        {
            .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
            .pApplicationInfo = &ApplicationInfo,
            .ppEnabledExtensionNames = Extensions,
            .enabledExtensionCount = ArrayCount(Extensions),
            .ppEnabledLayerNames = Layers,
            .enabledLayerCount = ArrayCount(Layers),
        };

        VulkanCheck(vkCreateInstance(&InstanceInfo, 0, &Vulkan.Instance));
    }

    // NOTE(vak): Load Vulkan functions
    {
        #define LoadVulkanFunction(Name) \
            Name = (PFN_##Name)vkGetInstanceProcAddr(Vulkan.Instance, #Name);

        AllVulkanFunctions(LoadVulkanFunction)

        #undef LoadVulkanFunction
    }

    // NOTE(vak): Surface
    {
        #if defined(VK_USE_PLATFORM_WIN32_KHR)
            VkWin32SurfaceCreateInfoKHR SurfaceInfo =
            {
                .sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
                .hinstance = GetModuleHandleA(0),
                .hwnd = Win32.Window,
            };

            VulkanCheck(vkCreateWin32SurfaceKHR(
                Vulkan.Instance, &SurfaceInfo, 0, &Vulkan.Surface
            ));
        #endif
    }

    // NOTE(vak): Physical Devices
    {
        VkPhysicalDevice PhysicalDevices[64] = {0};
        u32 PhysicalDeviceCount = ArrayCount(PhysicalDevices);

        VulkanCheck(vkEnumeratePhysicalDevices(
            Vulkan.Instance,
            &PhysicalDeviceCount,
            PhysicalDevices
        ));

        AlwaysAssert(PhysicalDeviceCount > 0);

        VkPhysicalDevice Fallback  = {0};
        VkPhysicalDevice Preferred = {0};

        for (u32 Index = 0; Index < PhysicalDeviceCount; Index++)
        {
            VkPhysicalDevice PhysicalDevice = PhysicalDevices[Index];

            VkPhysicalDeviceProperties Properties = {0};
            vkGetPhysicalDeviceProperties(PhysicalDevice, &Properties);

            if (Properties.apiVersion < Vulkan.VersionOfAPI)
                continue;

            if (Properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
                if (!Preferred) Preferred = PhysicalDevice;
            else
                if (!Fallback) Fallback = PhysicalDevice;
        }

        Vulkan.PhysicalDevice = (Preferred) ? (Preferred) : (Fallback);

        AlwaysAssert(Vulkan.PhysicalDevice);
    }

    // NOTE(vak): Queue family
    {
        VkQueueFamilyProperties QueueFamilyProperties[64] = {0};
        u32 QueueFamilyCount = ArrayCount(QueueFamilyProperties);

        vkGetPhysicalDeviceQueueFamilyProperties(
            Vulkan.PhysicalDevice,
            &QueueFamilyCount,
            QueueFamilyProperties
        );

        Vulkan.QueueFamilyIndex = U32Max;

        for (u32 Index = 0; Index < QueueFamilyCount; Index++)
        {
            VkQueueFamilyProperties* Properties = QueueFamilyProperties + Index;

            VkQueueFlags RequiredFlags =
                VK_QUEUE_GRAPHICS_BIT |
                VK_QUEUE_TRANSFER_BIT |
                VK_QUEUE_COMPUTE_BIT;

            if ((Properties->queueFlags & RequiredFlags) == RequiredFlags)
            {
                Vulkan.QueueFamilyIndex = Index;
                break;
            }
        }

        AlwaysAssert(Vulkan.QueueFamilyIndex != U32Max);
    }

    // NOTE(vak): Device
    {
        f32 QueuePriorities[1] = {1.0f};

        VkDeviceQueueCreateInfo QueueInfo =
        {
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .queueFamilyIndex = Vulkan.QueueFamilyIndex,
            .queueCount = 1,
            .pQueuePriorities = QueuePriorities,
        };

        char* Extensions[] =
        {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        };

        VkDeviceCreateInfo DeviceInfo =
        {
            .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
            .queueCreateInfoCount = 1,
            .pQueueCreateInfos = &QueueInfo,
            .ppEnabledExtensionNames = Extensions,
            .enabledExtensionCount = ArrayCount(Extensions),
        };

        VulkanCheck(vkCreateDevice(Vulkan.PhysicalDevice, &DeviceInfo, 0, &Vulkan.Device));
    }

    // NOTE(vak): Queue
    {
        vkGetDeviceQueue(Vulkan.Device, Vulkan.QueueFamilyIndex, 0, &Vulkan.Queue);
    }

    // NOTE(vak): Command pool
    {
        VkCommandPoolCreateInfo PoolInfo =
        {
            .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
            .queueFamilyIndex = Vulkan.QueueFamilyIndex,
        };

        VulkanCheck(vkCreateCommandPool(Vulkan.Device, &PoolInfo, 0, &Vulkan.CommandPool));
    }

    // NOTE(vak): Command buffer
    {
        VkCommandBufferAllocateInfo AllocateInfo =
        {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .commandPool = Vulkan.CommandPool,
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = 1,
        };

        VulkanCheck(vkAllocateCommandBuffers(
            Vulkan.Device, &AllocateInfo, &Vulkan.CommandBuffer
        ));
    }
}

local void VulkanDeleteRenderer(void)
{
    if (Vulkan.CommandPool)
        vkDestroyCommandPool(Vulkan.Device, Vulkan.CommandPool, 0);

    if (Vulkan.Device)
        vkDestroyDevice(Vulkan.Device, 0);

    if (Vulkan.Surface)
        vkDestroySurfaceKHR(Vulkan.Instance, Vulkan.Surface, 0);

    if (Vulkan.Instance)
        vkDestroyInstance(Vulkan.Instance, 0);

    ZeroType(&RendererFunctions);
    ZeroType(&Vulkan);

    UnloadVulkan();
}

local void VulkanBeginRendering(void)
{

}

local void VulkanEndRendering(void)
{

}
