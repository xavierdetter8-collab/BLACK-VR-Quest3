#include "openxr_runtime_resources.h"

#include <android/log.h>

namespace blackvr {

RuntimeResources::~RuntimeResources() {
    Shutdown();
}

void RuntimeResources::Shutdown() {
    // Caller must finish outstanding GPU and XR frame work
    // before invoking Shutdown.

    if (referenceSpace != XR_NULL_HANDLE) {
        xrDestroySpace(referenceSpace);
        referenceSpace = XR_NULL_HANDLE;
    }

    for (auto it = eyeSwapchains.rbegin();
         it != eyeSwapchains.rend(); ++it) {
        if (*it != XR_NULL_HANDLE) {
            xrDestroySwapchain(*it);
        }
    }

    eyeSwapchains.clear();

    if (session != XR_NULL_HANDLE) {
        xrDestroySession(session);
        session = XR_NULL_HANDLE;
    }

    if (device != VK_NULL_HANDLE) {
        vkDestroyDevice(device, nullptr);
        device = VK_NULL_HANDLE;
    }

    physicalDevice = VK_NULL_HANDLE;

    if (vkInstance != VK_NULL_HANDLE) {
        vkDestroyInstance(vkInstance, nullptr);
        vkInstance = VK_NULL_HANDLE;
    }

    if (instance != XR_NULL_HANDLE) {
        xrDestroyInstance(instance);
        instance = XR_NULL_HANDLE;
    }

    systemId = XR_NULL_SYSTEM_ID;

    __android_log_print(
        ANDROID_LOG_INFO,
        "BLACKVR",
        "VR resource manager shutdown complete"
    );
}

}
