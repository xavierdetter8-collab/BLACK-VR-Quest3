#include "openxr_session_lifecycle.h"
#include <android/log.h>

namespace blackvr {

XrResult SessionLifecycle::SubmitEmptyFrame(XrSession session) {
    if (!running)
        return XR_ERROR_SESSION_NOT_RUNNING;

    XrFrameWaitInfo waitInfo{
        XR_TYPE_FRAME_WAIT_INFO
    };

    XrFrameState frameState{
        XR_TYPE_FRAME_STATE
    };

    XrResult result = xrWaitFrame(
        session, &waitInfo, &frameState
    );

    if (XR_FAILED(result))
        return result;

    XrFrameBeginInfo beginInfo{
        XR_TYPE_FRAME_BEGIN_INFO
    };

    result = xrBeginFrame(session, &beginInfo);

    if (XR_FAILED(result))
        return result;

    XrFrameEndInfo endInfo{
        XR_TYPE_FRAME_END_INFO
    };

    endInfo.displayTime = frameState.predictedDisplayTime;
    endInfo.environmentBlendMode =
        XR_ENVIRONMENT_BLEND_MODE_OPAQUE;

    // No rendered images yet.
    endInfo.layerCount = 0;
    endInfo.layers = nullptr;

    return xrEndFrame(session, &endInfo);
}



XrResult SessionLifecycle::Poll(
    XrInstance instance,
    XrSession session
) {
    for (;;) {
        XrEventDataBuffer event{XR_TYPE_EVENT_DATA_BUFFER};

        XrResult result = xrPollEvent(instance, &event);

        if (result == XR_EVENT_UNAVAILABLE)
            return XR_SUCCESS;

        if (XR_FAILED(result))
            return result;

        if (event.type != XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED)
            continue;

        const auto* changed =
            reinterpret_cast<const XrEventDataSessionStateChanged*>(
                &event
            );

        if (changed->session != session)
            continue;

        state = changed->state;

        __android_log_print(
            ANDROID_LOG_INFO,
            "BLACKVR",
            "OpenXR session state: %d",
            static_cast<int>(state)
        );

        if (state == XR_SESSION_STATE_READY && !running) {
            XrSessionBeginInfo begin{
                XR_TYPE_SESSION_BEGIN_INFO
            };

            begin.primaryViewConfigurationType =
                XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;

            result = xrBeginSession(session, &begin);

            if (XR_FAILED(result))
                return result;

            running = true;
        }

        if (state == XR_SESSION_STATE_STOPPING && running) {
            result = xrEndSession(session);

            if (XR_FAILED(result))
                return result;

            running = false;
        }

        if (state == XR_SESSION_STATE_EXITING ||
            state == XR_SESSION_STATE_LOSS_PENDING) {
            running = false;
            exitRequested = true;
        }
    }
}

}
