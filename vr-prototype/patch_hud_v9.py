from pathlib import Path
import sys

root = Path(sys.argv[1])
src = root / "Samples/XrSamples/XrInput/Src/main.cpp"
code = src.read_text()

# Required math and text formatting headers.
code = code.replace(
    "#include <string>",
    "#include <cmath>\n#include <cstdio>\n#include <string>",
    1
)

# Replace full headset rotation with level, yaw-only rotation.
old = '''            const OVR::Posef head = FromXrPosef(headLocation.pose);

            const OVR::Vector3f panelPosition =
                head.Transform(
                    OVR::Vector3f{-0.18f, -0.12f, -0.85f});
            calibrationLabel_->SetLocalPose(
                OVR::Posef(head.Rotation, panelPosition));'''

new = '''            const OVR::Posef head = FromXrPosef(headLocation.pose);

            // Follow head yaw without copying pitch or roll.
            const OVR::Vector3f forward =
                head.Transform({0.f, 0.f, -1.f}) -
                head.Translation;

            const float yaw =
                std::atan2(-forward.x, -forward.z);

            const OVR::Quatf levelRotation =
                OVR::Quatf::FromRotationVector(
                    {0.f, yaw, 0.f});

            const OVR::Posef levelHead(
                levelRotation, head.Translation);

            const OVR::Vector3f worldPosition =
                levelHead.Transform(
                    {0.f, -0.12f, -1.0f});

            // TinyUI child position is relative to menu root.
            const OVR::Vector3f menuRoot{
                -0.35f, -0.15f, -1.1f};

            calibrationLabel_->SetLocalPose(
                OVR::Posef(
                    levelRotation,
                    worldPosition - menuRoot));'''

# Locate the actual Build 8 HUD code using stable markers.
start_marker = "            const OVR::Posef head = FromXrPosef(headLocation.pose);"
end_marker = "                OVR::Posef(head.Rotation, panelPosition));"

start = code.find(start_marker)
end = code.find(end_marker, start)

if start == -1 or end == -1:
    raise RuntimeError(
        "Could not locate the Build 8 HUD block. No files changed."
    )

code = code[:start] + new + code[end + len(end_marker):]


# Explicitly format fresh values for the display.
start = code.index("        calibrationLabel_->SetText(")
end_text = "gunPitch_, gunYaw_, gunRoll_);"
end = code.index(end_text, start) + len(end_text)

replacement = r'''        char calibrationText[512];

        std::snprintf(
            calibrationText,
            sizeof(calibrationText),
            "BLACK VR CALIBRATION\n"
            "MODE: %s | A presses: %d\n"
            "X: %.2f   Y: %.2f   Z: %.2f cm\n"
            "PITCH: %.2f   YAW: %.2f   ROLL: %.2f\n"
            "A: Switch Mode | Sticks: Adjust",
            calibrationMode_ == 0 ? "POSITION" : "ROTATION",
            calibrationToggleCount_,
            gunX_ * 100.f,
            gunY_ * 100.f,
            gunZ_ * 100.f,
            gunPitch_,
            gunYaw_,
            gunRoll_);

        calibrationLabel_->SetText(calibrationText);'''

code = code[:start] + replacement + code[end:]
src.write_text(code)

# Give Build 9 its own application ID.
gradle = root / "Samples/XrSamples/XrInput/Projects/Android/build.gradle"
s = gradle.read_text()

assert "com.xavier.blackvr.calibrationv8" in s

s = s.replace(
    "com.xavier.blackvr.calibrationv8",
    "com.xavier.blackvr.calibrationv9",
    1
)

gradle.write_text(s)

print("BLACK VR Build 9 HUD fix applied.")
