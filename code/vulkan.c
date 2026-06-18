
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
    X(vkGetPhysicalDeviceSurfaceFormatsKHR) \
    X(vkGetPhysicalDeviceSurfacePresentModesKHR) \
    X(vkGetPhysicalDeviceSurfaceCapabilitiesKHR) \
    \
    X(vkCreateDevice) \
    X(vkDeviceWaitIdle) \
    \
    X(vkGetDeviceQueue) \
    X(vkQueueSubmit) \
    X(vkQueuePresentKHR) \
    \
    X(vkCreateCommandPool) \
    X(vkAllocateCommandBuffers) \
    X(vkResetCommandBuffer) \
    X(vkBeginCommandBuffer) \
    X(vkEndCommandBuffer) \
    \
    X(vkCreateImageView) \
    \
    X(vkCreateSwapchainKHR) \
    X(vkGetSwapchainImagesKHR) \
    X(vkAcquireNextImageKHR) \
    \
    X(vkCreateSemaphore) \
    \
    X(vkCreateFence) \
    X(vkWaitForFences) \
    X(vkResetFences) \
    \
    X(vkCmdBeginRendering) \
    X(vkCmdEndRendering) \
    \
    X(vkCmdPipelineBarrier) \
    \
    X(vkDestroyInstance) \
    X(vkDestroySurfaceKHR) \
    X(vkDestroyDevice) \
    X(vkDestroyCommandPool) \
    X(vkDestroyImageView) \
    X(vkDestroySwapchainKHR) \
    X(vkDestroySemaphore) \
    X(vkDestroyFence)

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

// NOTE(vak): Maximum number of frames in flight.
#define VulkanMaxFlight (8)

typedef struct
{
    VkSwapchainKHR Swapchain;

    VkPresentModeKHR PresentMode;
    VkFormat Format;
    VkColorSpaceKHR ColorSpace;

    u32 ImageCount;
    u32 SizeX;
    u32 SizeY;

    VkImage Images[VulkanMaxFlight];
    VkImageView ImageViews[VulkanMaxFlight];
} vulkan_swapchain;

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
    vulkan_swapchain Swapchain;

    // NOTE(vak): Frame resources

    VkCommandBuffer CommandBuffers[VulkanMaxFlight];
    VkSemaphore AcquireSemaphores[VulkanMaxFlight];
    VkSemaphore SubmitSemaphores[VulkanMaxFlight];
    VkFence DrawFinishedFences[VulkanMaxFlight];

    // NOTE(vak): Rendering loop state

    u32 ImageIndex;
    u32 FlightIndex;
} vulkan_context;

local vulkan_context Vulkan = {0};

local void VulkanRecreateSwapchain(void)
{
    vulkan_swapchain* Swapchain = &Vulkan.Swapchain;

    if (Swapchain->Swapchain)
    {
        for (u32 Index = 0; Index < Swapchain->ImageCount; Index++)
            vkDestroyImageView(Vulkan.Device, Swapchain->ImageViews[Index], 0);
    }

    VkSurfaceCapabilitiesKHR SurfaceCapabilities = {0};

    VulkanCheck(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
        Vulkan.PhysicalDevice,
        Vulkan.Surface,
        &SurfaceCapabilities
    ));

    AlwaysAssert(VulkanMaxFlight >= SurfaceCapabilities.minImageCount);

    u32 MinImageCount = Minimum(
        VulkanMaxFlight, SurfaceCapabilities.maxImageCount
    );

    Swapchain->SizeX = SurfaceCapabilities.currentExtent.width;
    Swapchain->SizeY = SurfaceCapabilities.currentExtent.height;

    VkSwapchainKHR OldSwapchain = Swapchain->Swapchain;

    VkSwapchainCreateInfoKHR SwapchainInfo =
    {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = Vulkan.Surface,
        .minImageCount = MinImageCount,
        .imageFormat = Swapchain->Format,
        .imageColorSpace = Swapchain->ColorSpace,
        .imageExtent = SurfaceCapabilities.currentExtent,
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .preTransform = SurfaceCapabilities.currentTransform,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = Swapchain->PresentMode,
        .clipped = true,
        .oldSwapchain = OldSwapchain
    };

    VulkanCheck(vkCreateSwapchainKHR(Vulkan.Device, &SwapchainInfo, 0, &Swapchain->Swapchain));

    if (OldSwapchain)
        vkDestroySwapchainKHR(Vulkan.Device, OldSwapchain, 0);

    Swapchain->ImageCount = ArrayCount(Swapchain->Images);

    VulkanCheck(vkGetSwapchainImagesKHR(
        Vulkan.Device,
        Swapchain->Swapchain,
        &Swapchain->ImageCount,
        Swapchain->Images
    ));

    AlwaysAssert(Swapchain->ImageCount > 0);

    for (usize Index = 0; Index < Swapchain->ImageCount; Index++)
    {
        VkImageViewCreateInfo ViewInfo =
        {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = Swapchain->Images[Index],
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = Swapchain->Format,
            .components =
            {
                .r = VK_COMPONENT_SWIZZLE_IDENTITY,
                .g = VK_COMPONENT_SWIZZLE_IDENTITY,
                .b = VK_COMPONENT_SWIZZLE_IDENTITY,
                .a = VK_COMPONENT_SWIZZLE_IDENTITY,
            },
            .subresourceRange =
            {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .layerCount = 1,
                .levelCount = 1,
            },
        };

        VulkanCheck(vkCreateImageView(
            Vulkan.Device,
            &ViewInfo,
            0,
            &Swapchain->ImageViews[Index]
        ));
    }
}

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
        VkQueueFamilyProperties QueueFamilyProperties[32] = {0};
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

        VkPhysicalDeviceVulkan13Features Vulkan13Features =
        {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
            .dynamicRendering = true,
        };

        VkDeviceCreateInfo DeviceInfo =
        {
            .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
            .pNext = &Vulkan13Features,
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

    // NOTE(vak): Present mode
    {
        vulkan_swapchain* Swapchain = &Vulkan.Swapchain;

        VkPresentModeKHR PresentModes[16] = {0};
        u32 PresentModeCount = ArrayCount(PresentModes);

        VulkanCheck(vkGetPhysicalDeviceSurfacePresentModesKHR(
            Vulkan.PhysicalDevice,
            Vulkan.Surface,
            &PresentModeCount,
            PresentModes
        ));

        Swapchain->PresentMode = VK_PRESENT_MODE_FIFO_KHR;

        for (u32 Index = 0; Index < PresentModeCount; Index++)
        {
            VkPresentModeKHR PresentMode = PresentModes[Index];

            if (PresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
            {
                Swapchain->PresentMode = PresentMode;
                break;
            }
        }
    }

    // NOTE(vak): Swapchain format
    {
        vulkan_swapchain* Swapchain = &Vulkan.Swapchain;

        VkSurfaceFormatKHR SurfaceFormats[64] = {0};
        u32 SurfaceFormatCount = ArrayCount(SurfaceFormats);

        VulkanCheck(vkGetPhysicalDeviceSurfaceFormatsKHR(
            Vulkan.PhysicalDevice,
            Vulkan.Surface,
            &SurfaceFormatCount,
            SurfaceFormats
        ));

        Swapchain->Format = VK_FORMAT_UNDEFINED;
        Swapchain->ColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;

        for (u32 Index = 0; Index < SurfaceFormatCount; Index++)
        {
            VkSurfaceFormatKHR SurfaceFormat = SurfaceFormats[Index];

            if (
                (SurfaceFormat.format == VK_FORMAT_R8G8B8A8_UNORM) ||
                (SurfaceFormat.format == VK_FORMAT_B8G8R8A8_UNORM)
            )
            {
                Swapchain->Format = SurfaceFormat.format;
                Swapchain->ColorSpace = SurfaceFormat.colorSpace;
                break;
            }
        }

        AlwaysAssert(Swapchain->Format != VK_FORMAT_UNDEFINED);
    }

    // NOTE(vak): Swapchain
    {
        VulkanRecreateSwapchain();
    }

    // NOTE(vak): Frame resources
    {
        VkCommandBufferAllocateInfo AllocateInfo =
        {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .commandPool = Vulkan.CommandPool,
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = VulkanMaxFlight,
        };

        VulkanCheck(vkAllocateCommandBuffers(
            Vulkan.Device, &AllocateInfo, Vulkan.CommandBuffers
        ));

        VkSemaphoreCreateInfo SemaphoreInfo =
        {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        };

        VkFenceCreateInfo FenceInfo =
        {
            .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
            .flags = VK_FENCE_CREATE_SIGNALED_BIT,
        };

        for (usize Index = 0; Index < VulkanMaxFlight; Index++)
        {
            VulkanCheck(vkCreateSemaphore(Vulkan.Device, &SemaphoreInfo, 0, &Vulkan.AcquireSemaphores[Index]));
            VulkanCheck(vkCreateSemaphore(Vulkan.Device, &SemaphoreInfo, 0, &Vulkan.SubmitSemaphores[Index]));
            VulkanCheck(vkCreateFence(Vulkan.Device, &FenceInfo, 0, &Vulkan.DrawFinishedFences[Index]));
        }
    }
}

local void VulkanDeleteRenderer(void)
{
    VulkanCheck(vkDeviceWaitIdle(Vulkan.Device));

    for (usize Index = 0; Index < VulkanMaxFlight; Index++)
    {
        if (Vulkan.DrawFinishedFences[Index])
            vkDestroyFence(Vulkan.Device, Vulkan.DrawFinishedFences[Index], 0);

        if (Vulkan.AcquireSemaphores[Index])
            vkDestroySemaphore(Vulkan.Device, Vulkan.AcquireSemaphores[Index], 0);

        if (Vulkan.SubmitSemaphores[Index])
            vkDestroySemaphore(Vulkan.Device, Vulkan.SubmitSemaphores[Index], 0);
    }

    vulkan_swapchain* Swapchain = &Vulkan.Swapchain;

    if (Swapchain->Swapchain)
    {
        for (u32 Index = 0; Index < Swapchain->ImageCount; Index++)
            vkDestroyImageView(Vulkan.Device, Swapchain->ImageViews[Index], 0);

        vkDestroySwapchainKHR(Vulkan.Device, Swapchain->Swapchain, 0);
    }

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
    vulkan_swapchain* Swapchain = &Vulkan.Swapchain;

    // NOTE(vak): Resize swapchain if necessary
    {
        VkSurfaceCapabilitiesKHR SurfaceCapabilities = {0};

        VulkanCheck(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
            Vulkan.PhysicalDevice,
            Vulkan.Surface,
            &SurfaceCapabilities
        ));

        u32 SizeX = SurfaceCapabilities.currentExtent.width;
        u32 SizeY = SurfaceCapabilities.currentExtent.height;

        if ((SizeX != Swapchain->SizeX) || (SizeY != Swapchain->SizeY))
        {
            VulkanCheck(vkDeviceWaitIdle(Vulkan.Device));

            VulkanRecreateSwapchain();

            VulkanCheck(vkDeviceWaitIdle(Vulkan.Device));
        }
    }

    // NOTE(vak): Frame resources

    VkCommandBuffer CommandBuffer = Vulkan.CommandBuffers[Vulkan.FlightIndex];
    VkSemaphore AcquireSemaphore = Vulkan.AcquireSemaphores[Vulkan.FlightIndex];
    VkSemaphore SubmitSemaphores = Vulkan.SubmitSemaphores[Vulkan.FlightIndex];
    VkFence DrawFinishedFence = Vulkan.DrawFinishedFences[Vulkan.FlightIndex];

    // NOTE(vak): Wait for last draw utilizing this frame's resources
    {
        VulkanCheck(vkWaitForFences(Vulkan.Device, 1, &DrawFinishedFence, VK_TRUE, U64Max));
        VulkanCheck(vkResetFences(Vulkan.Device, 1, &DrawFinishedFence));
    }

    // NOTE(vak): Acquire next image
    {
        VulkanCheck(vkAcquireNextImageKHR(
            Vulkan.Device,
            Swapchain->Swapchain,
            U64Max,
            AcquireSemaphore,
            0,
            &Vulkan.ImageIndex
        ));
    }

    // NOTE(vak): Start recording commands
    {
        VkCommandBufferBeginInfo BeginInfo =
        {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        };

        VulkanCheck(vkResetCommandBuffer(CommandBuffer, 0));
        VulkanCheck(vkBeginCommandBuffer(CommandBuffer, &BeginInfo));
    }

    // NOTE(vak): Rendering pipeline barrier
    {
        VkImageMemoryBarrier RenderBarrier =
        {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT,  // NOTE(vak): Complete all reads
            .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = Swapchain->Images[Vulkan.ImageIndex],
            .subresourceRange =
            {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .layerCount = 1,
                .levelCount = 1,
            },
        };

        vkCmdPipelineBarrier(
            CommandBuffer,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, // NOTE(vak): Last Draw
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, // NOTE(vak): This Draw
            VK_DEPENDENCY_BY_REGION_BIT,
            0, 0,
            0, 0,
            1, &RenderBarrier
        );
    }

    // NOTE(vak): Begin rendering
    {
        VkRenderingAttachmentInfo ColorAttachment =
        {
            .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
            .imageView = Swapchain->ImageViews[Vulkan.ImageIndex],
            .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .clearValue =
            {
                .color = {0.08f, 0.09f, 0.11f, 1.0f},
            },
        };

        VkRenderingInfo RenderingInfo =
        {
            .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
            .renderArea = {{0, 0}, {Swapchain->SizeX, Swapchain->SizeY}},
            .layerCount = 1,
            .colorAttachmentCount = 1,
            .pColorAttachments = &ColorAttachment,
        };

        vkCmdBeginRendering(CommandBuffer, &RenderingInfo);
    }
}

local void VulkanEndRendering(void)
{
    vulkan_swapchain* Swapchain = &Vulkan.Swapchain;

    // NOTE(vak): Frame resources

    VkCommandBuffer CommandBuffer = Vulkan.CommandBuffers[Vulkan.FlightIndex];
    VkSemaphore AcquireSemaphore = Vulkan.AcquireSemaphores[Vulkan.FlightIndex];
    VkSemaphore SubmitSemaphore = Vulkan.SubmitSemaphores[Vulkan.FlightIndex];
    VkFence DrawFinishedFence = Vulkan.DrawFinishedFences[Vulkan.FlightIndex];

    // NOTE(vak): End rendering
    {
        vkCmdEndRendering(CommandBuffer);
    }

    // NOTE(vak): Present pipeline barrier
    {
        VkImageMemoryBarrier PresentBarrier =
        {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, // NOTE(vak): Complete all writes
            .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT,
            .oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = Swapchain->Images[Vulkan.ImageIndex],
            .subresourceRange =
            {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .layerCount = 1,
                .levelCount = 1,
            },
        };

        vkCmdPipelineBarrier(
            CommandBuffer,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            VK_DEPENDENCY_BY_REGION_BIT,
            0, 0,
            0, 0,
            1, &PresentBarrier
        );
    }

    // NOTE(vak): Stop recording commands
    {
        VulkanCheck(vkEndCommandBuffer(CommandBuffer));
    }

    // NOTE(vak): Submit commands
    {
        // NOTE(vak): Wait for the image to be acquired when we are ready to
        // write to the color attachment.
        VkPipelineStageFlags WaitDestinationStages = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

        VkSubmitInfo SubmitInfo =
        {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &AcquireSemaphore,
            .pWaitDstStageMask = &WaitDestinationStages,
            .commandBufferCount = 1,
            .pCommandBuffers = &CommandBuffer,
            .signalSemaphoreCount = 1,
            .pSignalSemaphores = &SubmitSemaphore,
        };

        VulkanCheck(vkQueueSubmit(Vulkan.Queue, 1, &SubmitInfo, DrawFinishedFence));
    }

    // NOTE(vak): Present
    {
        VkPresentInfoKHR PresentInfo =
        {
            .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &SubmitSemaphore,
            .swapchainCount = 1,
            .pSwapchains = &Swapchain->Swapchain,
            .pImageIndices = &Vulkan.ImageIndex,
        };

        VulkanCheck(vkQueuePresentKHR(Vulkan.Queue, &PresentInfo));
    }

    // NOTE(vak): Advance to next frame in flight
    {
        Vulkan.FlightIndex += 1;
        Vulkan.FlightIndex %= Swapchain->ImageCount;
    }
}
