#pragma once

#include <openxr/openxr.h>

namespace blackvr {

struct SessionLifecycle {
    XrSessionState state = XR_SESSION_STATE_UNKNOWN;
    bool running = false;
    bool exitRequested = false;

    XrResult Poll(XrInstance instance, XrSession session);
    XrResult SubmitEmptyFrame(XrSession session);
};

}
