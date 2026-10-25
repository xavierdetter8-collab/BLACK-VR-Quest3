from pathlib import Path
import sys

root = Path(sys.argv[1])
src = root / "Samples/XrSamples/XrInput/Src/main.cpp"
code = src.read_text()

# Create a dedicated calibration button action.
anchor = "        actionToggleColor_ = CreateAction("
assert anchor in code

code = code.replace(
    anchor,
    '''        actionCalibrationMode_ = CreateAction(
            actionSetWorld_,
            XR_ACTION_TYPE_BOOLEAN_INPUT,
            "calibration_mode",
            "Switch Calibration Mode");
''' + anchor,
    1
)

# Map the right controller A button to calibration.
assert code.count("{actionToggleColor_,") == 2
code = code.replace(
    "{actionToggleColor_,",
    "{actionCalibrationMode_,"
)

# Register the new action.
anchor = "    XrAction actionToggleColor_{XR_NULL_HANDLE};"
assert anchor in code
code = code.replace(
    anchor,
    anchor + "\n    XrAction actionCalibrationMode_{XR_NULL_HANDLE};",
    1
)

# Read the new button action.
anchor = "auto mode = GetActionStateBoolean(actionToggleColor_);"
assert anchor in code
code = code.replace(
    anchor,
    "auto mode = GetActionStateBoolean(actionCalibrationMode_);",
    1
)

# Count A-button presses for easier testing.
anchor = "int calibrationMode_{0};"
assert anchor in code
code = code.replace(
    anchor,
    anchor + "\n    int calibrationToggleCount_{0};",
    1
)

anchor = "calibrationMode_ = 1 - calibrationMode_;"
assert anchor in code
code = code.replace(
    anchor,
    anchor + "\n                    ++calibrationToggleCount_;",
    1
)

# Make the calibration display more visible.
assert "{0_m, 0.7_m, -1.5_m}," in code
code = code.replace(
    "{0_m, 0.7_m, -1.5_m},",
    "{0_m, 1.1_m, -1.2_m},",
    1
)

assert "{850.f, 180.f}" in code
code = code.replace(
    "{850.f, 180.f}",
    "{1000.f, 300.f}",
    1
)

anchor = '"BLACK VR | %s | A: Switch mode\\n"'
assert anchor in code
code = code.replace(
    anchor,
    '"BLACK VR | %s | A presses: %d\\n"',
    1
)

anchor = 'calibrationMode_ == 0 ? "POSITION" : "ROTATION",'
assert anchor in code
code = code.replace(
    anchor,
    anchor + "\n            calibrationToggleCount_,",
    1
)

src.write_text(code)

# Install alongside older test versions.
gradle = root / "Samples/XrSamples/XrInput/Projects/Android/build.gradle"
s = gradle.read_text()

old = "com.xavier.blackvr.calibrationfix"
assert old in s
gradle.write_text(
    s.replace(old, "com.xavier.blackvr.calibrationv6", 1)
)

print("Build #6 calibration button fix applied.")
