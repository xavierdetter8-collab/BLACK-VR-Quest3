#include "openxr_runtime_resources.h"

#include <android/log.h>

namespace blackvr {

RuntimeResources::~RuntimeResources() {
    Shutdown();
}

XrResult RuntimeResources::ReleaseSwapchains() {
    XrResult firstError = XR_SUCCESS;

    for (auto it = eyeSwapchains.rbegin();
         it != eyeSwapchains.rend(); ++it) {
        if (*it == XR_NULL_HANDLE)
            continue;

        XrResult result = xrDestroySwapchain(*it);

        if (XR_FAILED(result) && XR_SUCCEEDED(firstError)) {
            firstError = result;
        }
    }

    eyeSwapchains.clear();

    if (XR_FAILED(firstError)) {
        __android_log_print(
            ANDROID_LOG_ERROR,
            "BLACKVR",
            "Swapchain cleanup failed: %d",
            firstError
        );
    }

    return firstError;
}


XrResult RuntimeResources::ReleaseSession() {
    if (session == XR_NULL_HANDLE)
        return XR_SUCCESS;

    if (!eyeSwapchains.empty()) {
        XrResult result = ReleaseSwapchains();
        if (XR_FAILED(result))
            return result;
    }

    XrResult result = xrDestroySession(session);

    if (XR_SUCCEEDED(result)) {
        session = XR_NULL_HANDLE;
    } else {
        __android_log_print(
            ANDROID_LOG_ERROR,
            "BLACKVR",
            "Session cleanup failed: %d",
            result
        );
    }

    return result;
}

void RuntimeResources::Shutdown() {
    // Caller must finish outstanding GPU and XR frame work
    // before invoking Shutdown.

    if (referenceSpace != XR_NULL_HANDLE) {
        xrDestroySpace(referenceSpace);
        referenceSpace = XR_NULL_HANDLE;
    }

    ReleaseSwapchains();

    ReleaseSession();

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
