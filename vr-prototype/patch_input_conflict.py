from pathlib import Path
import sys

root = Path(sys.argv[1])
src = root / "Samples/XrSamples/XrInput/Src/main.cpp"
code = src.read_text()

start = "        auto cubeRotateState = GetActionStateVector2(actionRotateCube_);"
end = "        // Check validity of grip location before updating controllers with new location"

assert start in code
assert end in code

a = code.index(start)
b = code.index(end, a)

# Disable sample cube adjustments while holding the VR gun.
code = (
    code[:a]
    + "        if (!toolHeldInRight_) {\n"
    + code[a:b]
    + "        }\n"
    + code[b:]
)

# Reserve A for calibration when the gun is held.
old = "if (toggleColorState.changedSinceLastSync && toggleColorState.currentState) {"
assert old in code

code = code.replace(
    old,
    "if (!toolHeldInRight_ && toggleColorState.changedSinceLastSync && toggleColorState.currentState) {",
    1,
)

src.write_text(code)

# Give this test its own installation ID.
gradle = root / "Samples/XrSamples/XrInput/Projects/Android/build.gradle"
s = gradle.read_text()
assert "com.xavier.blackvr.calibration" in s

gradle.write_text(s.replace(
    "com.xavier.blackvr.calibration",
    "com.xavier.blackvr.calibrationfix",
    1,
))

strings = root / "Samples/XrSamples/XrInput/res/values/strings.xml"
s = strings.read_text()
strings.write_text(
    s.replace("BLACK VR Aim Test", "BLACK VR Cal Fix")
)

print("BLACK VR calibration input conflict patch applied.")
