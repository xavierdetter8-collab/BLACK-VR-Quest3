from pathlib import Path
import sys

root = Path(sys.argv[1])
src = root / "Samples/XrSamples/XrInput/Src/main.cpp"
code = src.read_text()

old = "{0_m, 1.1_m, -1.2_m},"
new = "{-0.35_m, -0.15_m, -1.1_m},"

assert old in code, "Calibration display location not found"
code = code.replace(old, new, 1)
src.write_text(code)

gradle = root / "Samples/XrSamples/XrInput/Projects/Android/build.gradle"
s = gradle.read_text()

assert "com.xavier.blackvr.calibrationv6" in s
s = s.replace(
    "com.xavier.blackvr.calibrationv6",
    "com.xavier.blackvr.calibrationv7",
    1
)
gradle.write_text(s)

print("Build #7: calibration display repositioned.")
