#pragma once

#include <openxr/openxr.h>
#include <vulkan/vulkan.h>
#include <vector>

namespace blackvr {

class RuntimeResources {
public:
    XrInstance instance = XR_NULL_HANDLE;
    XrSystemId systemId = XR_NULL_SYSTEM_ID;
    XrSession session = XR_NULL_HANDLE;
    XrSpace referenceSpace = XR_NULL_HANDLE;

    VkInstance vkInstance = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;

    uint32_t queueFamilyIndex = 0;
    uint32_t queueIndex = 0;

    std::vector<XrSwapchain> eyeSwapchains;

    RuntimeResources() = default;
    RuntimeResources(const RuntimeResources&) = delete;
    RuntimeResources& operator=(const RuntimeResources&) = delete;

    ~RuntimeResources();

    void Shutdown();
};

}
