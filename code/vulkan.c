
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
    X(vkDestroyInstance) \
    \
    X(vkCreateWin32SurfaceKHR) \
    X(vkDestroySurfaceKHR) \
    \
    X(vkEnumeratePhysicalDevices) \
    X(vkGetPhysicalDeviceProperties) \
    X(vkGetPhysicalDeviceQueueFamilyProperties) \
    X(vkGetPhysicalDeviceMemoryProperties) \
    \
    X(vkGetPhysicalDeviceSurfaceFormatsKHR) \
    X(vkGetPhysicalDeviceSurfacePresentModesKHR) \
    X(vkGetPhysicalDeviceSurfaceCapabilitiesKHR) \
    \
    X(vkCreateDevice) \
    X(vkDestroyDevice) \
    X(vkDeviceWaitIdle) \
    \
    X(vkGetDeviceQueue) \
    X(vkQueueSubmit) \
    X(vkQueuePresentKHR) \
    X(vkQueueWaitIdle) \
    \
    X(vkCreateCommandPool) \
    X(vkDestroyCommandPool) \
    X(vkAllocateCommandBuffers) \
    X(vkResetCommandBuffer) \
    X(vkBeginCommandBuffer) \
    X(vkEndCommandBuffer) \
    \
    X(vkCreateImageView) \
    X(vkDestroyImageView) \
    \
    X(vkCreateSwapchainKHR) \
    X(vkDestroySwapchainKHR) \
    X(vkGetSwapchainImagesKHR) \
    X(vkAcquireNextImageKHR) \
    \
    X(vkCreateShaderModule) \
    X(vkDestroyShaderModule) \
    \
    X(vkCreateDescriptorSetLayout) \
    X(vkDestroyDescriptorSetLayout) \
    \
    X(vkCreatePipelineLayout) \
    X(vkDestroyPipelineLayout) \
    \
    X(vkCreateGraphicsPipelines) \
    X(vkDestroyPipeline) \
    \
    X(vkAllocateMemory) \
    X(vkFreeMemory) \
    X(vkMapMemory) \
    X(vkUnmapMemory) \
    \
    X(vkCreateBuffer) \
    X(vkDestroyBuffer) \
    X(vkGetBufferMemoryRequirements) \
    X(vkBindBufferMemory) \
    \
    X(vkCreateSemaphore) \
    X(vkDestroySemaphore) \
    \
    X(vkCreateFence) \
    X(vkDestroyFence) \
    X(vkWaitForFences) \
    X(vkResetFences) \
    \
    X(vkCmdBeginRendering) \
    X(vkCmdBindPipeline) \
    X(vkCmdSetViewport) \
    X(vkCmdSetScissor) \
    X(vkCmdPushDescriptorSet) \
    X(vkCmdDraw) \
    X(vkCmdEndRendering) \
    \
    X(vkCmdCopyBuffer) \
    \
    X(vkCmdPipelineBarrier)

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
    VkDescriptorSetLayout SetLayout;
    VkPipelineLayout Layout;
    VkPipeline Pipeline;
} vulkan_pipeline;

typedef struct
{
    VkBuffer Buffer;
    VkDeviceMemory Memory;
    usize Size;
    void* Mapping;
} vulkan_buffer;

typedef struct
{
    f32 X, Y;
    f32 U, V;
    f32 R, G, B, A;
} vulkan_vertex;

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
    vulkan_pipeline Pipeline;

    vulkan_buffer VertexBuffer;

    // NOTE(vak): Frame-in-flight resources

    VkCommandBuffer CommandBuffers[VulkanMaxFlight];
    VkSemaphore AcquireSemaphores[VulkanMaxFlight];
    VkSemaphore SubmitSemaphores[VulkanMaxFlight];
    VkFence DrawFinishedFences[VulkanMaxFlight];

    // NOTE(vak): Rendering loop state

    u32 ImageIndex;
    u32 FlightIndex;
} vulkan_state;

local vulkan_state Vulkan = {0};

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

local vulkan_pipeline VulkanCreatePipeline(
    u32* VertexCode, usize VertexCodeSize,
    u32* FragmentCode, usize FragmentCodeSize
)
{
    vulkan_pipeline Pipeline = {0};

    VkShaderModule VertexShader = {0};
    {
        VkShaderModuleCreateInfo ShaderInfo =
        {
            .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
            .codeSize = VertexCodeSize,
            .pCode = VertexCode,
        };

        VulkanCheck(vkCreateShaderModule(Vulkan.Device, &ShaderInfo, 0, &VertexShader));
    }

    VkShaderModule FragmentShader = {0};
    {
        VkShaderModuleCreateInfo ShaderInfo =
        {
            .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
            .codeSize = FragmentCodeSize,
            .pCode = FragmentCode,
        };

        VulkanCheck(vkCreateShaderModule(Vulkan.Device, &ShaderInfo, 0, &FragmentShader));
    }

    // NOTE(vak): Descriptor set layout
    {
        VkDescriptorSetLayoutBinding Bindings[] =
        {
            {
                .binding = 0,
                .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                .descriptorCount = 1,
                .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
            },
        };

        VkDescriptorSetLayoutCreateInfo SetLayoutInfo =
        {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
            .flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT,
            .bindingCount = ArrayCount(Bindings),
            .pBindings = Bindings,
        };

        VulkanCheck(vkCreateDescriptorSetLayout(Vulkan.Device, &SetLayoutInfo, 0, &Pipeline.SetLayout));
    }

    // NOTE(vak): Pipeline layout
    {
        VkPipelineLayoutCreateInfo LayoutInfo =
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            .setLayoutCount = 1,
            .pSetLayouts = &Pipeline.SetLayout,
        };

        VulkanCheck(vkCreatePipelineLayout(Vulkan.Device, &LayoutInfo, 0, &Pipeline.Layout));
    }

    // NOTE(vak): Pipeline
    {
        VkPipelineShaderStageCreateInfo Stages[] =
        {
            {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                .stage = VK_SHADER_STAGE_VERTEX_BIT,
                .module = VertexShader,
                .pName = "main",
            },
            {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
                .module = FragmentShader,
                .pName = "main",
            },
        };

        VkPipelineVertexInputStateCreateInfo VertexInputState =
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        };

        VkPipelineInputAssemblyStateCreateInfo InputAssemblyState =
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
            .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        };

        VkPipelineTessellationStateCreateInfo TessellationState =
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO,
        };

        VkPipelineViewportStateCreateInfo ViewportState =
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
            .viewportCount = 1,
            .pViewports = &(VkViewport){0},
            .scissorCount = 1,
            .pScissors = &(VkRect2D){0},
        };

        VkPipelineRasterizationStateCreateInfo RasterizationState =
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
            .polygonMode = VK_POLYGON_MODE_FILL,
            .cullMode = VK_CULL_MODE_BACK_BIT,
            .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
            .lineWidth = 1.0f,
        };

        VkPipelineMultisampleStateCreateInfo MultisampleState =
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
            .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
        };

        VkPipelineDepthStencilStateCreateInfo DepthStencilState =
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        };

        VkPipelineColorBlendStateCreateInfo ColorBlendState =
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
            .attachmentCount = 1,
            .pAttachments = &(VkPipelineColorBlendAttachmentState)
            {
                .blendEnable = VK_TRUE,
                .srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
                .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
                .colorBlendOp = VK_TRUE,
                .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
                .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
                .alphaBlendOp = VK_TRUE,
                .colorWriteMask =
                    VK_COLOR_COMPONENT_R_BIT|
                    VK_COLOR_COMPONENT_G_BIT|
                    VK_COLOR_COMPONENT_B_BIT|
                    VK_COLOR_COMPONENT_A_BIT,
            },
        };

        VkDynamicState DynamicStates[] =
        {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR,
        };

        VkPipelineDynamicStateCreateInfo DynamicState =
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
            .dynamicStateCount = ArrayCount(DynamicStates),
            .pDynamicStates = DynamicStates,
        };

        VkPipelineRenderingCreateInfo RenderingInfo =
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
            .colorAttachmentCount = 1,
            .pColorAttachmentFormats = &Vulkan.Swapchain.Format,
        };

        VkGraphicsPipelineCreateInfo PipelineInfo =
        {
            .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
            .pNext = &RenderingInfo,
            .stageCount = ArrayCount(Stages),
            .pStages = Stages,
            .pVertexInputState = &VertexInputState,
            .pInputAssemblyState = &InputAssemblyState,
            .pTessellationState = &TessellationState,
            .pViewportState = &ViewportState,
            .pRasterizationState = &RasterizationState,
            .pMultisampleState = &MultisampleState,
            .pDepthStencilState = &DepthStencilState,
            .pColorBlendState = &ColorBlendState,
            .pDynamicState = &DynamicState,
            .layout = Pipeline.Layout,
        };

        VulkanCheck(vkCreateGraphicsPipelines(Vulkan.Device, 0, 1, &PipelineInfo, 0, &Pipeline.Pipeline));
    }

    // NOTE(vak): Cleanup
    {
        vkDestroyShaderModule(Vulkan.Device, FragmentShader, 0);
        vkDestroyShaderModule(Vulkan.Device, VertexShader, 0);
    }

    return (Pipeline);
}

local void VulkanDeletePipeline(vulkan_pipeline* Pipeline)
{
    if (Pipeline->SetLayout)
        vkDestroyDescriptorSetLayout(Vulkan.Device, Pipeline->SetLayout, 0);

    if (Pipeline->Layout)
        vkDestroyPipelineLayout(Vulkan.Device, Pipeline->Layout, 0);

    if (Pipeline->Pipeline)
        vkDestroyPipeline(Vulkan.Device, Pipeline->Pipeline, 0);

    ZeroType(Pipeline);
}

local u32 VulkanSelectMemoryType(u32 MemoryTypeBits, VkMemoryPropertyFlags PropertyFlags)
{
    u32 MemoryTypeIndex = U32Max;

    VkPhysicalDeviceMemoryProperties MemoryProperties = {0};
    vkGetPhysicalDeviceMemoryProperties(Vulkan.PhysicalDevice, &MemoryProperties);

    for (u32 Index = 0; Index < MemoryProperties.memoryTypeCount; Index++)
    {
        VkMemoryType MemoryType = MemoryProperties.memoryTypes[Index];

        if ((MemoryTypeBits & (1 << Index)) == 0)
            continue;

        if ((MemoryType.propertyFlags & PropertyFlags) != PropertyFlags)
            continue;

        MemoryTypeIndex = Index;
        break;
    }

    AlwaysAssert(MemoryTypeIndex != U32Max);

    return (MemoryTypeIndex);
}

local vulkan_buffer VulkanCreateBuffer(
    usize Size,
    VkBufferUsageFlags Usage,
    VkMemoryPropertyFlags MemoryProperties,
    b32 Mapped
)
{
    vulkan_buffer Buffer = {0};

    Buffer.Size = Size;

    VkBufferCreateInfo BufferInfo =
    {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = Size,
        .usage = Usage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };

    VulkanCheck(vkCreateBuffer(Vulkan.Device, &BufferInfo, 0, &Buffer.Buffer));

    VkMemoryRequirements MemoryRequirements = {0};
    vkGetBufferMemoryRequirements(Vulkan.Device, Buffer.Buffer, &MemoryRequirements);

    VkMemoryAllocateInfo AllocateInfo =
    {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = MemoryRequirements.size,
        .memoryTypeIndex = VulkanSelectMemoryType(MemoryRequirements.memoryTypeBits, MemoryProperties),
    };

    VulkanCheck(vkAllocateMemory(Vulkan.Device, &AllocateInfo, 0, &Buffer.Memory));
    VulkanCheck(vkBindBufferMemory(Vulkan.Device, Buffer.Buffer, Buffer.Memory, 0));

    if (Mapped)
    {
        VulkanCheck(vkMapMemory(
            Vulkan.Device,
            Buffer.Memory,
            0,
            Buffer.Size,
            0,
            &Buffer.Mapping
        ));
    }

    return (Buffer);
}

local void VulkanUploadBuffer(
    vulkan_buffer* DestBuffer,
    vulkan_buffer* TransferBuffer,
    void* Data, usize Offset, usize Size)
{
    VkCommandBuffer CommandBuffer = Vulkan.CommandBuffers[0];

    // NOTE(vak): Checks
    {
        AlwaysAssert(Data);
        AlwaysAssert(TransferBuffer->Mapping);
        AlwaysAssert(Size <= TransferBuffer->Size);
        AlwaysAssert(Offset + Size <= DestBuffer->Size);
    }

    // NOTE(vak): Copy into transfer buffer
    {
        CopyMemory(TransferBuffer->Mapping, Data, Size);
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

    // NOTE(vak): Issue transfer
    {
        VkBufferCopy Region =
        {
            .srcOffset = 0,
            .dstOffset = Offset,
            .size = Size,
        };

        vkCmdCopyBuffer(
            CommandBuffer,
            TransferBuffer->Buffer,
            DestBuffer->Buffer,
            1,
            &Region
        );
    }

    // NOTE(vak): Stop recording commands
    {
        VulkanCheck(vkEndCommandBuffer(CommandBuffer));
    }

    // NOTE(vak): Submit commands
    {
        VkSubmitInfo SubmitInfo =
        {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .commandBufferCount = 1,
            .pCommandBuffers = &CommandBuffer,
        };

        VulkanCheck(vkQueueSubmit(Vulkan.Queue, 1, &SubmitInfo, 0));
    }

    VulkanCheck(vkQueueWaitIdle(Vulkan.Queue));
}

local void VulkanDeleteBuffer(vulkan_buffer* Buffer)
{
    if (Buffer->Mapping)
        vkUnmapMemory(Vulkan.Device, Buffer->Memory);

    if (Buffer->Memory)
        vkFreeMemory(Vulkan.Device, Buffer->Memory, 0);

    if (Buffer->Buffer)
        vkDestroyBuffer(Vulkan.Device, Buffer->Buffer, 0);

    ZeroType(Buffer);
}

local void VulkanDeleteRenderer(void);
local void VulkanRender(draw_rect* Rects, usize RectCount);

local void VulkanMakeRenderer(void)
{
    // NOTE(vak): Setup renderer functions
    {
        RendererFunctions.CurrentAPI     = RendererAPI_Vulkan;
        RendererFunctions.DeleteRenderer = &VulkanDeleteRenderer;
        RendererFunctions.Render         = &VulkanRender;
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
        Vulkan.VersionOfAPI = VK_API_VERSION_1_4;

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

        VkPhysicalDeviceVulkan14Features Vulkan14Features =
        {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_4_FEATURES,
            .pushDescriptor = true,
        };

        VkPhysicalDeviceVulkan13Features Vulkan13Features =
        {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
            .pNext = &Vulkan14Features,
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

    // NOTE(vak): Command buffers
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

    // NOTE(vak): Pipeline
    {
        u32 VertexCode[] =
        {
            #include "shaders/basic.vert.h"
        };

        u32 FragmentCode[] =
        {
            #include "shaders/basic.frag.h"
        };

        Vulkan.Pipeline = VulkanCreatePipeline(
            VertexCode, sizeof(VertexCode),
            FragmentCode, sizeof(FragmentCode)
        );
    }

    // NOTE(vak): Buffers
    {
        Vulkan.VertexBuffer = VulkanCreateBuffer(
            MB(128),
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT|
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|
            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            true
        );
    }

    // NOTE(vak): Synchronization
    {
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

    VulkanDeleteBuffer(&Vulkan.VertexBuffer);

    VulkanDeletePipeline(&Vulkan.Pipeline);

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

local void VulkanRender(draw_rect* Rects, usize RectCount)
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

    // NOTE(vak): Flight resources

    VkCommandBuffer CommandBuffer = Vulkan.CommandBuffers[Vulkan.FlightIndex];
    VkSemaphore AcquireSemaphore = Vulkan.AcquireSemaphores[Vulkan.FlightIndex];
    VkSemaphore SubmitSemaphore = Vulkan.SubmitSemaphores[Vulkan.FlightIndex];
    VkFence DrawFinishedFence = Vulkan.DrawFinishedFences[Vulkan.FlightIndex];

    // NOTE(vak): Wait for last draw utilizing this flight's resources
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

    // NOTE(vak): Write draw data
    u32 VertexCount = 0;

    {
        usize MaxVertexCount = Vulkan.VertexBuffer.Size / sizeof(vulkan_vertex);
        usize MaxRectCount   = MaxVertexCount / 6;

        AlwaysAssert(RectCount <= MaxRectCount);

        vulkan_vertex* Base = (vulkan_vertex*)Vulkan.VertexBuffer.Mapping;

        for (usize RectIndex = 0; RectIndex < RectCount; RectIndex++)
        {
            draw_rect* Rect = Rects + RectIndex;

            *Base++ = (vulkan_vertex){Rect->MinX, Rect->MinY, 0.0f, 0.0f, Rect->R, Rect->G, Rect->B, Rect->A};
            *Base++ = (vulkan_vertex){Rect->MaxX, Rect->MinY, 0.0f, 0.0f, Rect->R, Rect->G, Rect->B, Rect->A};
            *Base++ = (vulkan_vertex){Rect->MaxX, Rect->MaxY, 0.0f, 0.0f, Rect->R, Rect->G, Rect->B, Rect->A};

            *Base++ = (vulkan_vertex){Rect->MaxX, Rect->MaxY, 0.0f, 0.0f, Rect->R, Rect->G, Rect->B, Rect->A};
            *Base++ = (vulkan_vertex){Rect->MinX, Rect->MaxY, 0.0f, 0.0f, Rect->R, Rect->G, Rect->B, Rect->A};
            *Base++ = (vulkan_vertex){Rect->MinX, Rect->MinY, 0.0f, 0.0f, Rect->R, Rect->G, Rect->B, Rect->A};

            VertexCount += 6;
        }
    }

    // NOTE(vak): Issue draw
    {
        vulkan_pipeline* Pipeline = &Vulkan.Pipeline;

        vkCmdBindPipeline(CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, Pipeline->Pipeline);

        VkViewport Viewport =
        {
            .x = 0.0f,
            .y = (f32)Swapchain->SizeY,
            .width = (f32)Swapchain->SizeX,
            .height = -(f32)Swapchain->SizeY,
            .minDepth = 0.0f,
            .maxDepth = 1.0f,
        };

        VkRect2D Scissor =
        {
            {0, 0},
            {Swapchain->SizeX, Swapchain->SizeY},
        };

        vkCmdSetViewport(CommandBuffer, 0, 1, &Viewport);
        vkCmdSetScissor(CommandBuffer, 0, 1, &Scissor);

        VkWriteDescriptorSet DescriptorWrites[] =
        {
            {
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstBinding = 0,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                .pBufferInfo = &(VkDescriptorBufferInfo)
                {
                    .buffer = Vulkan.VertexBuffer.Buffer,
                    .offset = 0,
                    .range = Vulkan.VertexBuffer.Size,
                },
            },
        };

        vkCmdPushDescriptorSet(
            CommandBuffer,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            Pipeline->Layout,
            0,
            ArrayCount(DescriptorWrites),
            DescriptorWrites
        );

        vkCmdDraw(CommandBuffer, VertexCount, 1, 0, 0);
    }

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
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, // NOTE(vak): This Draw
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, // NOTE(vak): This Draw
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
        // NOTE(vak): Start waiting for the image to be acquired when we're
        // ready to output to the color attachment.
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
