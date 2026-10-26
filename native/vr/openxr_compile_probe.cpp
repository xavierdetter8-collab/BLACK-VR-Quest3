
#define XR_USE_PLATFORM_ANDROID 1
#define XR_USE_GRAPHICS_API_VULKAN 1

#include <jni.h>
#include <vector>
#include <cstring>
#include <cstdio>
#include <vulkan/vulkan.h>
#include <SDL3/SDL_system.h>
#include <android/log.h>
#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>
#include "openxr_runtime_resources.h"

extern "C" int blackvr_openxr_compile_probe() {
    JNIEnv* env =
        reinterpret_cast<JNIEnv*>(SDL_GetAndroidJNIEnv());

    jobject activity =
        reinterpret_cast<jobject>(SDL_GetAndroidActivity());

    if (!env || !activity) {
        __android_log_print(
            ANDROID_LOG_ERROR, "BLACKVR",
            "OpenXR: Android JNI unavailable"
        );
        return -1;
    }

    JavaVM* vm = nullptr;

    if (env->GetJavaVM(&vm) != JNI_OK || !vm) {
        env->DeleteLocalRef(activity);
        return -2;
    }

    PFN_xrInitializeLoaderKHR initializeLoader = nullptr;

    XrResult result = xrGetInstanceProcAddr(
        XR_NULL_HANDLE,
        "xrInitializeLoaderKHR",
        reinterpret_cast<PFN_xrVoidFunction*>(
            &initializeLoader
        )
    );

    if (XR_FAILED(result) || !initializeLoader) {
        env->DeleteLocalRef(activity);
        return -3;
    }

    XrLoaderInitInfoAndroidKHR info{
        XR_TYPE_LOADER_INIT_INFO_ANDROID_KHR
    };

    info.applicationVM = vm;
    info.applicationContext = activity;

    __android_log_print(
        ANDROID_LOG_INFO,
        "BLACKVR",
        "Step 129: Testing OpenXR loader initialization"
    );

    XrLoaderInitInfoAndroidKHR testInfo{
        XR_TYPE_LOADER_INIT_INFO_ANDROID_KHR
    };

    testInfo.applicationVM = vm;
    // Keep a valid Android context for later OpenXR runtime discovery.
    static jobject xrPersistentContext = nullptr;

    if (!xrPersistentContext) {
        xrPersistentContext = env->NewGlobalRef(activity);

        if (!xrPersistentContext) {
            env->DeleteLocalRef(activity);
            return -1310;
        }
    }

    testInfo.applicationContext = xrPersistentContext;

    __android_log_print(
        ANDROID_LOG_INFO,
        "BLACKVR",
        "STEP 129: Calling xrInitializeLoaderKHR"
    );

    XrResult testResult = initializeLoader(
        reinterpret_cast<const XrLoaderInitInfoBaseHeaderKHR*>(
            &testInfo
        )
    );

    __android_log_print(
        ANDROID_LOG_INFO,
        "BLACKVR",
        "STEP 129: Loader returned %d",
        testResult
    );

    env->DeleteLocalRef(activity);

    if (XR_FAILED(testResult)) {
        return static_cast<int>(testResult);
    }

    uint32_t extensionCount = 0;

    XrResult extensionResult =
        xrEnumerateInstanceExtensionProperties(
            nullptr,
            0,
            &extensionCount,
            nullptr
        );

    __android_log_print(
        ANDROID_LOG_INFO,
        "BLACKVR",
        "STEP 130: Extension enumeration result %d, count %u",
        extensionResult,
        extensionCount
    );


    if (XR_FAILED(extensionResult))
        return static_cast<int>(extensionResult);

    // STEP 132: Verify the Android instance extension.
    std::vector<XrExtensionProperties> extensions(extensionCount);

    for (auto& ext : extensions) {
        ext.type = XR_TYPE_EXTENSION_PROPERTIES;
        ext.next = nullptr;
    }

    XrResult listResult = xrEnumerateInstanceExtensionProperties(
        nullptr,
        extensionCount,
        &extensionCount,
        extensions.data()
    );

    if (XR_FAILED(listResult))
        return static_cast<int>(listResult);

    bool hasAndroidExtension = false;
    bool hasVulkanExtension = false;

    for (const auto& ext : extensions) {
        if (std::strcmp(
                ext.extensionName,
                XR_KHR_ANDROID_CREATE_INSTANCE_EXTENSION_NAME) == 0) {
            hasAndroidExtension = true;
        }
    }

    for (const auto& ext : extensions) {
        if (std::strcmp(
                ext.extensionName,
                XR_KHR_VULKAN_ENABLE2_EXTENSION_NAME) == 0) {
            hasVulkanExtension = true;
        }
    }

    if (!hasVulkanExtension)
        return -1351;

    if (!hasAndroidExtension)
        return -1321;

    XrInstanceCreateInfoAndroidKHR androidInfo{
        XR_TYPE_INSTANCE_CREATE_INFO_ANDROID_KHR
    };

    androidInfo.applicationVM = vm;
    androidInfo.applicationActivity = xrPersistentContext;

    XrInstanceCreateInfo createInfo{
        XR_TYPE_INSTANCE_CREATE_INFO
    };

    createInfo.next = &androidInfo;

    std::snprintf(
        createInfo.applicationInfo.applicationName,
        XR_MAX_APPLICATION_NAME_SIZE,
        "%s",
        "BLACK VR Dev"
    );

    createInfo.applicationInfo.applicationVersion = 1;

    std::snprintf(
        createInfo.applicationInfo.engineName,
        XR_MAX_ENGINE_NAME_SIZE,
        "%s",
        "BLACKVR"
    );

    createInfo.applicationInfo.engineVersion = 1;
    createInfo.applicationInfo.apiVersion = XR_CURRENT_API_VERSION;

    const char* requiredExtensions[] = {
        XR_KHR_ANDROID_CREATE_INSTANCE_EXTENSION_NAME,
        XR_KHR_VULKAN_ENABLE2_EXTENSION_NAME
    };

    createInfo.enabledExtensionCount = 2;
    createInfo.enabledExtensionNames = requiredExtensions;

    XrInstance instance = XR_NULL_HANDLE;

    XrResult createResult = xrCreateInstance(
        &createInfo,
        &instance
    );

    __android_log_print(
        XR_SUCCEEDED(createResult)
            ? ANDROID_LOG_INFO
            : ANDROID_LOG_ERROR,
        "BLACKVR",
        "STEP 132 xrCreateInstance result: %d",
        createResult
    );

    if (XR_FAILED(createResult))
        return static_cast<int>(createResult);

    // STEP 133: Find the active Quest VR system.
    XrSystemGetInfo systemInfo{XR_TYPE_SYSTEM_GET_INFO};
    systemInfo.formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;

    XrSystemId systemId = XR_NULL_SYSTEM_ID;

    XrResult systemResult = xrGetSystem(
        instance,
        &systemInfo,
        &systemId
    );

    __android_log_print(
        XR_SUCCEEDED(systemResult)
            ? ANDROID_LOG_INFO
            : ANDROID_LOG_ERROR,
        "BLACKVR",
        "STEP 133: xrGetSystem result=%d, systemId=%llu",
        systemResult,
        static_cast<unsigned long long>(systemId)
    );

    // STEP 134: Verify stereoscopic view support.
    int resultCode = -134;

    if (XR_FAILED(systemResult)) {
        resultCode = static_cast<int>(systemResult);
    } else if (systemId == XR_NULL_SYSTEM_ID) {
        resultCode = -1331;
    } else {
        uint32_t configCount = 0;

        XrResult r = xrEnumerateViewConfigurations(
            instance, systemId, 0, &configCount, nullptr
        );

        if (XR_FAILED(r)) {
            resultCode = static_cast<int>(r);
        } else {
            std::vector<XrViewConfigurationType> configs(configCount);

            r = xrEnumerateViewConfigurations(
                instance, systemId, configCount,
                &configCount, configs.data()
            );

            if (XR_FAILED(r)) {
                resultCode = static_cast<int>(r);
            } else {
                bool stereo = false;

                for (auto config : configs) {
                    if (config == XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO)
                        stereo = true;
                }

                if (!stereo) {
                    resultCode = -1341;
                } else {
                    uint32_t viewCount = 0;

                    r = xrEnumerateViewConfigurationViews(
                        instance,
                        systemId,
                        XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO,
                        0,
                        &viewCount,
                        nullptr
                    );

                    if (XR_FAILED(r))
                        resultCode = static_cast<int>(r);
                    else if (viewCount != 2)
                        resultCode = -1342;
                }
            }
        }
    }

    __android_log_print(
        ANDROID_LOG_INFO,
        "BLACKVR",
        "STEP 134 stereo detection result: %d",
        resultCode
    );

    // STEP 135: Query Vulkan requirements for this XR system.
    if (resultCode == -134) {
        PFN_xrGetVulkanGraphicsRequirements2KHR getRequirements = nullptr;

        XrResult procResult = xrGetInstanceProcAddr(
            instance,
            "xrGetVulkanGraphicsRequirements2KHR",
            reinterpret_cast<PFN_xrVoidFunction*>(&getRequirements)
        );

        if (XR_FAILED(procResult)) {
            resultCode = static_cast<int>(procResult);
        } else if (!getRequirements) {
            resultCode = -1352;
        } else {
            XrGraphicsRequirementsVulkan2KHR requirements{
                XR_TYPE_GRAPHICS_REQUIREMENTS_VULKAN_KHR
            };

            XrResult r = getRequirements(
                instance, systemId, &requirements
            );

            if (XR_FAILED(r)) {
                resultCode = static_cast<int>(r);
            } else {
                __android_log_print(
                    ANDROID_LOG_INFO,
                    "BLACKVR",
                    "STEP 135 Vulkan min=%llu.%llu max=%llu.%llu",
                    (unsigned long long)XR_VERSION_MAJOR(
                        requirements.minApiVersionSupported),
                    (unsigned long long)XR_VERSION_MINOR(
                        requirements.minApiVersionSupported),
                    (unsigned long long)XR_VERSION_MAJOR(
                        requirements.maxApiVersionSupported),
                    (unsigned long long)XR_VERSION_MINOR(
                        requirements.maxApiVersionSupported)
                );

                resultCode = -135;
            }
        }
    }

    // STEP 136: Verify OpenXR Vulkan entry points.
    if (resultCode == -135) {
        const char* functions[] = {
            "xrCreateVulkanInstanceKHR",
            "xrGetVulkanGraphicsDevice2KHR",
            "xrCreateVulkanDeviceKHR"
        };

        for (const char* name : functions) {
            PFN_xrVoidFunction function = nullptr;

            XrResult lookupResult = xrGetInstanceProcAddr(
                instance, name, &function
            );

            __android_log_print(
                ANDROID_LOG_INFO,
                "BLACKVR",
                "STEP 136: %s result=%d found=%d",
                name,
                lookupResult,
                function != nullptr
            );

            if (XR_FAILED(lookupResult)) {
                resultCode = static_cast<int>(lookupResult);
                break;
            }

            if (!function) {
                resultCode = -1361;
                break;
            }
        }

        if (resultCode == -135)
            resultCode = -136;
    }


    // STEP 139: Create a temporary OpenXR-compatible Vulkan
    // instance and identify the headset's required GPU.
    if (resultCode == -136) {
        PFN_xrCreateVulkanInstanceKHR createVk = nullptr;

        XrResult lookup = xrGetInstanceProcAddr(
            instance,
            "xrCreateVulkanInstanceKHR",
            reinterpret_cast<PFN_xrVoidFunction*>(&createVk)
        );

        if (XR_FAILED(lookup)) {
            resultCode = static_cast<int>(lookup);
        } else if (!createVk) {
            resultCode = -1391;
        } else {
            VkApplicationInfo appInfo{
                VK_STRUCTURE_TYPE_APPLICATION_INFO
            };

            appInfo.pApplicationName = "BLACK VR GPU Test";
            appInfo.apiVersion = VK_API_VERSION_1_1;

            VkInstanceCreateInfo vkInfo{
                VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO
            };

            vkInfo.pApplicationInfo = &appInfo;

            XrVulkanInstanceCreateInfoKHR xrVkInfo{
                XR_TYPE_VULKAN_INSTANCE_CREATE_INFO_KHR
            };

            xrVkInfo.systemId = systemId;
            xrVkInfo.pfnGetInstanceProcAddr =
                vkGetInstanceProcAddr;
            xrVkInfo.vulkanCreateInfo = &vkInfo;

            VkInstance vkInstance = VK_NULL_HANDLE;
            VkResult vkResult = VK_SUCCESS;

            XrResult createResult = createVk(
                instance,
                &xrVkInfo,
                &vkInstance,
                &vkResult
            );

            __android_log_print(
                ANDROID_LOG_INFO,
                "BLACKVR",
                "STEP 139: Vulkan create XR=%d VK=%d",
                createResult,
                vkResult
            );

            if (XR_FAILED(createResult)) {
                resultCode = static_cast<int>(createResult);
            } else if (vkResult != VK_SUCCESS ||
                       vkInstance == VK_NULL_HANDLE) {
                resultCode = -1392;
            } else {
                PFN_xrGetVulkanGraphicsDevice2KHR getGpu =
                    nullptr;

                XrResult gpuLookup = xrGetInstanceProcAddr(
                    instance,
                    "xrGetVulkanGraphicsDevice2KHR",
                    reinterpret_cast<PFN_xrVoidFunction*>(&getGpu)
                );

                if (XR_FAILED(gpuLookup)) {
                    resultCode = static_cast<int>(gpuLookup);
                } else if (!getGpu) {
                    resultCode = -1393;
                } else {
                    XrVulkanGraphicsDeviceGetInfoKHR gpuInfo{
                        XR_TYPE_VULKAN_GRAPHICS_DEVICE_GET_INFO_KHR
                    };

                    gpuInfo.systemId = systemId;
                    gpuInfo.vulkanInstance = vkInstance;

                    VkPhysicalDevice gpu = VK_NULL_HANDLE;

                    XrResult gpuResult = getGpu(
                        instance,
                        &gpuInfo,
                        &gpu
                    );

                    if (XR_FAILED(gpuResult)) {
                        resultCode = static_cast<int>(gpuResult);
                    } else if (gpu == VK_NULL_HANDLE) {
                        resultCode = -1394;
                    } else {
                        VkPhysicalDeviceProperties props{};
                        vkGetPhysicalDeviceProperties(gpu, &props);

                        __android_log_print(
                            ANDROID_LOG_INFO,
                            "BLACKVR",
                            "STEP 139 GPU: %s vendor=%u device=%u",
                            props.deviceName,
                            props.vendorID,
                            props.deviceID
                        );

                        // STEP 140: Create a temporary Vulkan device.
                        PFN_xrCreateVulkanDeviceKHR createDevice = nullptr;

                        XrResult lookupDevice = xrGetInstanceProcAddr(
                            instance,
                            "xrCreateVulkanDeviceKHR",
                            reinterpret_cast<PFN_xrVoidFunction*>(
                                &createDevice
                            )
                        );

                        if (XR_FAILED(lookupDevice)) {
                            resultCode = static_cast<int>(lookupDevice);
                        } else if (!createDevice) {
                            resultCode = -1401;
                        } else {
                            uint32_t familyCount = 0;

                            vkGetPhysicalDeviceQueueFamilyProperties(
                                gpu, &familyCount, nullptr
                            );

                            std::vector<VkQueueFamilyProperties> families(
                                familyCount
                            );

                            if (familyCount) {
                                vkGetPhysicalDeviceQueueFamilyProperties(
                                    gpu, &familyCount, families.data()
                                );
                            }

                            uint32_t graphicsFamily = familyCount;

                            for (uint32_t i = 0; i < familyCount; ++i) {
                                if (families[i].queueCount > 0 &&
                                    (families[i].queueFlags &
                                     VK_QUEUE_GRAPHICS_BIT)) {
                                    graphicsFamily = i;
                                    break;
                                }
                            }

                            if (graphicsFamily == familyCount) {
                                resultCode = -1402;
                            } else {
                                float priority = 1.0f;

                                VkDeviceQueueCreateInfo queueInfo{
                                    VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO
                                };

                                queueInfo.queueFamilyIndex = graphicsFamily;
                                queueInfo.queueCount = 1;
                                queueInfo.pQueuePriorities = &priority;

                                VkDeviceCreateInfo deviceInfo{
                                    VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO
                                };

                                deviceInfo.queueCreateInfoCount = 1;
                                deviceInfo.pQueueCreateInfos = &queueInfo;

                                XrVulkanDeviceCreateInfoKHR xrDeviceInfo{
                                    XR_TYPE_VULKAN_DEVICE_CREATE_INFO_KHR
                                };

                                xrDeviceInfo.systemId = systemId;
                                xrDeviceInfo.pfnGetInstanceProcAddr =
                                    vkGetInstanceProcAddr;
                                xrDeviceInfo.vulkanPhysicalDevice = gpu;
                                xrDeviceInfo.vulkanCreateInfo = &deviceInfo;

                                // STEP 161: Own the Vulkan device.
                                blackvr::RuntimeResources deviceResources;
                                VkDevice device = VK_NULL_HANDLE;
                                VkResult vkDeviceResult =
                                    VK_ERROR_INITIALIZATION_FAILED;

                                XrResult xrDeviceResult = createDevice(
                                    instance,
                                    &xrDeviceInfo,
                                    &device,
                                    &vkDeviceResult
                                );

                                // Take ownership even if creation reports
                                // an error after returning a device handle.
                                deviceResources.device = device;

                                __android_log_print(
                                    ANDROID_LOG_INFO,
                                    "BLACKVR",
                                    "STEP 140 device XR=%d VK=%d queue=%u",
                                    xrDeviceResult,
                                    vkDeviceResult,
                                    graphicsFamily
                                );

                                if (XR_FAILED(xrDeviceResult)) {
                                    resultCode =
                                        static_cast<int>(xrDeviceResult);
                                } else if (
                                    vkDeviceResult != VK_SUCCESS ||
                                    device == VK_NULL_HANDLE
                                ) {
                                    resultCode = -1403;
                                } else {
                                    // STEP 141: Test OpenXR session creation.
                                    XrGraphicsBindingVulkan2KHR binding{
                                        XR_TYPE_GRAPHICS_BINDING_VULKAN_KHR
                                    };

                                    binding.instance = vkInstance;
                                    binding.physicalDevice = gpu;
                                    binding.device = device;
                                    binding.queueFamilyIndex = graphicsFamily;
                                    binding.queueIndex = 0;

                                    XrSessionCreateInfo sessionInfo{
                                        XR_TYPE_SESSION_CREATE_INFO
                                    };

                                    sessionInfo.next = &binding;
                                    sessionInfo.systemId = systemId;

                                    XrSession session = XR_NULL_HANDLE;

                                    XrResult sessionResult = xrCreateSession(
                                        instance,
                                        &sessionInfo,
                                        &session
                                    );

                                    __android_log_print(
                                        XR_SUCCEEDED(sessionResult)
                                            ? ANDROID_LOG_INFO
                                            : ANDROID_LOG_ERROR,
                                        "BLACKVR",
                                        "STEP 141 xrCreateSession: %d",
                                        sessionResult
                                    );

                                    if (XR_FAILED(sessionResult)) {
                                        resultCode =
                                            static_cast<int>(sessionResult);
                                    } else {
                                        blackvr::RuntimeResources sessionResources;
                                        sessionResources.session = session;

                                        // STEP 142: Enumerate VR swapchain formats.
                                        uint32_t formatCount = 0;

                                        XrResult r = xrEnumerateSwapchainFormats(
                                            session, 0, &formatCount, nullptr
                                        );

                                        if (XR_FAILED(r)) {
                                            resultCode = static_cast<int>(r);
                                        } else if (formatCount == 0) {
                                            resultCode = -1421;
                                        } else {
                                            std::vector<int64_t> formats(
                                                formatCount
                                            );

                                            r = xrEnumerateSwapchainFormats(
                                                session,
                                                formatCount,
                                                &formatCount,
                                                formats.data()
                                            );

                                            if (XR_FAILED(r)) {
                                                resultCode = static_cast<int>(r);
                                            } else {
                                                bool colorSupported = false;

                                                for (int64_t format : formats) {
                                                    if (
                                                        format == VK_FORMAT_R8G8B8A8_SRGB ||
                                                        format == VK_FORMAT_B8G8R8A8_SRGB ||
                                                        format == VK_FORMAT_R8G8B8A8_UNORM ||
                                                        format == VK_FORMAT_B8G8R8A8_UNORM
                                                    ) {
                                                        colorSupported = true;
                                                    }
                                                }

                                                __android_log_print(
                                                    ANDROID_LOG_INFO,
                                                    "BLACKVR",
                                                    "STEP 142 formats=%u compatible=%d",
                                                    formatCount,
                                                    colorSupported
                                                );

                                                resultCode = -1422;

                                                if (colorSupported) {
                                                    int64_t selectedFormat = 0;

                                                    for (int64_t format : formats) {
                                                        if (
                                                            format == VK_FORMAT_R8G8B8A8_SRGB ||
                                                            format == VK_FORMAT_B8G8R8A8_SRGB ||
                                                            format == VK_FORMAT_R8G8B8A8_UNORM ||
                                                            format == VK_FORMAT_B8G8R8A8_UNORM
                                                        ) {
                                                            selectedFormat = format;
                                                            break;
                                                        }
                                                    }

                                                    uint32_t viewCount = 0;

                                                    XrResult viewResult =
                                                        xrEnumerateViewConfigurationViews(
                                                            instance,
                                                            systemId,
                                                            XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO,
                                                            0,
                                                            &viewCount,
                                                            nullptr
                                                        );

                                                    if (XR_FAILED(viewResult)) {
                                                        resultCode = static_cast<int>(viewResult);
                                                    } else if (viewCount != 2) {
                                                        resultCode = -1431;
                                                    } else {
                                                        std::vector<XrViewConfigurationView>
                                                            views(viewCount);

                                                        for (auto& view : views) {
                                                            view.type =
                                                                XR_TYPE_VIEW_CONFIGURATION_VIEW;
                                                        }

                                                        viewResult =
                                                            xrEnumerateViewConfigurationViews(
                                                                instance,
                                                                systemId,
                                                                XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO,
                                                                viewCount,
                                                                &viewCount,
                                                                views.data()
                                                            );

                                                        if (XR_FAILED(viewResult)) {
                                                            resultCode =
                                                                static_cast<int>(viewResult);
                                                        } else {
                                                            // Session resources are owned by the outer scope.
                                                            resultCode = -144;

                                                            for (uint32_t eye = 0;
                                                                 eye < viewCount;
                                                                 ++eye) {
                                                                XrSwapchainCreateInfo info{
                                                                    XR_TYPE_SWAPCHAIN_CREATE_INFO
                                                                };

                                                                info.usageFlags =
                                                                    XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT;

                                                                info.format = selectedFormat;
                                                                info.sampleCount = 1;
                                                                info.width =
                                                                    views[eye].recommendedImageRectWidth;
                                                                info.height =
                                                                    views[eye].recommendedImageRectHeight;
                                                                info.faceCount = 1;
                                                                info.arraySize = 1;
                                                                info.mipCount = 1;

                                                                XrSwapchain swapchain =
                                                                    XR_NULL_HANDLE;

                                                                XrResult createResult =
                                                                    xrCreateSwapchain(
                                                                        session,
                                                                        &info,
                                                                        &swapchain
                                                                    );

                                                                if (XR_FAILED(createResult)) {
                                                                    resultCode =
                                                                        static_cast<int>(createResult);
                                                                    break;
                                                                }

                                                                sessionResources.eyeSwapchains.push_back(swapchain);

                                                                uint32_t imageCount = 0;

                                                                XrResult imageResult =
                                                                    xrEnumerateSwapchainImages(
                                                                        swapchain,
                                                                        0,
                                                                        &imageCount,
                                                                        nullptr
                                                                    );

                                                                if (XR_FAILED(imageResult)) {
                                                                    resultCode =
                                                                        static_cast<int>(imageResult);
                                                                    break;
                                                                }

                                                                if (imageCount == 0) {
                                                                    resultCode = -1432;
                                                                    break;
                                                                }

                                                                // STEP 144: Retrieve Vulkan eye images.
                                                                std::vector<XrSwapchainImageVulkan2KHR>
                                                                    images(imageCount);

                                                                for (auto& image : images) {
                                                                    image.type =
                                                                        XR_TYPE_SWAPCHAIN_IMAGE_VULKAN_KHR;
                                                                    image.next = nullptr;
                                                                }

                                                                uint32_t returnedCount = 0;

                                                                XrResult fetchResult =
                                                                    xrEnumerateSwapchainImages(
                                                                        swapchain,
                                                                        imageCount,
                                                                        &returnedCount,
                                                                        reinterpret_cast<
                                                                            XrSwapchainImageBaseHeader*>(
                                                                                images.data()
                                                                            )
                                                                    );

                                                                if (XR_FAILED(fetchResult)) {
                                                                    resultCode =
                                                                        static_cast<int>(fetchResult);
                                                                    break;
                                                                }

                                                                if (returnedCount == 0 ||
                                                                    returnedCount > imageCount) {
                                                                    resultCode = -1441;
                                                                    break;
                                                                }

                                                                bool validImages = true;

                                                                for (uint32_t i = 0;
                                                                     i < returnedCount;
                                                                     ++i) {
                                                                    if (images[i].image ==
                                                                        VK_NULL_HANDLE) {
                                                                        validImages = false;
                                                                        break;
                                                                    }
                                                                }

                                                                if (!validImages) {
                                                                    resultCode = -1442;
                                                                    break;
                                                                }

                                                                __android_log_print(
                                                                    ANDROID_LOG_INFO,
                                                                    "BLACKVR",
                                                                    "STEP 144 eye=%u valid Vulkan images=%u",
                                                                    eye,
                                                                    returnedCount
                                                                );

                                                                __android_log_print(
                                                                    ANDROID_LOG_INFO,
                                                                    "BLACKVR",
                                                                    "STEP 143 eye=%u size=%ux%u images=%u",
                                                                    eye,
                                                                    info.width,
                                                                    info.height,
                                                                    imageCount
                                                                );
                                                            }

                                                            // STEP 155: Release eye swapchains through
                                                            // their dedicated resource owner.
                                                            XrResult cleanup =
                                                                sessionResources.ReleaseSwapchains();

                                                            if (XR_FAILED(cleanup) &&
                                                                resultCode == -144) {
                                                                resultCode =
                                                                    static_cast<int>(cleanup);
                                                            }
                                                        }
                                                    }
                                                }
                                            }
                                        }

                                        // Release session before Vulkan cleanup.
                                        XrResult destroySessionResult =
                                            sessionResources.ReleaseSession();

                                        if (XR_FAILED(destroySessionResult)) {
                                            resultCode =
                                                static_cast<int>(
                                                    destroySessionResult
                                                );
                                        }
                                    }
                                }

                                // Release the Vulkan device only after
                                // the OpenXR session has been released.
                                deviceResources.Shutdown();
                            }
                        }
                    }
                }
            }

            if (vkInstance != VK_NULL_HANDLE) {
                vkDestroyInstance(vkInstance, nullptr);
            }
        }
    }

    XrResult destroyResult = xrDestroyInstance(instance);

    if (XR_FAILED(destroyResult))
        return static_cast<int>(destroyResult);

    return resultCode;


    result = initializeLoader(
        reinterpret_cast<
            const XrLoaderInitInfoBaseHeaderKHR*
        >(&info)
    );

    env->DeleteLocalRef(activity);

    if (XR_FAILED(result)) {
        __android_log_print(
            ANDROID_LOG_ERROR, "BLACKVR",
            "OpenXR loader initialization failed: %d",
            result
        );
        return static_cast<int>(result);
    }

    uint32_t count = 0;

    result = xrEnumerateInstanceExtensionProperties(
        nullptr, 0, &count, nullptr
    );

    __android_log_print(
        XR_SUCCEEDED(result)
            ? ANDROID_LOG_INFO
            : ANDROID_LOG_ERROR,
        "BLACKVR",
        "OpenXR extension enumeration: %d, count: %u",
        result, count
    );

    return XR_FAILED(result)
        ? static_cast<int>(result)
        : static_cast<int>(count);
}
