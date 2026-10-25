from pathlib import Path
import sys

root = Path(sys.argv[1])
src = root / "Samples/XrSamples/XrInput/Src/main.cpp"
code = src.read_text()

# Keep the calibration display in front of the headset.
anchor = "        calibrationLabel_->SetText("
assert anchor in code, "Calibration text update not found"

hud = """
        XrSpaceLocation headLocation{XR_TYPE_SPACE_LOCATION};

        XrResult headResult = xrLocateSpace(
            GetHeadSpace(),
            GetCurrentSpace(),
            ToXrTime(in.PredictedDisplayTime),
            &headLocation);

        const XrSpaceLocationFlags required =
            XR_SPACE_LOCATION_POSITION_VALID_BIT |
            XR_SPACE_LOCATION_ORIENTATION_VALID_BIT;

        if (XR_SUCCEEDED(headResult) &&
            (headLocation.locationFlags & required) == required) {

            const OVR::Posef head = FromXrPosef(headLocation.pose);

            const OVR::Vector3f panelPosition =
                head.Transform(
                    OVR::Vector3f{-0.18f, -0.12f, -0.85f});

            calibrationLabel_->SetLocalPose(
                OVR::Posef(head.Rotation, panelPosition));
        }

"""

code = code.replace(anchor, hud + anchor, 1)

# Prevent cube geometry from hiding the display.
anchor = "            {1000.f, 300.f});"
assert anchor in code, "Calibration panel setup not found"

code = code.replace(
    anchor,
    anchor + """
        calibrationLabel_->AddFlags(
            OVRFW::VRMenuObjectFlags_t(
                OVRFW::VRMENUOBJECT_FLAG_NO_DEPTH));
""",
    1
)

src.write_text(code)

# Install separately from Build 7.
gradle = root / "Samples/XrSamples/XrInput/Projects/Android/build.gradle"
s = gradle.read_text()

assert "com.xavier.blackvr.calibrationv7" in s
s = s.replace(
    "com.xavier.blackvr.calibrationv7",
    "com.xavier.blackvr.calibrationv8",
    1
)

gradle.write_text(s)

print("Build 8 headset-following display patch applied.")
