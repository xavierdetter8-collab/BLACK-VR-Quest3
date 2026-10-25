from pathlib import Path
import sys

root = Path(sys.argv[1])
src = root / "Samples/XrSamples/XrInput/Src/main.cpp"
code = src.read_text()

start = "        // --- Create the model for the cube-spawning tool"
end = "        toolRenderer_.Init(toolGeometry.ToGeometryDescriptor());"

assert start in code and end in code, "Tool geometry not found"

gun = """
        // BLACK VR: first tracked weapon prototype.
        OVRFW::GeometryBuilder toolGeometry;
        const OVR::Vector4f dark{0.12f, 0.13f, 0.15f, 1.f};
        const OVR::Vector4f metal{0.38f, 0.40f, 0.42f, 1.f};

        auto part = [&](float x, float y, float z,
                        float w, float h, float d,
                        const OVR::Vector4f& color) {
            toolGeometry.Add(
                OVRFW::BuildUnitCubeDescriptor(),
                OVRFW::GeometryBuilder::kInvalidIndex,
                color,
                OVR::Matrix4f::Translation(x, y, z) *
                OVR::Matrix4f::Scaling(w, h, d));
        };

        part(0.f, 0.f, -0.12f, 0.035f, 0.04f, 0.24f, metal);
        part(0.f, 0.f, -0.03f, 0.055f, 0.06f, 0.15f, dark);
        part(0.f, -0.075f, 0.03f, 0.035f, 0.13f, 0.065f, dark);
        part(0.f, 0.04f, -0.14f, 0.012f, 0.02f, 0.025f, metal);
"""

a = code.index(start)
b = code.index(end, a)
code = code[:a] + gun + code[b:]

old = "            toolPose.Rotation *= OVR::Quatf({OVR::DegreeToRad(60.f), 0.f, 0.f}, 1.f);"
assert old in code, "Aim rotation not found"
code = code.replace(
    old,
    "            toolPose.Translation = toolPose.Transform({0.f, -0.025f, -0.06f});",
    1,
)
code = code.replace(
    'auto titleLabel = ui_.AddLabel("XrInput Sample"',
    'auto titleLabel = ui_.AddLabel("BLACK VR: Aim Prototype"',
    1,
)
src.write_text(code)

android = root / "Samples/XrSamples/XrInput/Projects/Android/build.gradle"
gradle = android.read_text()
gradle = gradle.replace(
    'applicationId "com.oculus.xrsamples.xrinput"',
    'applicationId "com.xavier.blackvr.aimtest"',
)
android.write_text(gradle)

strings = root / "Samples/XrSamples/XrInput/res/values/strings.xml"
strings.write_text(
    strings.read_text().replace(
        "OpenXR Input Sample", "BLACK VR Aim Test"
    )
)
print("BLACK VR aiming prototype patch applied.")
