from pathlib import Path
import sys

root = Path(sys.argv[1])
src = root / "Samples/XrSamples/XrInput/Src/main.cpp"
code = src.read_text()

old = "            toolPose.Translation = toolPose.Transform({0.f, -0.025f, -0.06f});"
assert old in code, "Original gun pose not found"

new = r'''
            // BLACK VR: adjustable controller calibration.
            auto ls = GetActionStateVector2(actionRotateCube_);
            auto rx = GetActionStateFloat(actionScaleCube_);
            auto ry = GetActionStateFloat(actionTranslateCube_);
            auto mode = GetActionStateBoolean(actionToggleColor_);

            if (toolHeldInRight_) {
                if (mode.changedSinceLastSync && mode.currentState) {
                    calibrationMode_ = 1 - calibrationMode_;
                }

                float x = ls.isActive ? ls.currentState.x : 0.f;
                float y = ls.isActive ? ls.currentState.y : 0.f;
                float z = ry.isActive ? ry.currentState : 0.f;
                float r = rx.isActive ? rx.currentState : 0.f;
                float dt = in.DeltaSeconds;

                if (calibrationMode_ == 0) {
                    gunX_ += x * dt * 0.07f;
                    gunY_ += y * dt * 0.07f;
                    gunZ_ += z * dt * 0.07f;
                } else {
                    gunYaw_ += x * dt * 30.f;
                    gunPitch_ += y * dt * 30.f;
                    gunRoll_ += r * dt * 30.f;
                }

                auto limit = [](float v, float lo, float hi) {
                    return v < lo ? lo : (v > hi ? hi : v);
                };
                gunX_ = limit(gunX_, -0.15f, 0.15f);
                gunY_ = limit(gunY_, -0.15f, 0.15f);
                gunZ_ = limit(gunZ_, -0.25f, 0.10f);
                gunPitch_ = limit(gunPitch_, -45.f, 45.f);
                gunYaw_ = limit(gunYaw_, -45.f, 45.f);
                gunRoll_ = limit(gunRoll_, -45.f, 45.f);
            }

            toolPose.Rotation *= OVR::Quatf::FromRotationVector(
                {OVR::DegreeToRad(gunPitch_),
                 OVR::DegreeToRad(gunYaw_),
                 OVR::DegreeToRad(gunRoll_)});

            toolPose.Translation =
                toolPose.Transform({gunX_, gunY_, gunZ_});
'''

code = code.replace(old, new, 1)

anchor = "    float toolHitBox_{17_cm};"
assert anchor in code

fields = '''
    float gunX_{0.f}, gunY_{-0.025f}, gunZ_{-0.06f};
    float gunPitch_{0.f}, gunYaw_{0.f}, gunRoll_{0.f};
    int calibrationMode_{0};
    OVRFW::VRMenuObject* calibrationLabel_{nullptr};
'''
code = code.replace(anchor, fields + anchor, 1)

anchor = "    void SetupMenuPanels() {"
assert anchor in code

code = code.replace(
    anchor,
    anchor + '''
        calibrationLabel_ = ui_.AddLabel(
            "BLACK VR Calibration",
            {0_m, 0.7_m, -1.5_m},
            {850.f, 180.f});
''',
    1
)

anchor = "    void UpdateUI(const OVRFW::ovrApplFrameIn& in) {"
assert anchor in code

code = code.replace(
    anchor,
    anchor + r'''
        calibrationLabel_->SetText(
            "BLACK VR | %s | A: Switch mode\n"
            "Left stick: XY or Yaw/Pitch\n"
            "Right stick: Z or Roll\n"
            "XYZ cm: %.1f %.1f %.1f\n"
            "Pitch/Yaw/Roll: %.1f %.1f %.1f",
            calibrationMode_ == 0 ? "POSITION" : "ROTATION",
            gunX_ * 100.f, gunY_ * 100.f, gunZ_ * 100.f,
            gunPitch_, gunYaw_, gunRoll_);
''',
    1
)

code = code.replace(
    "BLACK VR: Aim Prototype",
    "BLACK VR: Calibration"
)

src.write_text(code)

gradle = root / "Samples/XrSamples/XrInput/Projects/Android/build.gradle"
s = gradle.read_text()
assert "com.xavier.blackvr.aimtest" in s
gradle.write_text(s.replace(
    "com.xavier.blackvr.aimtest",
    "com.xavier.blackvr.calibration"
))

print("BLACK VR controller calibration added.")
